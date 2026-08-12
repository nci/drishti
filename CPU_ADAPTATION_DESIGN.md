# Drishti 纯 CPU / 核显适配设计方案

> 状态：**方案设计文档（不涉及代码改动）**
> 范围：让 `drishti`、`drishtipaint`、`drishtimesh`、`drishtiimport` 四个工具
> 在**纯 CPU 设备（可能无显示、无独立显卡）**上能启动并完成计算，渲染可降级。

---

## 1. 现状与结论速览

| 工具 | 计算核心 | GPU 依赖 | 无头启动障碍 |
|---|---|---|---|
| `drishtiimport` | 数据转换（netCDF/DICOM/raw→PVL） | **无**（纯 CPU） | 高：`main.cpp` 强制 `QApplication` + `QGLFormat` + 显示窗口 |
| `drishtipaint` | 标注/分割（GraphCut/骨架化/连通域） | **无**（纯 CPU） | 中：`main.cpp` 强制 `QApplication` + `QGLFormat`，但渲染为 2D |
| `drishtimesh` | 网格重建（gmsh/OpenVDB） | **少量**：`GL_COMPUTE_SHADER` 画笔 | 中：`main.cpp` 强制 `QApplication` + `QGLFormat` |
| `drishti`（主） | 三维体渲染 | **核心**：GLSL 着色器 + FBO 光线投射 | 高：`QGLFormat` + `QGLViewer` 渲染管线 |

**核心结论：**
- 除 `drishti` 主程序的三维渲染和 `drishtimesh` 的交互画笔外，**其余全部计算已经是纯 CPU**（GraphCut、ITK、OpenVDB、gmsh、连通域等）。
- 主程序渲染要求 **OpenGL 2.1 时代**（GLSL 1.0/1.3 + FBO），现代核显都能满足；**真正的"无 GPU"问题不是性能，而是"没有 OpenGL 上下文可用"**。
- 因此"纯 CPU 适配"的核心是两个问题：
  1. **无头/离屏启动**：让各工具在无显示环境下也能创建、跑计算、导结果。
  2. **渲染降级**：主程序在无 GL 上下文时回退到 CPU 光线投射（软件渲染）。

---

## 2. 设计目标（分级）

按可行性和价值分三级，建议按此顺序落地：

- **P0（必做，低风险）**：四个工具**无头/离屏启动**——把 `QApplication` 与 GL 上下文解耦，加 `--headless`/`--nogui` 命令行模式，让 CPU 计算部分能在无显示服务器上跑通。
- **P1（核心价值）**：`drishti` 主程序**CPU 光线投射回退**——在检测到无可用 GL/OpenGL 上下文时自动切换到软件渲染（或强制 `--cpu-render`）。
- **P2（增强）**：`drishtimesh` 的**交互画笔 CPU 化**——把 `GL_COMPUTE_SHADER` 画笔替换/回退为 CPU 实现。

> ⚠️ 说明：P1（CPU 光线投射）是**最大的工程**，涉及替换整个 GPU 渲染管线，工作量数周级；P0/P2 是可控的小中型改动。

---

## 3. 各工具现状与无头障碍（代码证据）

### 3.1 `tools/import/main.cpp`
- 使用 `QApplication`，且 `DrishtiImport` 继承自带窗口的类。
- 嵌入了 Python 解释器（`PythonEngine`），stdout/cerr 重定向到 `QDockWidget` 的 QTextEdit —— **意味着它默认依赖 GUI**。
- **无头改造**：把 `QApplication` 条件切换为 `QCoreApplication`（无 GUI 模式），Python/日志重定向改为文件或 `QTextStream`；计算入口（`raw2pvl`、`volumedata`）本身是纯 CPU 可直接复用。

### 3.2 `tools/paint/main.cpp`
- 使用 `QApplication` + `QGLFormat`，`DrishtiPaint` 基于 `QGLViewer`。
- 计算（`graphcut/*`、`livewire`、`morphslice`、`cc3d.h`、ITK 插件）**均为 CPU**，但 UI 全部挂在 GL 窗口上。
- **无头改造**：将分割/标注算法从 `QGLViewer` 交互中抽出为独立 CPU 接口；无头模式提供"加载 mask + 跑分割 + 导出结果"的命令行路径，跳过渲染。

### 3.3 `tools/mesh/main.cpp`
- 使用 `QApplication` + `QGLFormat`。
- 网格重建（gmsh `smoothMesh`、OpenVDB `VolumeToMesh`、`meshtools.cpp`）**均为 CPU**。
- 唯一 GPU 计算是 `trisets.cpp:4025` 附近调用的 `ComputeShaderFactory::paintShader()`（`GL_COMPUTE_SHADER`，需要 OpenGL 4.3+）。
- **无头改造**：同 3.2，抽出 CPU 网格重建入口；交互画笔做 CPU 回退（见 §5）。

### 3.4 `drishti/main.cpp`（主程序）
- `QApplication` + `QGLFormat`（`setSampleBuffers/doubleBuffer/rgba/alpha/depth`），窗口基于 `QGLViewer`。
- 渲染依赖 `DrawHiresVolume`/`DrawLowresVolume` 的 GL 纹理 + 着色器（`rcshaderfactory.cpp` 等），**无软件回退**。
- **无头改造**：最复杂，见 §4。

---

## 4. 主程序 CPU 光线投射回退设计（P1）

### 4.1 抽象渲染后端接口

引入一个**渲染后端抽象层**，让 `Viewer`/`DrawVolume` 不再直接调用 GL：

```
RenderBackend (interface)
 ├─ GPU_Backend      (现有 GL 实现，内部走 GL 纹理 + 着色器)
 └─ CPU_Backend      (软件光线投射，输出 QImage/像素缓冲)
```

`DrawHiresVolume::draw()` / `Viewer::paintGL()` 改为调用后端，由后端决定用 GL 还是 CPU。

### 4.2 CPU 光线投射核心算法

CPU 光线投射（volumetric raycasting）在 `CPU_Backend` 中实现：
1. **遍历体素**：对每个输出像素发出一条射线，沿射线步进采样体数据（`volumebase.cpp`/`volume.cpp` 中已有的体数据访问接口可复用）。
2. **传输函数**：复用现有 `SplinesTransferFunction` / LUT（`lookupTable()`、`lutSize()`），把采样值映射为颜色/不透明度并累加（front-to-back alpha blending）。
3. **步进优化**：使用**空空间跳跃（empty space skipping）**和**early ray termination**，弥补 CPU 性能短板。
4. **降采样**：交互时用低分辨率步进 + 重投影，静止时提分辨率（与现有"低分辨率/高分辨率"模式对应）。
5. **多线程**：用 `QThreadPool`/OpenMP 按扫描线分块并行 —— CPU 多核是关键加速手段。

### 4.3 后端选择逻辑

- 启动时探测 `GlewInit::initialise()` 是否成功；失败或检测不到 GL 上下文 → `--cpu-render` 自动启用 `CPU_Backend`。
- 也提供 `--cpu-render` 强制开关和 `--gpu-render` 强制开关。
- 无头模式下 `--nogui --cpu-render --render-to <file>` 可离线渲染出图。

### 4.4 工作量评估
- **大**：需要重写光线投射核心 + 后端抽象 + 多线程 + 降采样调度，涉及 `viewer.cpp`/`drawhiresvolume.cpp`/`rcshaderfactory.cpp` 大改。
- 建议**分阶段**：先实现 `--nogui` 离线 CPU 渲染（不碰交互），再渐进替换交互渲染。

---

## 5. mesh 交互画笔 CPU 化设计（P2）

### 5.1 现状（代码证据）
- `tools/mesh/computeshaderfactory.cpp` 创建 `GL_COMPUTE_SHADER` 程序，参数（`hitPt`/`radius`/`hitColor`/`blendType`/`blendFraction`/`blendOctave`/`bmin`/`blen`/`roughnessType`）在 `trisets.cpp:4025` 处设置，用于在体数据上画 stroke（把画笔写入 volume）。
- 这是 OpenGL 4.3+ 特性，核显/旧驱动可能不支持（`createPaintShader` 失败只弹 MessageBox，无 CPU 回退）。

### 5.2 改造方案
- 新增 `CpuPaintBackend`，实现**同样的语义**：给定命中点、半径、颜色、混合类型/比例，在体数据（`m_solidTexData`）对应体素区域写入/混合颜色。
- 把 `trisets.cpp` 的画笔调用抽到 `PaintBrush` 接口，根据后端能力选择 GPU compute 或 CPU 实现。
- **关键点**：CPU 画笔只影响被画笔命中的**局部体素球体区域**，计算量可控；交互时用低半径 + 局部更新即可流畅。
- 检测 `GL_COMPUTE_SHADER` 是否可用（`glewIsSupported`），不可用则自动回退 CPU。

### 5.3 工作量评估
- **中**：抽出画笔接口 + 实现 CPU 球体写入/混合 + 能力探测，约 1~3 天量级。

---

## 6. 无头/离屏启动策略（P0）

### 6.1 统一命令行约定
为四个工具统一增加参数：
- `--nogui` / `--headless`：无 GUI 运行（`QCoreApplication`，跳过 `QGLFormat`/窗口创建）。
- `--render-to <file>`：离线渲染输出（主程序 + mesh）。
- `--cpu-render` / `--gpu-render`：强制渲染后端。
- 对 paint/import/mesh：`--in <file> --out <file> --op <op>` 指定命令行计算入口。

### 6.2 无 GUI 下的 Qt 选型
- 有显示但无独立显卡：**保持 `QApplication`**（核显提供 OpenGL 2.1，主程序渲染可跑）。
- 完全无显示（无 X/Wayland/桌面）：用 `QCoreApplication` + `QOffscreenSurface`（Qt 离屏渲染，仅当需要 GL 时）；纯 CPU 计算则无需任何 GL。
- **Linux 服务器无头**：`QT_QPA_PLATFORM=offscreen` 或 `--platform offscreen` 可辅助，但代码层面应优先用 `QCoreApplication` 路径避免依赖 QPA。

### 6.3 具体改动点（各工具 main.cpp）
| 文件 | 改动 |
|---|---|
| `tools/import/main.cpp` | 条件选择 `QCoreApplication`；Python/日志重定向到文件；抽取 CPU 转换入口 |
| `tools/paint/main.cpp` | 条件选择 `QCoreApplication`；抽出分割/标注 CPU 接口 |
| `tools/mesh/main.cpp` | 条件选择 `QCoreApplication`；抽出网格重建 CPU 入口 |
| `drishti/main.cpp` | 条件选择 `QCoreApplication` + `CPU_Backend` 离线渲染 |

---

## 7. 优先级建议与落地路线

| 阶段 | 内容 | 工作量 | 价值 |
|---|---|---|---|
| **1 (P0)** | 四工具无头/离屏启动 + CLI 计算入口 | 中 | 高（纯 CPU 设备可跑计算） |
| **2 (P2)** | mesh 画笔 CPU 回退 | 中 | 中（核显更稳） |
| **3 (P1-a)** | 主程序 `--nogui --cpu-render` 离线渲染 | 大 | 高（无显示也能出图） |
| **4 (P1-b)** | 主程序交互 CPU 光线投射 | 很大 | 高（完全纯 CPU 交互） |

**建议**：先做阶段 1（无头启动，低风险高价值，且不破坏现有 GPU 路径），再做阶段 2（画笔回退），最后评估阶段 3/4（CPU 光线投射，需权衡投入产出）。

---

## 8. 关键风险与注意点

1. **CPU 光线投射性能**：体渲染在 CPU 上很慢（尤其大体积）。必须靠降采样、空空间跳跃、early termination、多线程并行来缓解；建议默认"低分辨率交互 + 高分辨率静止"策略。
2. **内存**：核显共享内存，CPU 路径也会把体积加载进内存。大体积（>2GB）在无显存设备上受限，需依赖现有 brick/子体积加载策略。
3. **不破坏现有 GPU 路径**：所有改造应通过后端抽象/能力探测实现**自动回退**，保留 GL 版本优先，避免核显（能跑 GL）反而退化。
4. **QGLViewer 强耦合**：主程序和 mesh 的 `Viewer` 继承自 `QGLViewer`，纯 CPU 交互渲染需要把渲染逻辑从 QGLViewer 的 `paintGL` 解耦，这是阶段 3/4 的主要复杂度来源。

---

## 9. 目标平台：Win11 + i7-13700H 核显（已确认）

> 已确认：**Win11 核显笔记本，CPU i7-13700H，保留 GL 路径优先。**

### 9.1 平台能力评估（重要前提修正）

i7-13700H（Raptor Lake 移动版）配套 **Intel Iris Xe 核显**（96 EU / 128 EU，取决于具体型号），能力如下：
- **OpenGL 4.6 / 4.5** 完整支持 —— 远高于本项目的 **GLSL 1.0/1.3 要求**，主程序渲染直接能跑，**不需要 CPU 光线投射**。
- 支持 `GL_COMPUTE_SHADER`（OpenGL 4.3+）—— mesh 交互画笔也能直接跑。
- **没有独立显存**：纹理/缓冲占用**系统内存**（通常从 16GB/32GB 共享一部分）。

**因此，本平台的核心问题不是"无 GPU"，而是：**
1. **显存受限**（共享系统内存，大体积数据的 GPU 纹理上限低于独立显卡）。
2. **算力相对弱**（EU 数少，大体积光线投射帧率偏低）。
3. **无头场景**（如需远程/无人值守跑计算）。

### 9.2 核显内存控制的现成杠杆（代码证据）

已定位到关键内存控制点：
- **`Global::m_textureSize = 25`**（`global.cpp:176`，注释 `512x256x256 (9+8+8)`）：这是 brick 纹理的**幂次预算**（三个方向指数和 = 25），直接决定高分辨率体积在 GPU 侧占用的纹理内存。
- **`VolumeBase::loadDummyVolume`/`loadVolume`**（`volumebase.cpp`）：用 `px2+py2+pz2 > 22` 计算**低分辨率子采样级**（`m_subSamplingLevel`），限制低分辨率纹理体积。
- **brick 机制**（`bricks.cpp` + `drawhiresvolume.cpp` 的 `glTexImage3D(GL_TEXTURE_2D_ARRAY)`）：把大体积切成可管理的 brick，逐步上载。

**核显适配的核心思路**：动态降低 `m_textureSize` 预算 + 提高低分辨率子采样，在共享内存压力大时自动调低 GPU 纹理占用，换取稳定不崩、可交互。

### 9.3 针对本平台的适配优先级（修订）

| 优先级 | 内容 | 理由 |
|---|---|---|
| **A（最高，低风险）** | **核显内存/帧率调优**：动态 `m_textureSize`、子采样自适应、brick 上载策略 | 直接解决本平台最大痛点（共享显存 + 弱算力），不动渲染架构 |
| **B（中）** | mesh 画笔 **compute→CPU 兜底**（能力探测 + 回退） | 核显能跑 compute，但留兜底更稳；改动小 |
| **C（中）** | 四工具 **`--nogui` 无头计算入口** | 支持远程/无人值守跑 import/paint/mesh 的 CPU 计算 |
| **D（大，可选）** | 主程序 **CPU 光线投射回退** | 本平台核显已够用，**通常不需要**；仅在无任何 GL 上下文时才用，属兜底而非主力 |

### 9.4 保留 GL 路径优先（已确认）

- **不替换现有 GL 渲染**，GPU 路径保持默认优先。
- 所有改造走**能力探测 + 自动回退**：
  - `GlewInit::initialise()` 成功 + 核显 GL 可用 → 走 GL。
  - 检测到无 GL 上下文 / 驱动过旧 / 显存不足 → 才降级（mesh 画笔走 CPU、主程序走软件光线投射）。
- 这样既不影响核显用户的现有流畅度，又给极端情况兜底。

### 9.5 面向本平台的落地路线（修订）

**阶段 1（建议先做）— 核显调优**（A）
- 把 `Global::m_textureSize` 改为**可在设置中配置 + 自动探测显存**。
- 添加"低显存模式"：降低 brick 纹理预算 + 提高子采样，用分辨率换稳定性。
- 小改动、可逆、直接改善本平台体验。

**阶段 2 — mesh 画笔 CPU 兜底**（B）
- 能力探测 `GL_COMPUTE_SHADER`，不可用时走 CPU 画笔。

**阶段 3 — 无头计算入口**（C）
- `--nogui` 模式跑 import/paint/mesh 的 CPU 计算（远程/批处理）。

**阶段 4（可选）— 主程序 CPU 渲染兜底**（D）
- 仅当确有"完全无 GL 上下文"需求时再投入；本平台核显通常用不到。

---

## 10. 待确认的开放问题（供后续细化）

1. **核显具体型号 / 系统内存大小**：i7-13700H 的 Iris Xe 是 96EU 还是 128EU？笔记本是 16GB 还是 32GB 内存？（决定显存预算上限）
2. **数据体积典型规模**：通常处理多大体积？（如 512³ / 1024³ / 更大？）—— 决定 brick/子采样参数。
3. **性能底线**：交互可接受的帧率？静止渲染可接受时间？（决定降采样/优化深度）
4. **是否需要无头/远程**：是否有 SSH/无人值守跑计算的需求？（决定是否做 `--nogui`）
5. **输出形态**：无头模式是否需要批处理/脚本化（一次处理多个文件）？
