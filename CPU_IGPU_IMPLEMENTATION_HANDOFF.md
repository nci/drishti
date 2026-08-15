# Drishti CPU + Intel/AMD 核显实施交接

> 交接日期：2026-08-10；背景补充复核：2026-08-13
> 上游审计起点：`b53bd9790829dbeb38cbe0f160e79a95349905d2`
> 当前实现检查点：`e731972d2c8663b001ce06384b9b4074854d0c00`（`codex/cpu-igpu-worktree-checkpoint-20260813`，`checkpoint: preserve CPU and iGPU adaptation work`）
> 目标平台：Windows 11、i7-13700H 核显笔记本，以及近五年发布的 Intel/AMD 核显设备
> 目标架构：CPU 执行导入、直方图、转换、分割和网格构建；核显执行 OpenGL 交互显示
> 非目标：纯 CPU 软件光栅化、无显示设备运行、向上游 GitHub 推送

## 0. 给目标设备智能体的直接指令

请把本文件当作实施任务书，而不是调查建议。先读取目标设备仓库、已安装程序和问题图片的只读信息，再按第 7 节分阶段实施和验证。

第 4 节描述的是当前实现检查点的状态、已达到的契约和补充背景复核确认的边界。目标笔记本未必拥有该检查点，目标智能体必须先核对 `git rev-parse HEAD` 和目标源码：已有则验证，缺失则移植或实现；标为未完成/P0 的项目即使在 `e731972` 上也尚未关闭，不能因为相邻条目写着“已实现”就跳过编码。

正式源码、测试和此前新增文件已经纳入检查点 `e731972d2c8663b001ce06384b9b4074854d0c00`，不再需要复制整个脏工作树来保住实现。该检查点创建后，本文件又按补充背景修订；交接时必须同时保留当前版本的本 Markdown，不能只解压 `e731972` 的旧文档。当前工作区的其他 untracked 项（如 `.lab-agent/`、`aqtinstall.log`、`__pycache__/`、根目录 `*.obj` 和名为 `-` 的文件）是审计/构建产物，不是产品源码，也不应成为代码智能体的输入载荷。

该检查点所在本地分支尚未 push，目标机器不能假定 `git fetch origin` 可以取得它。如果目标机器能通过 bundle 或其他方式访问该 Git 对象，优先让目标智能体从 `e731972` 建分支或按仓库策略移植该提交；如果只能传文件，导出该检查点的源码归档，并把当前修订后的本文件作为单独交接件放在归档旁：

```powershell
$checkpoint = 'e731972d2c8663b001ce06384b9b4074854d0c00'
$dst = 'D:\transfer'
git archive --format=zip --output "$dst\drishti-$($checkpoint.Substring(0,7)).zip" $checkpoint
Copy-Item -LiteralPath .\CPU_IGPU_IMPLEMENTATION_HANDOFF.md -Destination $dst
```

无论采用 Git 还是归档，至少确认以下检查点新增源码存在：

```text
framebufferbudget.h
cpumeshpaint.h
meshvertexbuffer.h
drishti/plugins/itk/itkmemoryadmission.h
tools/import/importmemoryadmission.cpp
tools/import/importmemoryadmission.h
tools/import/pluginoperationstatus.h
tools/import/tests/
tools/paint/slabsavetransaction.cpp
tools/paint/slabsavetransaction.h
tools/paint/tests/
```

不要从相邻的 `_external\bin` 或本机 `.lab-agent\build` 收集 DLL；那里是审计期间的 shadow-build 产物。目标机必须用自己的正式 Qt/MSVC/第三方依赖重新编译。

若目标智能体只收到本 Markdown，则把第 4 节的行为契约作为实现标准，从目标仓库逐项编码，不能假定上游 `b53bd97` 或目标仓库已经包含 `e731972` 的改动。

必须遵守：

1. 开始前记录 `git rev-parse HEAD`、`git status --short` 和现有二进制版本；保留用户所有未提交修改。
2. 先单独构建和部署导入插件，确认选择/扫描原来的 10 张图片不再拖垮系统；随后构建包含 `importmemoryadmission.*` 和 `raw2pvl.cpp` 修改的 Import EXE 验证 PVL 转换。不要一开始就替换三套 OpenGL EXE。
3. 正式插件必须与目标 Drishti 使用完全相同的 Qt 5.15、MSVC、CRT 和插件 ABI。禁止部署本交接机器生成的 Anaconda Qt 测试 DLL。
4. 先测合成小样，再按 1、2、4、10 张逐级测试原始数据。系统一旦开始持续换页，不要继续重复触发完整问题数据。
5. 核显路径必须是 Windows Desktop OpenGL。日志中的 Renderer 不得是 `GDI Generic`、`Microsoft Basic Render Driver` 或远程桌面的软件实现。
6. 不要把“程序能启动”当作“全部功能可用”，也不要把“全部功能可用”当作“任意尺寸数据都流畅”。按本文件列出的不同口径分别验收。
7. 不要把历史实验补丁中的 `BATCH_SIZE = 32` 当成现场既定事实。只有目标 `drishtiimport.exe` 能对应到包含该补丁的源码或二进制证据时，才能把它列为现场首因。

`CPU_ADAPTATION_DESIGN.md` 中“主程序只需 OpenGL 2.1”和“Paint 是纯 2D”等判断与当前源码不符，不应作为实施依据。`DRISHTI_END_TO_END_WORKFLOW_FIX_HANDOFF_2026-08-12.md` 是补充背景后的全链路问题基线，`DRISHTI_IMPORT_FREEZE_HANDOFF.md` 保存导入卡死审计过程；实现状态发生冲突时，以当前源码、可复现测试和全链路问题基线为准，不能用本文较早的“已完成”概括覆盖后来确认的缺口。

## 1. 结论和概率

目标不是纯 CPU 软件渲染，而是 **CPU 计算 + Intel/AMD 核显 OpenGL 显示**。仓库没有 CUDA/OpenCL/NVIDIA 专用运行时依赖；主要图形接口是 Desktop OpenGL，因此 i7-13700H 和近五年的 Intel/AMD 核显在硬件能力上具备可行性。

不要只按仓库 README 中“OpenGL 3.3”这一旧说明验收。当前源码和 shader 的实际下限更高：主程序请求 4.5 compatibility，Paint/Mesh 请求 4.2 compatibility；以运行日志中的真实上下文为准。

下列数字是基于源码覆盖面、已完成测试和剩余未知项给出的**工程置信度**，不是在目标笔记本上测得的成功率，也不是帧率保证：

当前证据支持以下估计：

| 口径 | 当前估计 | 含义 |
|---|---:|---|
| CPU + 近五年 Intel/AMD 核显这一架构本身 | **90%–95%** | Drishti 的计算链路没有 CUDA/OpenCL/NVIDIA 硬依赖；Desktop OpenGL 4.2/4.5 驱动能力是主要图形前提 |
| 已审计插件的选择、元数据检查和逐张解码不再拖死系统 | **90%–96%** | TIFF/Image Stack 在进入 codec 前做尺寸与实时内存/Commit 门控，并采用有界、可取消路径；单次第三方 codec 调用仍不能安全强停 |
| 用户原来约 10 张图片的完整 Import -> PVL 流程不再拖死系统 | **80%–90%** | 导入、滤波、PVL 分片初始化/写入/取消/回滚和头文件提交已接成完整事务；仍缺原图、目标 EXE、RAM/页面文件数据和现场复现证据 |
| 当前代码的单通道灰度、典型尺寸核心工作流 | **82%–88%** | 指成功路径上的 Import -> PVL -> 主程序显示 -> Paint 标注 -> Mesh 构建/编辑/导出；坏候选保留旧 Drishti 工作集尚未实现，P0-6 关闭前不能据此发布 |
| 目标机正式构建并跑通第 8 节矩阵后的核心工作流 | **90%–94%** | 前提是匹配 ABI 的完整依赖、真实 Intel/AMD Renderer 和用户原始数据均通过；中心估计约 **92%** |
| 字面意义上的每个菜单、ITK/VED/ML/OpenVR、罕见格式都能运行 | **55%–70%** | 中心估计约 **62%**；外部运行时、旧插件 ABI、罕见输入和未审计路径仍明显降低置信度 |
| 典型尺寸下日常导入、显示、标注和构建“较流畅” | **65%–80%** | 尚未在 i7-13700H 核显机计时；多个 CPU 长任务仍在 UI 线程，流畅度不能由内存安全测试代替 |
| 大 mask、复杂网格、接近内存上限时所有任务仍流畅 | **30%–50%** | 整卷 snapshot、同步算法、CPU 网格画笔 `O(total vertices)` 和核显共享带宽仍会带来长停顿 |

因此，用户所说的 **CPU + 核显日常核心工作流**目前用单值表达约为 **85%**；“Drishti 字面意义上的所有任务”约为 **62%**；目标笔记本用正式依赖和真实数据跑通验收矩阵后，核心工作流约为 **92%**。这些数字是工程置信度，不是帧率或任意规模保证。

目标笔记本的 RAM 容量尚未提供，因此不能再把概率压缩成更精确的单值。8 GiB 设备应把上述核心口径下调约 5–10 个百分点；16 GiB 可按表中基准；32 GiB 会增加余量，但不能消除核显带宽、驱动和最大纹理尺寸限制。内存必须启用系统管理页面文件，关闭页面文件会显著放大系统级卡死风险。当前工作树已用实时可用物理内存和 Windows Commit 余量替换原先按总 RAM 粗估的准入；超预算时会在危险分配前拒绝整卷加载。由于现有 Viewer 和部分算法仍要求整卷裸指针，这项保护不等于已经提供大卷 out-of-core 编辑。

## 2. 项目实际如何使用 CPU 和核显

| 程序 | CPU 工作 | 核显/OpenGL 工作 | 当前实际要求 |
|---|---|---|---|
| `drishtiimport` | 图片解码、元数据检查、直方图和 PVL/格式转换 | 无 | 导入路径不依赖 OpenGL；原卡死首先是 CPU/内存/I/O 问题 |
| `drishti` | 文件读取、数据组织、部分处理、视频编码 | 三维体渲染、FBO、纹理和常规 vertex/fragment shader | 当前程序请求并检查 OpenGL 4.5 Compatibility Profile |
| `drishtipaint` | GraphCut、形态学、连通域、LiveWire、标注数据处理 | 三维预览、切片合成、FBO 和 raycast shader | 当前 shader 使用到 GLSL 4.20；不是纯 2D CPU 程序 |
| `drishtimesh` | 网格生成、几何处理、导出和网格画笔计算 | 交互显示、FBO 和常规 vertex/fragment shader | 当前程序请求并检查 OpenGL 4.2；网格画笔已走 CPU |

源码编译本身只使用 CPU；没有独显不会阻止 qmake/MSVC 构建。Windows 构建的现实阻碍是 Qt、MSVC、QGLViewer、GLEW、ITK、OpenVDB、Assimp、netCDF、FFmpeg 等版本和 ABI 必须匹配，运行三维界面时才需要合格的 OpenGL 驱动。

因此，正确的兼容设计是：

```text
磁盘/图片
  -> CPU：校验、顺序解码、直方图、PVL/标注/网格计算
  -> 有界系统内存
  -> 核显：低分辨率交互、纹理/FBO、静止时提高质量
```

不是：

```text
把整个 Drishti 改成 CPU 软件渲染器
```

后者需要重写体渲染后端，工程量和性能风险都明显更高，对 i7-13700H 核显设备没有必要。

当前活动构建路径中没有 `glDispatchCompute` 调用。仓库仍保留旧 `computeshaderfactory.cpp` 源文件，但当前 `.pro` 未把它作为运行路径，主程序和 Mesh 的网格画笔均已改为共享的 `CpuMeshPaint`。因此后续重点是 CPU 画笔性能和结果一致性，不是再补一套 compute CPU fallback。

## 3. 已确认的源码缺陷与待现场确认的首因

发生卡死的阶段属于 `drishtiimport`，不创建 OpenGL 上下文，因此它与核显的 shader 性能无关。可以确认上游基线存在同步、不可取消的 GUI 线程导入，以及格式校验、边界检查和像素查询资源管理缺陷；但当前没有目标二进制、原始图片或资源曲线，不能把现场卡死唯一归因于某一条代码路径。

上游 `b53bd97` 的 TIFF 直方图路径是 GUI 线程中的**顺序解码**，并复用一张切片缓冲。固定 `BATCH_SIZE = 32` 和 `QtConcurrent::blockingMap()` 只存在于后来已移除的实验补丁中。只有目标设备实际运行的二进制包含该补丁时，下面的批量内存机制才可认定为现场首因：

```text
GUI 线程进入实验版 TIFF 插件
  -> 同时预分配并解码最多 32 张完整切片
  -> 10 张输入同时持有 10 张解码缓冲
  -> CPU、磁盘、Working Set 和系统 Commit 同时上升
  -> Windows 大量 Hard Fault / 页面文件 I/O
  -> DrishtiImport 与 Explorer 一起表现为未响应
```

仅计算 16 位灰度像素，不含 Qt、libtiff 和其他进程：

| 单张尺寸 | 单张解码后 | 10 张同时存在 |
|---|---:|---:|
| 4096 x 4096 | 32 MiB | 320 MiB |
| 8192 x 8192 | 128 MiB | 1.25 GiB |
| 16384 x 16384 | 512 MiB | 5 GiB |

无论是否包含该实验补丁，都确认了以下独立风险：

- 上游 GUI 线程同步完成 TIFF 解码和直方图，单张很大或 I/O 很慢时也会长时间无响应。
- TIFF 栈未完整校验时，RGB、混合尺寸/位深、异常页和整数溢出可能写越界。
- 旧 `rawValue()` 每次鼠标查询都会泄漏一张完整切片。
- 普通 Image Stack 对损坏图片和尺寸不一致缺少失败传播。

这些缺陷都与“DrishtiImport 和 Explorer 一起卡死”的症状相符：可能是系统 Commit/页面文件压力，也可能是输入不匹配导致的堆破坏。目标智能体必须记录实际导入菜单、目标二进制、每张图片解码尺寸、系统 Commit 和 Hard Fault 曲线后再确认首因。压缩 TIFF 的文件大小不能代表解码后内存。

## 4. 当前工作树实现状态与边界

### 4.1 TIFF 插件

文件：

```text
tools/import/plugins/tiff/tiffplugin.cpp
tools/import/plugins/tiff/tiffplugin.h
tools/import/plugins/tiff/tiff.pro
```

实现契约：

- 全栈只读元数据预检；逐文件、逐页面验证尺寸、位深、sample format、灰度通道、planar config、photometric、orientation、scanline 和缓冲边界。
- 支持多文件、多页 TIFF、BigTIFF 和 Windows Unicode 路径；Windows 使用 `TIFFOpenW`。
- 当前未实现的 RGB、tiled、`MINISWHITE`、非 `TOPLEFT`、unsigned 32-bit 和混合布局在解码前明确拒绝。
- 所有尺寸和偏移使用检查过的 64 位计算，并在 Qt 5 现有 `int` 接口上限前拒绝。
- 每次 `TIFFReadScanline()` 前按当前页的 slice、scanline、codec safety 峰值查询实时可用物理内存和 Windows Commit；预算不足时在读像素前拒绝。
- 直方图统计在单个后台任务中顺序解码，只复用一张完整切片；GUI 可刷新并可取消。
- 8/16 位有符号直方图正确覆盖 `-128` 和 `-32768`；统计阶段使用 64 位计数。
- `rawValue()` 只读取目标扫描行，不再为一个像素解码并泄漏整张切片。
- 打开、解码、取消或统计失败后恢复插件先前状态。
- `TIFF_INCLUDE_PATH` 和 `TIFF_LIBRARY_PATH` 现在置于全局 vcpkg 路径之前，避免两套 libtiff 同时存在时选错 ABI。

### 4.2 普通 Image Stack

文件：

```text
tools/import/plugins/imagestack/imagestackplugin.cpp
tools/import/plugins/imagestack/imagestackplugin.h
tools/import/plugins/imagestack/imagestack.pro
```

实现契约：

- 初始堆栈校验在单个后台任务中只读取每张图片的尺寸元数据，不再为了选择 10 张图片而同时或连续完整解码全部像素；支持进度和文件间取消。
- 每张图片先用 `QImageReader::size()` 获取元数据，并按最坏 12 B/pixel（8 B/pixel codec/source 峰值 + 4 B/pixel ARGB32 目标）加 64 MiB codec safety 做实时物理内存/Commit 准入；预览、转换和导出真正调用 `reader.read()` 前会再次检查。无法在解码前确定尺寸时保守拒绝。
- 尺寸不一致和超过 Qt 5 `int` 缓冲容量在初始校验时失败；像素 payload 损坏若元数据仍可读，则在首次实际解码时明确失败并传播，不会把空图继续传下去。
- `setFile()` 正确传播失败；预览和像素查询检查索引、空图、尺寸和格式转换。
- `rawValue()` 使用正确扫描行，由 `QImage` RAII 管理内存。
- 裁剪和 RGB(A) 导出的临时切片/通道缓冲也使用受检容量、实时准入、`nothrow` 和 RAII；活动路径已无 throwing `new[]`。
- RGB 与 RGBA 的解码输出严格按 3 B/pixel 与 4 B/pixel 打包，修复了 3 B/pixel 目标缓冲被 4 字节 `QRgb` 写穿的问题。
- 单通道裁剪导出支持取消，逐 scanline 用 `qRed()` 读取，并通过 `QSaveFile` 检查 header、payload、最终大小和提交；失败或取消不会留下半文件。
- RGB(A) 裁剪导出复用父级 hardened `VolumeFileManager`；插件目录中重复、行为较弱的同名文件管理器已经删除。
- 插件显式链接 Qt Concurrent。

### 4.2a Import 主程序到 PVL 的端到端内存准入

文件：

```text
tools/import/importmemoryadmission.cpp/.h
tools/import/pluginoperationstatus.h
tools/import/raw2pvl.cpp
tools/import/volumedata.cpp
tools/import/import.pro
tools/import/tests/import_memory_admission_smoke.cpp/.pro
tools/import/tests/plugin_error_bridge_smoke.cpp/.pro
```

实现契约：

- 公共准入层使用受检 `uint64_t/size_t`，每个危险阶段按实时可用物理内存和 Windows Commit 双门控，并预留 `max(2 GiB, RAM*20%)` 系统空间及 `clamp(RAM*6.25%, 512 MiB, 2 GiB)` 核显共享空间。
- PVL、Batch、MHD、Merge、Quick RAW、VDB、等值面/网格等显式大缓冲在打开输出或分配前估算峰值；VDB 与网格阶段分别按约 32/96 B/voxel 保守估算。
- Quick RAW 明确只接受 scalar 数据；RGB/RGBA 不再按 1 B/voxel 分配后让插件写入 4 B/pixel。
- 转换切片和滤波窗口使用连续 RAII 缓冲、`new (std::nothrow)` 和错误传播；滤波峰值包含 `(2*spread+1)` 个切片窗口，不再只按单切片判断。
- `VolumeData` 三个方向预览使用事务式候选缓冲，全部分配成功后才替换旧状态。
- `VolumeData::getDepthSlice()` 返回显式成功状态，并通过 Qt 元对象可选查询插件的 `Q_INVOKABLE lastError()`；Image Stack/TIFF 解码失败可传播到 PVL、MHD、Merge、Quick RAW、VDB、网格和图片导出调用方。没有该方法的旧 `VolInterface/1.0` 插件仍按旧行为工作，接口 vtable/ABI 未改变。
- 预算或分配失败时停止当前阶段、保留可重试状态并给出 required/physical/Commit 诊断；安全拒绝不等于大数据功能已经 out-of-core 化。
- 仍无法在不破坏第三方库状态的前提下强杀单次 `QImageReader`/libtiff codec 调用。外层任务可取消，但损坏 codec 若永久不返回，仍需要进程级隔离才能提供硬超时。
- TIFF 的目录/IFD 元数据枚举仍在 GUI 线程；极端页数或异常慢介质可能造成长停顿。并行等值面任务使用 point-in-time 准入，多个任务同时获批后仍可能竞争同一物理/Commit 余量；OpenVDB/Qt 内部分配也只是保守估算，不是硬配额。

### 4.2b Import 输出事务、取消和 RGB 像素契约

文件：

```text
tools/import/volumefilemanager.cpp/.h
tools/import/raw2pvl.cpp/.h
tools/import/remapwidget.cpp
tools/import/plugins/imagestack/imagestackplugin.cpp/.h/.pro
tools/import/plugins/imagestack/imagestackpixelconversion.h
```

实现契约：

- Import `VolumeFileManager` 的尺寸、slice byte count、文件 offset 和 slab 大小全部使用受检 64 位计算；初始化采用同目录临时文件，逐段检查 exact read/write、seek、flush 和最终尺寸，并允许在预分配期间取消。
- 覆盖已有 PVL/RAW slab 时先保存 backup；析构或任何未提交返回都会自动 rollback。`createFile()`、`setSlice()`、`commitFileCreation()` 和 `rollbackFileCreation()` 均返回状态并提供 `lastError()`。
- `savePvl()`、`batchProcess()` 和 `mergeVolumes()` 已检查所有创建和分片写入；取消会立即返回，不再继续尾部 padding 或显示 Done。数据完整后才用 `QSaveFile` 原子提交 PVL header，随后提交所有 slab 事务。
- Z 采样倍率不再大于 depth，XY 倍率不再大于 `min(width,height)`，因此不会生成零深度、零宽高或零 slab 容量。
- MHD header 和 RAW payload 都先写临时文件并检查短写；取消保留旧文件。替换 RAW 时保留旧备份，若 header 最终提交失败则恢复旧 RAW。
- Quick RAW 使用 `QSaveFile`，检查 13 字节头、seek、逐 slice payload、最终大小和 commit；取消直接丢弃临时文件并保留旧输出。
- RemapWidget 的 RAW 与普通图片逐文件原子提交，检查容量、内存准入、slice 解码、图片编码和 commit，并允许在文件之间取消。
- Image Stack 的 RGB 与 RGBA 像素契约分别是严格 3 B/pixel 与 4 B/pixel；不再用 3 B/pixel 缓冲写入 4 字节 `QRgb`。RGB(A) 导出复用父级受保护文件管理器，移除了重复 `createFile(true)` 和旧的同名实现。
- `VolumeData::clear()` 删除插件接口后立即置空，避免失败清理后再次析构造成 double delete。
- 单次第三方 codec 调用仍无法在同一进程内强制中止；当前取消粒度是图片之间或 TIFF scanline 之间。对恶意 codec 的硬超时需要独立 worker process，不属于本次改动。

### 4.3 核显启动基线

文件：

```text
drishti/main.cpp
drishti/glewinitialisation.cpp
drishti/mainwindow.cpp
drishti/viewer.cpp
drishti/trisets.cpp
drishti/shaderfactory.cpp
tools/mesh/main.cpp
tools/mesh/glewinitialisation.cpp
tools/mesh/mainwindow.cpp
tools/mesh/viewer.cpp
tools/mesh/trisets.cpp
tools/mesh/shaderfactory.cpp
tools/paint/main.cpp
tools/paint/viewer.cpp
framebufferbudget.h
```

当前实现状态（已完成项与缺口）：

- Windows 强制 Desktop OpenGL，避免落到不兼容的 ANGLE/OpenGL ES 路径。
- 主程序、Paint、Mesh 分别请求 OpenGL 4.5、4.2、4.2 Compatibility Profile。
- 默认关闭 MSAA，减少核显启动时的颜色/深度缓冲压力。
- 输出 Vendor、Renderer、OpenGL 和 GLSL 信息。
- 当前日志**尚未**输出 `GL_CONTEXT_PROFILE_MASK` 的数值和解析结果，也没有在源码中主动拒绝 `GDI Generic`、`Microsoft Basic Render Driver` 等通用/软件 Renderer；现有版本/profile 检查不能替代这两项。目标实现必须补齐日志和明确拒绝/错误页，不能只把它们留作人工验收规则。
- GLEW、版本、profile 或通用 FBO 能力检查失败时停止后续 shader/FBO 初始化。
- 延迟首次 FBO 分配；主 Viewer 的 LUT texture 也移到上下文验证成功后创建。
- 三个 Viewer 都有 `rendererReady`/初始化状态；启动 shader 和实际 FBO 成功后才进入正常渲染，失败时用 QPainter 显示错误页。
- 主程序、主 raycast、Paint 和 Mesh 的活动 shader loader 会识别 legacy builtin，并把对应 `#version ... core` 源码在编译入口规范化为 compatibility。
- 上述 loader 使用稳定的内容哈希标签，记录完整的失败 compile/link log，并清理半创建的 shader/program。
- 延迟资源失败会锁存，避免每帧重新编译或重复弹窗；lighting 可降级到基础光照，Prune 会关闭 empty-space skip 并阻止无效操作，Mesh 的可选 ClearView 后处理会跳过。
- 目前只有三处 FBO 使用 `FramebufferBudget::evaluate()` 和 512 MiB 硬上限：主程序 Trisets 按 68 B/pixel、Mesh Trisets 按 100 B/pixel、Paint preview 按 48 B/pixel。两套 Trisets 还具备受检尺寸、事务式 candidate 分配和失败尺寸缓存；失败后走基础网格显示，不在每帧重试同一失败尺寸。
- **该预算不是全局 FBO 预算。**Drishti/Mesh Viewer、`DrawHiresVolume`、lighting、prune 和 `RcViewer` 等 FBO 路径尚未接入同一准入层；Paint Viewer preview 是上述三处已预算路径之一，但其状态恢复仍需按 P0-5 验证。现有公式也只计算 candidate attachment 集合，没有计入替换时仍存活的旧附件、驱动 staging 和其他同时驻留资源；事务式替换的瞬时占用可能接近公式结果的两倍。后续智能体必须逐路径补齐或给出等价的受检上限，不能把三处覆盖外推为全部 FBO 已受控。
- 两套 `ScopedTrisetGlState` 已保存并恢复 DRAW/READ framebuffer、renderbuffer、VAO、array buffer、viewport、program、blend、depth、color mask、cull/front-face/polygon mode、active texture，以及纹理单元 0–7 和原 active unit 的 2D/rectangle binding 与 enable 状态。旧内部 FBO/renderbuffer/texture 被替换时，保存的绑定会映射到新对象，避免恢复到已删除对象。**仍不是通用 GL 快照**：lighting、line smooth/width、point state、depth range、scissor、clear values、draw/read-buffer selection 未覆盖，纹理保护也有 target/unit 范围；见 P0-5 的哨兵验证要求。
- Mesh/shadow shader 失败按源码锁存，不再每帧重新编译或反复弹窗。Paint crop 改变触发 raycast shader 重建时也检查返回值；失败源码被锁存，渲染、拾取和读回均不得使用旧 shader 或陈旧 FBO。
- 正常每帧的整窗口 GPU -> CPU 读回已经移除；只在冻结画面或消息首次显示等确实需要静态背景时捕获，避免核显共享内存总线被同步读回持续阻塞。

本列表中明确写成已实现的项目已经存在于当前检查点，不应重复重写；明确写成“尚未”或“未覆盖”的项目仍须关闭。其中 profile-mask 日志和软件 Renderer 主动拒绝属于 P0-1，全路径 FBO 预算与非 Trisets 路径的 GL 状态恢复属于 P0-5。不得把相邻完成项外推到这些缺口。已完成部分也仍只是源码/API 级兼容基线，尚未在目标 Intel/AMD 驱动上完成端到端验证。当前检查点中常用的 high/low volume、lighting、prune 以及 Mesh copy/dilate/blur 延迟 shader 已不再在失败时 `exit(0)`：失败会清理半成品并返回零 program/不可用状态，已审计调用方会停止对应功能。剩余可执行 `exit()` 属于 launcher/批处理正常结束，或旧 PLY/netCDF 库的边缘输入路径。内容哈希仍不是人工可读的语义名称，目标驱动上的全部调用方也尚未实测。

### 4.4 纹理预算和 slab 边界

当前工作树已实现：

- 主程序和 Mesh 现在把配置夹在 128–512 MiB，当前代码默认 512 MiB。这个数是硬上限，不是所有核显笔记本的推荐默认值；8/16 GiB 设备应按 P0-3 降到 128/256 MiB 起测。
- Paint 原先固定约 1000 MiB 的 3D 预览预算已改为 128–512 MiB，当前默认 512 MiB；`.drishti.xml` 中旧的 1000 MiB 配置会被夹取，不会继续直接申请约 1 GiB 常驻纹理。
- Paint 按体数据与 16 位 label texture 的合计字节选择 LOD，使用检查过的 `qint64`/`size_t` 乘法和 `new (std::nothrow)`。数据与标签的 CPU 暂存缓冲顺序创建和释放，更新前先删除旧 3D texture，避免新旧纹理同时常驻。
- Paint 的两次 `glTexImage3D`、增量 `glTexSubImage3D`、纹理句柄和 lookup texture 创建均检查 OpenGL 错误。失败时释放半成品并禁用 3D 预览，不丢弃 CPU/2D 标注数据，也不继续访问空 texture。
- 二维纹理工作上限取 `GL_MAX_TEXTURE_SIZE` 与 rectangle texture limit 的较小值，用户缩放配置只能降低、不能突破硬件上限。
- `getSubsamplingLevel()` 对零/负可用预算、尺寸溢出和 LOD 不终止做了有界处理，并同时约束显存预算、二维宽高和 `GL_MAX_ARRAY_TEXTURE_LAYERS`。
- drag volume 的二维打包预算计入空槽，并对乘法溢出及最小维度做保护。
- `getSlabs()` 不再强制至少两层；单层本身超预算、或多层数据无法容纳一层重叠时安全拒绝。最后一个 slab 的空间上界明确延伸到 `dataMax.z`。
- `slab.x` 现在是显式 `layerCount`，沿 `DrawHiresVolume -> Volume -> VolumeSingle/VolumeRGB` 传递；CPU 缓存按所有 slab 的实际最大层数分配，不再使用 `firstSlab + 1` 或由原始 Z 范围反推上传深度。
- RGB/RGBA 和多卷路径补齐 QuadVolume 第四通道、RGB/RGBA 数据所有权、LOD 输出索引、slab 起始偏移、通道尺寸/空指针和检查过的 `nothrow` 分配。
- 非零 `dataMin.z`、多 slab 累积和卷 Z 偏移下的 drag 层索引已改为相对坐标；局部卷切片不再错误地与全局裁剪 Z 比较。
- `loadTextureMemory()` 检查 CPU 分配、空 slab、OpenGL 尺寸、纹理句柄和两次 `glTexImage3D` 错误。失败时删除半成品 GL/CPU 资源、恢复 Viewer/UI 状态，并阻止后续 prune/light 访问空纹理。
- `saveForDrishtiPrayog()` 在上传失败、空纹理或多 slab 数据不完整时明确拒绝，不再访问 `m_dataTex[1]` 或写出未初始化尾部。
- `VolumeSingle::setMaxDimensions()`、单卷和多卷的初始切片缓冲已经使用受检乘法、`nothrow` 分配和失败传播；`saveSubsampledVolume()` 也会在临时缓冲、空切片或分配失败时中止并清理部分 LOD 文件。

纯 Z/slab 边界模型已经覆盖奇数深度、LOD 大于 1、多 slab、非零 Z 原点、数组层上限 1 和零预算；极端 XY 裁剪的夹取修复和组合模型见 P0-4。此前位于这些上层保护之前的 `VolumeFileManager`、`VolumeBase`、`VolumeRGBBase` 分配/I/O 风险，以及 RGB/RGBA 整卷降采样缓存和多卷双份 slab，现已按 4.7、4.8 修复。目标机仍必须实测 CPU 峰值、共享纹理和驱动 staging 的组合压力。

### 4.5 CPU 网格画笔和 VBO 布局

当前活动的主程序和 Mesh 网格画笔已使用共享 `CpuMeshPaint`，不再 dispatch compute shader。两套 `TrisetObject` 同时统一为 9-float 基础布局和 12-float tangent 布局；Assimp 缺少 normal/color 时补默认值，并在打包或 VBO 上传失败时回滚状态。

这使 OpenGL 4.2 核显不再因网格画笔要求 4.3 compute，但 CPU 画笔每次事件仍遍历全部顶点并映射 VBO，复杂度为 `O(total vertices)`。小中型网格预计可用，大网格连续笔触是否流畅必须在目标设备实测。

最终静态复核结论如下：

- `meshvertexbuffer.h` 的 packed-count 乘法已经在运算前把 `vertexCount` 提升为 `qint64`，极大网格会在 `QVector<int>` 容量边界前安全拒绝；该 header 已用 MSVC 2019/Qt 5.15 独立编译通过。
- 两套 `TrisetObject::updateVertexColorBuffer()` 已用 `ScopedArrayBufferBinding` 保存并恢复原 `GL_ARRAY_BUFFER` 绑定，不应重复实现。剩余问题是映射/unmap/重建失败时，CPU 颜色虽会回滚，GPU VBO 内容仍可能已经未定义或与 CPU 状态不一致；失败后应重建完整 VBO，或把对象锁存为不可绘制，不能只回滚 `m_vcolor`。
- `cpumeshpaint.h:105` 的 ridged 分支把 value noise 映射到 `[-1,1]`，而原 `bin/assets/shaders/paintShader.glsl:266` 使用 3D simplex `snoise()`。因此 `roughnessType == 0` 目前不是逐语义等价；需要 golden corpus 对比旧 GPU 结果，或明确接受并记录图案变化。现有 helper smoke 只证明可编译运行，不能证明画笔效果一致。

### 4.6 Paint GraphCut 内存安全

当前工作树已经把 GraphCut 从“内存不足可终止整个 Paint”改为可恢复失败：

- `Block`/`DBlock` 和 Graph 节点、边、双向 arc 的数量及分配字节数均做溢出检查；allocator 不再调用 `exit(1)`。
- GraphCut 使用实际四方向边数预分配，sigma、梯度和直方图使用 RAII；异常转换为 `bool + QString` 错误。
- 同时执行两层门控：固定保守上限 512 MiB，以及操作当下的可用物理内存/Windows Commit 预算。512² ROI 估算约 82 MiB，1024² 约 329 MiB；4096² 会在大分配前拒绝并提示缩小 2D box。
- 调用方只构建 ROI 暂存，求解结果在全部分配和计算成功后才事务式写入 `m_tags`；失败时当前标注保持不变，递归任务停止并恢复 UI。

该修改已经通过独立编译、安全 smoke 和 200 组确定性随机等价测试。GraphCut 目前仍在 UI 线程同步执行且没有算法内取消点，接近预算上限或使用很大的 `boxSize` 时仍可能长时间显示未响应；内存安全不等于交互流畅。

### 4.6a Paint 原生三维算法准入与回滚

文件：

```text
tools/paint/volumeoperations.cpp/.h
tools/paint/tests/volumeoperations_memory_admission_smoke.cpp/.pro
```

8 个高风险入口已经在首个显式整卷工作缓冲前执行受检 64 位、实时物理内存和 Windows Commit 准入：Remove Smaller Components、Remove Largest Components、Connected Components、Distance Transform、Local Thickness、Watershed、Watershed Plus 和 Watershed Priority Queue。

- Connected Components 使用 `192 B/voxel + 64 MiB`，并额外计入 `depth*512 B` 的 `getVisibleRegion()` 任务列表，覆盖高度碎片化 mask 下的 `QMap/QMultiMap/QList` 节点及极瘦 ROI；结果窗口最多显示 1000 行，避免组件数巨大时先被 `QString`/UI 拖死。
- Distance Transform 使用 `32 B/voxel + 64 MiB`，Local Thickness 使用 `64 B/voxel + 64 MiB`，Watershed 三种实现使用 `160 B/voxel + 128 MiB`。
- 所有距离变换类算法额外计入 `depth*width*512 B` 的扫描线任务列表和 `maxDimension*1024 B` worker scratch，避免窄长 ROI 被固定 bytes/voxel 模型低估。
- 会修改 mask 的入口先保存选区快照，只有完整成功才 `commit()`；`bad_alloc`、`std::exception` 或未知异常都会由 RAII 恢复原选区，并保持输出 bounds 无效，调用方不得保存半成品。
- Connected Components 和三个 Watershed 标签入口在第一次写 mask 前验证 `startLabel + label span <= 65535`；高碎片 ROI 超出 16 位标签容量时明确拒绝，不再发生标签归零、碰撞后仍提交。三个 Watershed 使用固定 65536 位跟踪器统计唯一标签，扫描复杂度为 `O(voxels)`、固定额外占用约 8 KiB，不再逐体素调用 `QList::contains()` 形成最坏二次复杂度。
- `MaskRegionTransaction` 持有原始 mask 指针期间，所有显式事件泵均使用 `QEventLoop::ExcludeUserInputEvents`；用户不能在回滚快照存活时通过菜单或鼠标重入切卷、关闭卷或开始另一项编辑。
- 三个 Watershed 的局部/全局索引已收口：Priority Queue 的所有 mask 访问补上 `hs/ws/ds` ROI 偏移；另外两个版本在 `findSteepestDescent()` 无有效邻居时先检查负索引，不再访问 `dt[-1]` 或 `m_maskDataUS[-1]`。目标移植不能只复制内存准入而遗漏这些边界修复。
- profile smoke 已覆盖公式、容量溢出、16 位标签边界、重复/非法标签、物理/Commit 拒绝和形状相关开销；它不是操作级分配失败注入测试，目标机仍须证明“准入调用早于分配”和“失败后 mask 字节完全不变”。LiveWire、其他形态学和未列出的旧算法仍未获得同等级证明。

### 4.7 底层卷文件 I/O 和候选对象内部事务（应用层未完成）

文件：

```text
drishti/volumefilemanager.cpp/.h
drishti/volumebase.cpp/.h
drishti/volumesingle.cpp/.h
drishti/mainwindow.cpp/.h
```

当前工作树已经完成原 P0-2，而不是仍待编码：

- `VolumeFileManager` 用受检 `qint64/size_t` 计算轴向、横向和纵向平面容量，按实际请求动态扩容，不再按 `max(width,height,depth)^2` 预分配。
- 缓冲使用 `new (std::nothrow)`；索引、几何、open、seek、read/write、短读/短写、flush 和最终文件尺寸都必须成功。短读返回 `nullptr`，写 API 返回 `bool`，具体原因由 `lastError()` 向上传递。
- 新建磁盘卷失败时清理半成品；候选对象内部的内存卷只在完整构造成功后提交，该 candidate 不会暴露半初始化数据。此保证不覆盖 `MainWindow` 的活动工作集切换。
- `VolumeBase` 的直方图、lowres volume 和 lowres texture 三阶段改为 `bool` 传播。加载任一步失败都会回滚该 candidate 内部结果，并移除一份不必要的完整切片副本；同时修复 uchar 梯度末端的越界读取。
- `VolumeSingle` 中本轮覆盖的 18 个 `VolumeFileManager` 调用全部检查结果。LOD、梯度、重切片和导出失败会删除半成品并恢复 UI；bitmask 失败会阻止后续面积、掩码和连通域处理。
- **应用层加载仍是破坏性的，原 P0-2 只完成了底层对象内部的失败传播。**`MainWindow::preLoadVolume()` 在候选验证前就清空 RawVolume、lighting、主要几何集合和关键帧；单通道、RGB/RGBA、2/3/4 卷入口随后才调用加载。`Volume::loadVolume()` 各重载也会先删除当前活动 volume，失败恢复只是 `clearVolumes()` 并切到 `DummyVolume`，不会恢复旧工作集。DummyVolume 分配失败同样会先清关键帧和旧 volume；项目加载还会提前修改项目/UI 状态，并且没有统一成功结果阻止后续资源继续加载。
- 因此“失败后可再次重试”不等于事务安全。后续智能体必须按 P0-6 实现两阶段候选加载：volume、lowres 和项目关联资源全部独立准备并验证成功后，才一次性提交活动指针与 UI；任一失败必须让旧体积、几何、TF、关键帧、相机、当前项目/标题和可渲染性逐项保持不变。

底层 VFM/I/O 的显式错误传播和已审计调用方检查已经扩展到 Paint 的独立卷文件链路以及部分 ITK、Mesh 和 Raw 导出路径，具体见 4.9；这只说明这些底层/定向路径不再静默吞掉已覆盖的失败。它不代表 `MainWindow` 的活动工作集和项目加载已经事务化，后者仍由 P0-6 负责。也不能把这些定向修复外推为全部菜单都具备同等级故障注入证据；旧 PLY、netCDF、视频和未审计插件继续保留在“全部菜单/罕见错误输入”风险口径中。

### 4.8 RGB/RGBA、多卷固定 slab 和高分辨率失败传播

文件：

```text
drishti/volumergbbase.cpp/.h
drishti/volumergb.cpp/.h
drishti/volume.cpp/.h
drishti/drawhiresvolume.cpp/.h
```

当前工作树已经完成原 P0-3 的核心编码：

- `VolumeRGBBase` 安全解析 `<gridsize>`，验证 RGB(A) 全部通道和每次切片读取；尺寸/容量计算、分配和工作缓冲均受检，并以 RAII/事务方式生成结果。
- RGB(A) 低分辨率生成使用连续有界 ring buffer；薄 Z、窄轴和各向异性数据按截断窗口平均，不再成功返回全零卷。加载期间禁止 Qt 事件重入，避免其他槽函数看到半初始化状态。
- `Volume::valid()` 认可 RGB/RGBA；`forceCreateLowresVolume()` 返回 `bool`。单卷和 2–4 卷高分辨率上传使用固定容量 slab，多卷只保留最终交错 slab 加一个通道 scratch，不再同时常驻 N 份通道 slab。
- `VolumeRGB` 按请求层逐层读取/降采样到可复用 slab，LOD > 1 只使用固定单层 scratch，不再先构造完整降采样整卷；drag/slab 缓冲同样固定容量。
- RGB/RGBA 与 opacity 导出补齐受检分配、I/O 失败和半成品清理；同时修复 RGBA alpha 查表索引和 opacity 未初始化区域。
- `DrawHiresVolume` 的重采样、路径、clip、表面和图像转体积路径检查所有主 `VolumeFileManager` create/get/set 结果；失败会停止流程、删除部分输出并恢复 GL/UI。

旧的显式 `VolumeRGB::getSubvolume()` 整卷接口仍保留，供历史调用方兼容；正常高分辨率上传已经不再调用它。目标智能体不得把这个旧接口的存在误判为当前常用上传路径仍在整卷缓存。

### 4.9 Paint 独立 I/O、ITK 与导出插件失败契约

文件：

```text
tools/paint/filehandler.cpp/.h
tools/paint/volumefilemanager.cpp/.h
tools/paint/slabsavetransaction.cpp/.h
tools/paint/volume.cpp/.h
tools/paint/volumemask.cpp/.h
tools/paint/drishtipaint.cpp/.h
tools/paint/tests/{undo,vfm_lifecycle,slabsavetransaction}_smoke.cpp/.pro
drishti/plugins/itk/*/{filter.cpp,label.cpp,skeletonizer.cpp}
drishti/plugins/mesh/meshgenerator.cpp
drishti/plugins/meshpaint/meshgenerator.cpp
drishti/rawvolume.cpp
```

当前工作树已经完成以下编码：

- Paint 不再沿用一套未检查的旧 VFM：尺寸、容量、offset 和切片索引使用受检 `qint64/size_t`；缓冲按实际平面动态扩容；open/seek/read/write/flush、短读短写、最终大小和 Blosc block 都有错误返回。
- Paint 的压缩 mask、普通文件 copy 和相关 sidecar 使用禁用 direct fallback 的 `QSaveFile` 或等价临时提交；主 mask 的损坏签名、网格不匹配、压缩块尺寸异常和解压长度不符会在写入用户状态前失败。`Volume`、`VolumeMask` 和 `DrishtiPaint` 已把主要 load/save/tag/extract 调用改为 `bool + lastError()` 传播。
- Paint 只明确接受 unsigned 8/16-bit 体和 8/16-bit mask；直方图、三个方向切片、形态学工作缓冲和从另一体数据提取流程增加了空切片、受检容量和 `nothrow`/RAII 保护。PVL header 使用事务写入，数据切片失败时清理本轮部分输出。
- 六个 ITK 插件只接受 `_UChar`/`_UShort`，所有整卷/平面容量和 trim/prune/blend 索引提升为受检 64 位；主输出和 edge/smoothing/VED sidecar 使用 `QSaveFile`。16 位 edge/smoothing/VED 输入先正确归一化到 8 位输出，VED 不再把 `Image<double>` 缓冲错误解释为 `float*`，拷贝到 ITK image 后立即释放额外的 double staging。
- `rawvolume.cpp` 的 masked RAW 导出和 Mesh/MeshPaint 的切片读取会检查空指针、写入结果和容量；Mesh/MeshPaint 失败清理只移除本轮新建文件，不删除运行前已经存在的同名输出。

这些修改提升的是故障可恢复性和典型数据的交互性，不是无限规模保证：

- 压缩 mask 自动保存已改为 350 ms debounce、generation 合并和单 worker。GUI 线程先按 8 MiB 分段顺序写同目录不可变原始 snapshot，worker 再以 Blosc level 3、8 MiB block 压缩并通过 `QSaveFile` 提交；`saveBlock()`、`checkFileSave()` 和非强制 `saveIntermediateResults(false)` 只请求保存，显式保存、切卷和退出通过 `flushPendingChanges()` 等待最新代。后台失败保留 dirty 状态，下一次显式保存可以重试。
- 这已经消除每次编辑直接等待 `BlockingQueuedConnection` 和高压缩级别的问题，但不是完整 dirty-chunk 后端：整卷原始 snapshot 仍由 GUI 线程同步写盘，停顿与 mask 大小和磁盘速度成正比。当前 `blosc_compress_ctx` 路径均使用 level 3；进程崩溃或删除失败仍可能留下 `.drishti-mask-snapshot-*.tmp`。
- `stopFileHandlerThread(bool)`、`reset()` 和 `setMemMapped()` 均返回 `bool`。`VolumeMask`、`Volume` 及常用 load/reset/offload 调用方会传播失败；保存失败时不释放 dirty buffer，也不改变旧映射状态。后台保存已有 30 秒无进展 watchdog，线程关闭也有 30 秒超时；析构仍只能 best effort，目标机需故障注入验证超时后的 dirty 状态。
- 非压缩多 slab 保存已采用全体 staging、`STAGING`/`PREPARED`/`COMMITTED` journal、backup 切换和启动恢复；journal 在首个 stage 写入前就落盘。`STAGING` 清理本轮不完整 staging，`PREPARED` 恢复旧代，`COMMITTED` 保留新代，避免留下混合代际卷。
- Undo 已接通实际用户边界：`paint3DStart()` 先 flush 当前 mask，再通过 `createUndo()` 原子生成一层 Undo；任一步失败都会设置 `m_paintUndoReady=false`，整次 3D 笔触不修改 mask。恢复时先把 `.tmp` 完整校验并解压到 staging，随后原子替换正式 mask并一次性更新活动 buffer；损坏或截断 Undo 不改内存和正式文件。代价是大 mask 在笔触开始时仍可能因 flush 和文件 copy 出现停顿。
- 六个 ITK 插件已在构造整卷 ITK image 前使用与 Paint 相同的实时物理内存/Windows Commit 准入：Binary Thinning `32 B/voxel + 64 MiB`、Connected Components `48 + 64 MiB`、Distance Map `48 + 64 MiB`、Smoothing `96 + 128 MiB`、Edge Preserving `128 + 128 MiB`、VED `256 + 256 MiB`。无法可靠表示或预算不足时抛出可报告错误，不试探性分配。
- 本机没有正式 ITK 4.x/OpenVDB 头文件和库。Paint 调用层只在临时 OpenVDB 前向声明桩下完成对象级语法检查；六个 ITK 文件只能做静态复核，不能声称插件已经正式编译或运行。VED 当前也未列入 `drishti/plugins/itk/itk.pro` 的默认 SUBDIRS。

### 4.10 Paint 实时内存和 Commit 准入

文件：

```text
tools/paint/getmemorysize.cpp/.h
tools/paint/volume.cpp
```

当前工作树已经替换原先只读取总 RAM、再弹窗询问是否整卷加载的粗略策略：

- Windows 使用 `GlobalMemoryStatusEx` 读取实时可用物理内存、Commit Limit、已提交量和 Commit 余量；非 Windows 保留可编译的 `sysconf/sysctl` 路径，Commit 状态未知时保守使用物理内存门槛。
- 容量运算使用受检 64 位整数。保守峰值模型为 `raw + mask + max(raw, mask) + 512 MiB`。
- 从可用预算中预留 `max(2 GiB, RAM * 20%)` 给系统，并为核显共享内存预留 `clamp(RAM * 6.25%, 512 MiB, 2 GiB)`。
- 驻留原卷与 mask 达到 `min(RAM * 12.5%, 2 GiB)` 时拒绝整卷加载；物理内存或 Windows Commit 任一预算不足也不尝试危险分配。显式 `loadMemFile()` 被拒绝时保持 offloaded，普通 `setFile()` 则返回清晰错误。
- `setFile()` 和显式 `loadMemFile()` 共用同一准入，且不再显示默认选择“继续加载”的阻塞式内存对话框；日志会记录模式、原因、峰值和两类预算。

这项改动的目标是防止 Paint 把 Windows 推入持续换页甚至桌面无响应，不是把现有 Paint 自动变成完整的 brick/ROI 编辑器。多个 Viewer 和算法仍要求整卷裸指针；超预算大卷目前会被拒绝，而不是以功能受限模式正常打开。目标智能体必须分别验收“系统不被拖死”“拒绝信息明确”和“典型数据完整可用”。

### 4.11 Checkpoint 流式事务和损坏文件恢复

文件：

```text
tools/paint/checkpointhandler.cpp/.h
```

当前工作树已经完成此前的 Checkpoint 发布阻断项：

- 按真实 8/16-bit `voxelType` 计算字节数；不支持的类型和所有尺寸、计数、offset、size、block count、压缩长度均在分配或访问前拒绝。
- 保持旧磁盘布局兼容。新记录使用 8 MiB Blosc block，读取端仍接受旧版最大 100 MiB block。
- 保存和删除使用 `QSaveFile` 流式复制并原子提交，不再构建完整记录 `QBuffer`；已有 84 字节 FAT 描述字段原样保留。
- 加载先完整校验归档并解压到 staging mask，核对每块 Blosc 输出长度，全部成功后才一次性替换活动 mask。损坏、截断、OOM 或非法索引均不得修改目标 buffer。
- 提供基于零起始 record index 的非交互 load/delete API 和线程安全 `lastError()`；交互包装仍负责选择记录和显示错误。重复描述通过记录编号区分。

Checkpoint 的峰值和损坏输入风险已经显著降低。Undo 也已独立接通 3D 画笔开始、失败门控和事务恢复；所有当前 Blosc 写路径均使用 level 3。当前仍可能出现的保存停顿主要来自 GUI 线程顺序写整卷原始 snapshot 和大 mask 的完整文件 copy。

### 4.12 Windows qmake 可移植配置与剩余依赖

此前的 Windows 路径和 shadow-build 阻塞已在当前工作树收口。`drishti.pri` 保留作者原路径作为未传参时的兼容 fallback，但目标机可通过 qmake 参数或同名环境变量覆盖安装根目录；生成的 Makefile 已验证实际使用覆盖值，而不是继续使用旧路径。

主要参数：

| 参数 | 用途 |
|---|---|
| `DRISHTI_BIN_DIR` | 四个 EXE 和所有插件的统一输出根目录；默认是当前源码树 `bin` |
| `DRISHTI_VCPKG_ROOT` / `DRISHTI_VCPKG_TRIPLET` | vcpkg 根和 triplet，默认 triplet 为 `x64-windows` |
| `DRISHTI_QGLVIEWER_ROOT` | QGLViewer include/lib 根；也可分别传 `QGLVIEWER_INCLUDE_PATH`、`QGLVIEWER_LIBRARY_PATH` |
| `DRISHTI_ITK_VERSION` / `DRISHTI_ITK_SOURCE_ROOT` / `DRISHTI_ITK_BUILD_ROOT` | ITK 库后缀、源码树和构建树；88 个 Windows ITK 库名均跟随版本 |
| `DRISHTI_ENABLE_VED=1` | 把 VED 加入默认 ITK 子项目；不传时维持原默认集合 |
| `DRISHTI_PYTHON_ROOT` / `DRISHTI_PYTHON_LIB` | Import 的 Python include、lib 目录和库名 |
| `DRISHTI_COMMON_LIB_DIR` / `DRISHTI_PLUGIN_COMMON_LIB_DIR` | 仓库 VDB 静态库和 render-plugin common 静态库目录 |
| `DRISHTI_GLMEDIA_LIBRARY_PATH` | legacy MeshPaint/MeshSimplify 的 glmedia 库目录 |
| `DRISHTI_*_LIB` / `DRISHTI_NETCDF_LIBS` / `DRISHTI_FFMPEG_LIBS` | QGLViewer、GLEW、Assimp、Imath、OpenVDB、VDB、Gmsh、Blosc、netCDF、FFmpeg 等实际库名 |
| `DRISHTI_EXTRA_INCLUDEPATH` / `DRISHTI_EXTRA_LIBDIR` / `DRISHTI_EXTRA_LIBS` | 目标机额外依赖位置和库 |

应用、Import 插件、render plugins、ITK 和 MOP 的 Windows `DESTDIR` 均从 `DRISHTI_BIN_DIR` 派生。Paint、Mesh plugin、ITK 和 MOP 的 `PRE_TARGETDEPS` 使用源码树绝对路径，不再错误解析到 shadow-build 目录。仓库 `vdb.lib` 和 plugin `common.lib` 的实际产物名会跟随对应库名参数。根项目和 render plugins 都使用 ordered 子项目，保证 VDB、plugin common 等前置库先构建。VED 已改为复用当前 `drishti.pri`/`plugin.itk`，不再包含固定 ITK 4.3、旧 Qt 或 `D:` 路径。

完整配置示例：

```powershell
& 'C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe' `
  'C:\src\drishti\drishti.pro' `
  'DRISHTI_BIN_DIR=C:\DrishtiBuild\deploy' `
  'DRISHTI_VCPKG_ROOT=C:\Apps\vcpkg' `
  'DRISHTI_VCPKG_TRIPLET=x64-windows' `
  'DRISHTI_QGLVIEWER_ROOT=C:\Deps\libQGLViewer-2.9.1' `
  'DRISHTI_ITK_VERSION=5.0' `
  'DRISHTI_ITK_SOURCE_ROOT=C:\Deps\InsightToolkit-5.0.1' `
  'DRISHTI_ITK_BUILD_ROOT=C:\Deps\ITK-build' `
  'DRISHTI_PYTHON_ROOT=C:\Python311' `
  'DRISHTI_PYTHON_LIB=python311.lib' `
  'DRISHTI_ASSIMP_LIB=assimp-vc142-mt.lib' `
  'DRISHTI_ENABLE_VED=1'
nmake /NOLOGO
```

这只解决路径、输出目录、依赖名和构建顺序，不会自动安装第三方库，也不会修复 ABI 不匹配。完整 Import 仍会链接 Python、Imath、OpenVDB、仓库 VDB 和 Gmsh；三套 OpenGL 程序仍需要匹配的 QGLViewer、GLEW、Assimp、netCDF、FFmpeg 等。可选且未进入默认 `plugins.pro` 的旧 HDF4 工程仍保留历史 `C:\drishtilib` 路径。目标智能体必须形成同一 Qt/MSVC/CRT ABI 的依赖清单，不得通过混拷 DLL 解决链接问题。

## 5. 本机验证证据和限制

验证工具链：MSVC 19.29、Qt 5.15.2、libtiff 4.5.1。

已通过：

- TIFF 插件全新 shadow build，显式依赖路径确实排在仓库默认 vcpkg 前面。
- 中文路径、3 页 uint16 BigTIFF：尺寸、类型、0–59 范围、65536-bin 直方图、随机像素和转置切片全部正确。
- 第一页完整、下一 IFD 指向文件末尾的截断 BigTIFF：Windows QPA 下插件返回 `false`，没有进程崩溃。
- uint8、int8、int16、int32、float，以及 RGB、tiled、MINISWHITE、错误方向、uint32、混合页面和损坏输入的正/负向测试。
- 128 x 1024² uint16 堆栈取消测试。
- Image Stack 与 TIFF 插件的最终源码均以 MSVC 2019 Release 配置重新编译并链接成功；`raw2pvl.obj`、`volumedata.obj` 和 `remapwidget.obj` 也具有当前源码对象证据。插件使用本机 Anaconda Qt，只证明构建闭合，最终已通过 `distclean` 清除，不能部署到目标机。完整 Import EXE 仍缺正式 OpenVDB/Gmsh/Python 等同 ABI 依赖。
- 审计过程曾完成两份 GLEW 初始化源文件、QGLFormat API smoke 和 `meshvertexbuffer` helper 的临时编译/运行；这些临时对象和 EXE 当前未保留在 `.lab-agent/build`，只能作为历史过程记录，不能作为当前工作树的可复核产品构建证据。
- 最终 qmake 矩阵 9/9 成功：Drishti、Import、Paint、Mesh、仓库 VDB、ITK Smoothing、ITK VED、根 `drishti.pro`，以及 `itk.pro + DRISHTI_ENABLE_VED=1`。占位覆盖值实际进入 INCPATH/LIBS；Paint VDB、ITK/plugin common 和 ordered 子项目路径均指向预期位置。
- 根项目随后执行 `nmake qmake_all`，递归生成 34 个 Makefile，退出码 0；覆盖全部默认 Import 插件、六个 ITK/VED、MOP、Paint 和 Mesh。所有应用/插件目标都落在传入的 `DRISHTI_BIN_DIR`，仅有 Image Stack 与 Import 同名 `volumefilemanager.cpp` 的既有 qmake/no-batch 警告。
- Windows 输出目录另做 12 项 default/custom 探针：主程序、Paint、Import plugin、render plugin、ITK plugin 和 MOP 在默认模式都指向当前源码树 `bin`，传入 `DRISHTI_BIN_DIR=C:\portable\drishti-bin` 时都指向该目录。
- Paint `viewer.h` 的 MOC 代码生成退出码为 0。
- slab 边界模型的 6 个定向案例和 10,000 个随机案例通过；另有 10,000 个随机案例复核显式层数、末尾范围、多 slab 重叠和 drag 层不越界。
- Paint 的 10,000 个随机 LOD/128–512 MiB 预算/`GL_MAX_3D_TEXTURE_SIZE` 模型通过，没有发现不终止、零尺寸或预算超限案例。
- GraphCut 安全 smoke 通过，覆盖小图求解、容量溢出、4096² 预算预拒绝和失败时输出不变；512²/1024² 的保守估算分别为 82/329 MiB。
- GraphCut 与基线 `b53bd979` 的 200 组确定性随机分割结果哈希一致：`7d6b8cf0db480f4d`。
- qmake 生成的 Release `graph.obj` 和 `graphcut.obj` 单独编译通过；GraphCut 活动路径已无 `exit()`。
- `VolumeFileManager` safety smoke 通过：覆盖多 slab、三个方向的精确读写、短读、越界、尺寸溢出、失败清理和事务状态。
- `VolumeRGBBase` 等价测试通过：200 组正常 RGB/RGBA 与旧算法逐字节一致；100 组薄 Z/各向异性输入与截断窗口参考逐字节一致。
- 容量模型显示：4096² x 10 uint16 的 `VolumeBase` 路径约 32.62 MiB；RGB/RGBA 直方图阶段分别由约 96.75/129 MiB 降为 48.75/65 MiB；LOD 4 的 RGB/RGBA 阶段分别由约 100/128 MiB 降为 81/108 MiB。这些是缓冲容量模型，不是目标机进程 RSS 或共享显存实测。
- 常用 high/low volume、lighting、prune、Mesh copy/dilate/blur shader 路径已无可执行 `exit()`；Mesh 调用方会检查零 program。
- 审计过程曾用 VS2019 + Qt 5.15 和临时第三方声明桩对多份 Drishti、Paint、Mesh TU 做对象级语法检查；相关产品对象当前未保留在 `.lab-agent/build`。声明桩检查本来也只证明当时的接口/C++ 语法，不证明正式依赖 ABI、完整链接或当前源码状态，因此不得把这些历史检查列为目标机可跳过的构建步骤。
- Paint 内存准入的 MSVC 对象编译和 Windows 策略 smoke 通过，覆盖小卷、大卷、物理不足、Commit 不足、状态未知、溢出和实时 API 字段关系；WSL `g++ -std=c++17 -Wall -Wextra -Werror` 也通过非 Windows 路径。
- Checkpoint 当前源码的 MSVC 对象编译和独立链接 smoke 通过，覆盖 8-bit round trip、16-bit 多 block round trip、8 MiB 新 block、模拟旧 100 MiB block、追加、按索引删除、FAT count/offset/size、block count、压缩长度、损坏 payload 和截断归档；所有失败加载均保持目标 buffer 不变，被拒绝的保存保持原归档逐字节不变。
- `drawhiresvolume.obj`/`mainwindow.obj` 等对象级检查使用了临时 GLEW、QGLViewer 和 FFmpeg 前向声明桩，只证明本轮接口与 C++ 语法，不是正式第三方依赖下的完整链接或运行验证。
- 2026-08-10 时十一个定向 smoke 均曾用 MSVC 2019 Release 构建并运行通过：Algorithm memory admission、Framebuffer budget、GraphCut memory admission、Image Stack pixel contract、Import memory admission、ITK memory admission、Plugin error bridge、Volume operations memory admission、Slab save transaction、Undo 和 VFM lifecycle。**其中 Framebuffer budget 只测试 `FramebufferBudget::evaluate()` 的算术，且现存 EXE/OBJ 早于 2026-08-12 的三处调用方与状态守卫修改；它不是当前 FBO 实现、GL 状态恢复或实机分配证据，必须从当前源码重建并另加真实上下文哨兵测试。**Volume-operations smoke 包含极瘦 ROI 可见性任务项、16 位标签上限和 Watershed 唯一标签边界。
- VDB 自定义产物名和默认 netCDF4 插件库名覆盖也分别生成 Makefile 成功；根 qmake 已确认 `DRISHTI_BIN_DIR`、VED 开关和全部 ordered 依赖传播到子项目命令。
- `git diff --check`。

**2026-08-10 12:25 历史构建产物检查点（已被下方最终复核取代）**

本小节保留早期证据链，不能再作为当前状态引用。尤其是其中 `raw2pvl.obj` 未编译、插件 DLL 陈旧和仅 9 个 smoke EXE 的描述，已由 2026-08-10 最终复核更新。

当前共有 62 个修改或新增的正式产品 `.cpp`。`.lab-agent/build` 只保留 38 个 OBJ 和 9 个 smoke EXE；按“对象时间不得早于源码及已知关键头文件、且同名对象必须由对应 Makefile 依赖关系定位”的规则，其中只有下列 10 个产品 TU 具备当前源码对象证据：

```text
tools/import/importmemoryadmission.cpp
tools/import/plugins/tiff/tiffplugin.cpp
tools/import/plugins/imagestack/imagestackplugin.cpp
tools/import/volumedata.cpp
tools/paint/getmemorysize.cpp
tools/paint/graphcut/graph.cpp
tools/paint/graphcut/graphcut.cpp
tools/paint/filehandler.cpp
tools/paint/slabsavetransaction.cpp
tools/paint/volumefilemanager.cpp
```

其中插件对象是最新的，但现存两个插件 DLL 早于对象，故链接证据陈旧。九个 smoke EXE 只证明各自 Makefile 的 `OBJECTS`，不能外推为完整 Import、Drishti、Paint、Mesh 或 ITK 插件已经链接。

其余修改产品 TU 当前没有可复核的最新产品对象，主要包括：

- `drishti/` 主程序全部修改 TU。
- `tools/mesh/`、Mesh plugin 和 MeshPaint plugin 的修改 TU。
- Paint 的 `volumeoperations.cpp`、`viewer.cpp`、`checkpointhandler.cpp`、`drishtipaint.cpp`、`imagewidget.cpp`、`main.cpp`、`shaderfactory.cpp`、`volume.cpp` 和 `volumemask.cpp`。
- Import 的 `raw2pvl.cpp`；其产品对象被缺失 OpenVDB headers 阻断。
- 六个 ITK/VED 产品 TU；各目录中的 `getmemorysize.obj` 不是插件算法对象，且部分对象早于最新共享准入头文件。

`volumeoperations_memory_admission_smoke.obj` 是 profile 测试 TU，不是 `volumeoperations.obj`。同理，qmake/Makefile 生成成功只证明构建图和参数解析，不是 C++ 编译或产品链接成功。目标机补证优先级为：`volumeoperations.cpp`、`raw2pvl.cpp`、Paint `viewer.cpp`、Drishti/Mesh `trisets.cpp`、Drishti/Mesh `shaderfactory.cpp`、六个 ITK/VED 产品 TU，然后是 Paint 的 Checkpoint/Volume/Mask/应用生命周期链。

产物新鲜度必须逐层判断：OBJ 不得早于其源码和依赖头文件；EXE/DLL 不得早于任何链接输入对象。只看同名文件、qmake 成功或 Makefile 时间都不足以判定当前源码已构建。

4096² uint16 TIFF 的内存实测：

| 深度 | Peak Working Set | Peak Private |
|---:|---:|---:|
| 10 | 54.4 MiB | 37.0 MiB |
| 40 | 54.2 MiB | 37.0 MiB |

`rawValue()` 连续 100 次后的峰值为 55.6 MiB / 37.1 MiB。峰值不再按堆栈深度或鼠标事件线性增长。

最后一次 TIFF 验证产物：

```text
C:\saveproject\LBJ-workspace\_external\drishti-release\
  tiff-priority-20260809-191812\bin\importplugins\tiffplugin.dll
SHA-256: 4A29C10343E1E74EC31BB18F6221F9F6655D6923D99079FA27FDCA4E15FAAF9C
```

这个 DLL 只用于证明源码能编译和测试。它依赖 Anaconda 的 `Qt5*_conda.dll`，不得复制到正式 Drishti 目录。

本机没有目标 i7-13700H 核显和原始问题图片。qmake、输出目录和上述关键对象虽然通过，完整 Release 仍缺正式匹配 ABI 的 GLEW、QGLViewer、FFmpeg、netCDF、Assimp、OpenVDB、Gmsh 和 ITK 等依赖；`common/lib/vdb.lib` 的错误路径已修复，但本机没有用正式 OpenVDB 依赖生成该库。因此，不能声称三套应用及可选插件已经完整链接，也不能声称 OpenGL 改动已在目标 Intel 驱动上通过端到端验证。

验证边界必须明确：

- TIFF 的功能和内存数字来自合成数据，不是用户现场原图。
- Image Stack 有独立 Release 构建和 codec 前内存准入证据，但仍没有与 TIFF 同等级的合成数据、取消和内存曲线矩阵。
- CPU mesh paint/VBO smoke test 只证明基础 helper 可编译运行，不是画笔行为一致性或大网格性能测试；根目录的 `rgbbase_equivalence.obj` 是对象级测试产物，不是部署文件。
- Shader/FBO 只完成源码审计和可用 API 的编译检查，没有在真实 Intel/AMD 驱动上执行全部启动及延迟创建路径。
- 现存 Framebuffer budget smoke 仅覆盖容量公式，时间也早于当前 Trisets/Paint FBO 调用方；不能用于证明 512 MiB 覆盖完整、candidate 替换峰值安全或 GL 状态恢复正确。
- Paint 纹理修复通过了预算模型和 qmake，但仍没有正式依赖下的完整链接或实机纹理上传测试，不能把模型测试当成核显验证。
- GraphCut 核心对象已独立编译和测试；完整 Paint UI 没有在正式 GLEW/OpenVDB/Qt 依赖下链接运行。ASan 可执行文件也因本机缺少运行时 DLL，以 `0xC0000135` 退出，未能运行。因此不能声称 UI 运行层或 sanitizer 已通过。
- GraphCut、六个 ITK 插件和 8 个 Paint 原生三维入口都有内存准入；会修改 mask 的上述原生入口使用选区事务回滚。现有 profile smoke 不验证真实入口调用顺序或故障注入后的 mask 哈希，LiveWire 和其他未列出的旧算法也不能外推为已覆盖。
- Paint 独立 VFM、实时内存/Commit 准入、Checkpoint、异步 mask 压缩、多 slab 事务、生命周期失败传播、Undo、六个 ITK 输出路径、Mesh/MeshPaint 和 masked RAW 的已知空切片/短读调用已经补齐并完成相应定向检查；但 GUI 线程仍顺序写整卷原始 snapshot，snapshot 临时文件尚无完整清扫，旧 netCDF/视频和其他未审计插件也不能外推为全部通过。
- 尚无四个程序使用同一套正式 Qt/MSVC/第三方依赖的完整 Release 构建证据。
- 最新 `volumeoperations.cpp` 产品对象编译仍在进入源码语义检查前被缺失的 `GL/glew.h` 阻断；`raw2pvl.cpp` 已使用测试用 OpenVDB 声明桩生成当前 Release 对象，但这不能替代目标机正式依赖下的完整链接。

2026-08-10 最终静态复核：

- `git diff --check` 退出码 0；只有现存 LF/CRLF 转换提示，没有空白错误。
- `rg -n "glDispatchCompute" .` 只命中本交接文档的说明，活动源码零命中。
- `rg -n "\\bexit\\s*\\(" drishti tools` 的剩余可执行命中属于 launcher/正常程序退出、batch 完成、旧 netCDF 导入器和多份旧 PLY C 解析器；交互式 volume/shader/GraphCut 活动路径未再命中。PLY/netCDF 仍是全部菜单可靠性的已知缺口。
- 2026-08-13 provenance 更新：实施内容已提交为 `e731972d2c8663b001ce06384b9b4074854d0c00`（本地分支 `codex/cpu-igpu-worktree-checkpoint-20260813`）；tracked worktree 与 index 在本文修订前为干净状态。该检查点未 push，不能假定任何远端引用包含它。除本次 Markdown 修订外，`git status --short` 的剩余项目是未跟踪的审计/构建产物、依赖副本、日志、缓存或对象文件，不应作为源码传输载荷或正式构建证据。此前“尚未 commit”的表述只记录 2026-08-10 的历史状态。
- 审计过程记录曾运行 VFM safety、RGB(A) 等价、CPU mesh paint、Paint 准入、Checkpoint、异步保存、slab 事务、Undo 和生命周期测试；其中当前仍保留且可核对的只有上文九个 `.lab-agent/build` smoke。未保留 EXE/日志的项目只能视为历史过程记录，目标机必须重新生成并保存结果。
- 当前保留的最新产品对象只限于上文 10 个 TU。`checkpointhandler.cpp`、`volumemask.cpp`、`volume.cpp`、`drishtipaint.cpp` 及其 MOC 的历史对象当前未保留，不得称为当前源码已有对象证据。

**2026-08-10 最终 Import/构建复核（当前有效）**

- `tools/import/raw2pvl.cpp` 使用 MSVC 2019、Qt 5.15 Release 和测试用 OpenVDB/FFmpeg 语法 stub 编译成功；stub 只补足本机缺失的正式头文件，不能替代目标 ABI 链接验证。
- Image Stack 插件重新生成 qmake、编译并链接成功；Makefile 明确编译父级 `tools/import/volumefilemanager.cpp`。目录内旧的同名文件管理器已删除，不再产生 qmake source conflict。
- Import `volumefilemanager.obj`、`volumedata.obj`、`remapwidget.obj` 和最新 `raw2pvl.obj` 均有当前源码的 Release 对象证据。
- TIFF 与 Image Stack 插件均从最终源码完成 Release 链接；`VolumeData` 的可选插件错误桥未改变 `VolInterface/1.0` vtable，并由独立 smoke 覆盖旧插件、空错误和合成解码错误三种情况。
- 11 个现存 Release smoke 在 2026-08-10 构建时全部退出 0：Algorithm memory admission、Framebuffer budget、GraphCut memory admission、Image Stack pixel contract、Import memory admission、ITK memory admission、Plugin error bridge、Slab save transaction、Undo、VFM lifecycle、Volume operations memory admission。Framebuffer budget 项只保留为历史算术证据，不能算作 2026-08-12 当前 FBO 源码证据。
- qmake 可移植矩阵重新执行 9/9 成功；根 `nmake qmake_all` 成功遍历 VDB、Drishti、render/ITK/MOP plugins、Import 全部插件、Paint 和 Mesh 的构建图。
- `git diff --check` 退出 0（仅 LF/CRLF 提示）；活动源码没有 `glDispatchCompute`；`raw2pvl.cpp` 与 Image Stack RGB 导出不再存在忽略返回值的 `createFile(true);` 或目标 `setSlice(...);` 调用；全部目标 `volData->getDepthSlice()` 调用均显式检查失败。
- 仍未在 i7-13700H 笔记本、用户原图或正式 Intel 驱动上运行。Image Stack 插件链接使用本机 Anaconda Qt，只是源码构建证据，不得复制其 DLL 到目标安装目录。
- 尚缺故障注入级 MHD 磁盘写满测试、真实损坏 codec 硬超时、目标机完整 Release 链接和真实 OpenGL 4.2/4.5 shader/FBO 执行证据。因此本文件的 85%/62%/92% 仍是工程置信度，而非现场通过率。

## 6. 当前完成状态与目标机剩余必修项

### P0-1：目标机正式构建和真实驱动验证

1. 使用目标安装完全匹配的 Qt 5.15、MSVC、CRT、libtiff、QGLViewer、Assimp、netCDF 和 FFmpeg 构建四个程序及插件。
2. 在非远程桌面会话记录 Vendor、Renderer、OpenGL、GLSL，以及 `GL_CONTEXT_PROFILE_MASK` 的十六进制数值和解析结果；主程序必须拿到 4.5 compatibility，Paint/Mesh 必须拿到 4.2 compatibility。当前源码尚缺 profile mask 日志，必须补齐。
3. 在初始化代码中明确识别并拒绝 `GDI Generic`、`Microsoft Basic Render Driver` 和已知远程桌面软件 Renderer，进入可诊断错误页且不继续创建 shader/FBO；不能只依赖测试人员目测日志。保留可配置的显式 override 只能用于诊断，不能作为发布默认值。
4. 逐项运行主 volume、lighting、crop/prune、Paint raycast 和 Mesh display 的 shader/FBO 路径。不能只验证启动页。
5. 若 Intel/AMD 驱动拒绝 shader，先保存规范化后的最终源码和完整日志，再做最小语义修复；不要盲目全局替换版本字符串。

### P0-2：已完成编码，目标机同步并复核底层 I/O

4.7 所列实现已经完成，并通过 safety smoke 与关键对象编译。目标智能体不应重新设计这套接口，而应先逐项比对目标源码是否包含同等契约；若目标仓库没有本工作树修改，再完整移植相关调用链，不能只复制 `volumefilemanager.cpp`。

目标机仍需完成以下 P0 验证：

1. 用正式依赖完整编译所有受 `createFile()/setSlice()` 返回类型变化影响的调用方。
2. 对缺文件、截断文件、只读目录、seek/read/write 短操作和磁盘写满做故障注入，确认失败不会返回旧切片或保留部分输出。
3. 对极端但合法的宽、高、深只计算或安全拒绝，确认不会按最大轴平方分配，也不会发生整数溢出。
4. 对底层 API 独立验证候选对象失败时不泄漏、不暴露半初始化 volume，也不遗留临时 LOD 文件。应用层“旧工作集不变”的契约尚未实现，必须按 P0-6 完成；切到 `DummyVolume` 后能再加载小样不算通过。
5. 按 4.9–4.11 用正式依赖编译并故障注入 Paint VFM、内存准入、Checkpoint、ITK、Mesh/MeshPaint 和 masked RAW；特别验证压缩 mask 保存失败、stop/reset/退出时的 dirty 状态、已有同名输出、8/16-bit mask、损坏 Checkpoint、物理/Commit 预算不足、ITK 16-bit 输入以及整卷算法接近预算时的行为。多 slab 写入必须注入第二个及后续 slab 的失败，并证明不会留下混合代际卷。

### P0-3：已完成核心编码，目标机复核固定 slab 峰值

4.8 所列 RGB/RGBA 流式 slab 和 2–4 卷“最终交错 slab + 单通道 scratch”已经实现，正常 highres 上传不再建立完整降采样整卷。目标机的工作重点从“继续编码”改为“用正式构建证明内存峰值和结果一致性”：

1. RGB、RGBA、2/3/4 卷分别覆盖 LOD 1 和 LOD > 1、单 slab 和多 slab、非零 Z 原点及末端薄 slab。
2. 记录每次 CPU 输出 slab、通道 scratch、drag cache、已上传纹理和进程 Peak Private；CPU highres 暂存必须由最大单 slab 决定，不随完整体深度增长。
3. 对每种通道数比较首层、末层、随机体素、alpha/opacity 和导出后重载结果，避免只看“没有崩溃”。
4. 失败注入必须删除半写 RGB/opacity/重采样输出，并使 Viewer/UI 可重试。
5. 初始建议：8 GiB RAM 用 128 MiB，16 GiB 用 256 MiB，32 GiB 以上才把 512 MiB 作为候选上限；最终值由 Peak Commit 验收，而不是只看 OpenGL 报告的共享显存。

### P0-4：补齐纹理/slab 行为测试

高 LOD + 末端极薄 XY 裁剪的代码级越界已在 `VolumeSingle::getSlab()` 和同源 `getSubvolume()` 路径修复：降采样源尺寸保证至少为 1，并按实际 `maxHsl/maxWsl` 与输出宽高夹取 `imin/jmin/offH/offW`。1,393,920 个组合模型覆盖完整维度 1..32、LOD 2/3/4/8/16、卷偏移和末端单像素/极薄裁剪，所有源、目标区间均保持在缓冲内。目标机仍需用真实文件和哨兵缓冲验证实现集成，不能只验证 `minX=minY=0` 的整幅数据。

至少覆盖：

- 深度 1，以及深度 5/9/10 配合 LOD 2/3，校验最后一个原始切片的空间范围没有丢失。
- XY 尺寸 1..9、LOD 2/3/4/8，以及从末端开始的单像素/极薄裁剪，校验 `getSubvolume()` 与 `getSlab()` 均无越界。
- 单卷、2/3/4 卷、RGB 和 RGBA 的单 slab 与多 slab，校验每次上传的首层、层数、字节数和最后一层。
- 单层已超过纹理预算、二维宽高接近 `GL_MAX_TEXTURE_SIZE`、数组层数接近 `GL_MAX_ARRAY_TEXTURE_LAYERS` 的拒绝或降采样行为。
- CPU 暂存、二维打包空槽和实际 GL allocation 都计入预算；分配失败必须返回 UI 错误，不得进入换页风暴。

任何 AddressSanitizer、Application Verifier、GL debug output 或哨兵缓冲报告越界，都应视为发布阻断。RGB/RGBA、多卷和奇数深度的目标机集成测试通过前，不要把核心工作流提高到 **88%–93%**；即使通过，也不改变“全部菜单和插件”仅 **50%–65%** 的独立口径。

### P0-5：Trisets 状态守卫已扩展，补齐其他 FBO 路径、预算与实机核验

启动 `rendererReady` 已覆盖三个 Viewer 的启动资源，当前工作树已把审计到的 lighting、prune、volume 和 Mesh 延迟 shader/FBO 改成失败返回或功能降级。目标智能体仍必须逐项核对，因为“删除 `exit()`”本身不代表每个调用方都能处理零 program/FBO：

1. 不要按旧说明重写两套 `ScopedTrisetGlState`：renderbuffer、VAO、array buffer、active texture，以及单元 0–7/原 active unit 的 2D/rectangle texture 状态都已覆盖。用预置非零哨兵对象验证成功、预算拒绝和 GL allocation 失败三条路径；vertex attribute enable/pointer 是 VAO-local，正常 fallback 路径恢复原 VAO 即可，不能再笼统写成“VAO/attribute 完全未保护”。
2. 按 fallback 实际修改面核对并按需补齐 lighting、line smooth/width、point state、depth range、scissor、clear values、draw/read-buffer selection；同时验证纹理单元超过 7 或其他 texture target 时的契约。只保护当前审计路径可以，但必须在类型/注释/测试中明确它不是通用 GL 快照。
3. `createTrisetFramebuffer()` 和外层 `createFBO()` 是嵌套状态保护；确认内层析构不会把外层期望提交的 candidate 状态误恢复。旧内部 FBO/renderbuffer/texture 被删除时继续映射保存绑定，任何其他保存后又被删除的 GL 对象也必须同样处理。
4. 状态恢复要求适用于所有 FBO 创建、校验和替换路径，不只适用于 Trisets。至少审计 Paint `Viewer::createFBO()/createFramebufferSet()`、Drishti/Mesh Viewer 的 image/lowres/save-image FBO、`DrawHiresVolume`、lighting、prune 和 `RcViewer`。`QGLFramebufferObject::release()` 会绑定默认 framebuffer，不能视为恢复调用者原绑定；每条路径必须按实际修改面保存并恢复 DRAW/READ framebuffer、renderbuffer、VAO/buffer、viewport、program、active texture、被触碰的纹理绑定、draw/read-buffer selection 及相关 enable 状态。
5. 成功替换时，如果保存状态引用了即将删除的旧 FBO、renderbuffer 或 texture，必须映射到 candidate，或在 API 契约中明确解除该引用；失败、预算拒绝和 FBO incomplete 时必须完整恢复原对象与状态。Paint preview 虽已有预算和 candidate 分配，仍需要补齐这项状态契约。所有路径都要用非零哨兵分别验证成功、预算拒绝、GL allocation/FBO incomplete 和旧对象替换，并断言恢复结果没有绑定已删除对象。
6. `rg -n "\\bexit\\s*\\(" drishti tools` 后逐个分类。launcher 或批处理正常结束可保留；交互功能的 shader、FBO、分配和文件解析失败不得退出整个进程。
7. shader 工厂失败后必须删除 candidate、把 program 置零并返回；每个调用方在设置 uniform 或 draw 前检查零 program，停用对应功能并恢复 framebuffer/viewport/UI 状态。
8. 给人工常用路径增加语义名称；内容哈希用于稳定关联，不能替代 `lighting/diffuse`、`volume/highres` 这类名称。
9. 在实际附件配置完成处检查每个延迟 FBO，并在失败时释放半初始化资源。把同一受检预算扩展到上述非 Trisets 路径，或为每条路径建立有证据的等价上限和降级策略；预算必须覆盖旧资源、candidate、驱动 staging 和同阶段其他常驻附件的并存峰值，而不只是新 attachment 字节和。
10. 正常情况下可只记录摘要；失败时必须记录最终 shader 源码、compile/link log、GL error、FBO status、估算 attachment 字节、预算档位，以及旧资源/candidate/driver staging 的并存峰值。
11. 当前 512 MiB 公式在 3840 x 2160 下估算：主程序 Trisets 约 538 MiB、Mesh Trisets 约 791 MiB，均会拒绝；Paint preview 约 380 MiB，会通过公式。目标机既要验证前两者的 basic-rendering 降级，也要验证 Paint 在旧 FBO + candidate + staging 并存时不会触发系统换页；必要时采用 `RGBA16F`、自适应尺寸或更低预算，但必须先证明算法/画面精度可接受。

### P0-6：阶段 2部分实现，Drishti MainWindow 两阶段加载与项目原子提交仍未关闭

这是新增背景确认后的发布阻断项。阶段 2已经把部分普通 volume/candidate、时间点 manifest 预检和项目 XML 局部解析接上，但 lowres/preferences/TF/keyframe/渲染资源尚未形成完整 detached candidate，因此仍不能关闭本项。目标智能体必须继续修改 `drishti/mainwindow.cpp/.h`、`drishti/volume.cpp/.h` 及候选资源所有权链：

1. 单通道、RGB/RGBA、2/3/4 卷和 DummyVolume 都先构造独立 candidate；完成文件/manifest、容量、volume、lowres、必要 GL/渲染资源验证前，不得调用会清空活动状态的 `preLoadVolume()`、`clearVolumes()` 或等价逻辑。
2. 把 `preLoadVolume()` 的破坏性清理移到成功提交之后，或拆成“无副作用预检/候选准备 + 不失败提交”。旧 volume 和场景对象必须一直保有所有权并保持可渲染，直到 candidate 已具备接管条件；只保存已被删除对象的裸指针不构成回滚。
3. 成功时一次性切换活动 volume、`Global::volumeType()`、RawVolume、lighting、几何、TF、关键帧、相机和 Viewer 状态，再释放旧资源。若提交阶段仍可能失败，必须使用可回滚事务，不能形成新旧混合状态。
4. **项目解析阶段必须无副作用。**把 XML、volume type、四组文件列表、渲染/偏好字段、相机 FOV、repeat type、TF、lowres 和关键帧解析到独立 `ProjectCandidate`/DTO；解析和验证期间不得修改 `m_volFiles1..4`、任何活动 `Global::*` 状态、Viewer/相机、PreferencesWidget、TF 容器、关键帧、当前项目路径、previous directory、窗口标题或活动工作集。当前 `loadVolumeFromProject()` 会把打开/XML 解析失败默认为 `DummyVolume`，并在解析中直接修改这些现场状态，必须改为 `bool/Result + candidate + error` 或等价的显式失败接口。
5. `loadProject()` 及 lowres、preferences、TF、keyframe 等项目子加载器必须统一返回成功/失败并支持 detached candidate；不得继续使用 `void`、弹警告后返回、先清空活动容器再解析，或某个子资源失败后继续加载后续资源的契约。先为每类资源明确必需/可选语义：必需资源缺失、损坏或截断必须令整个 candidate 失败；真正可选的资源可以缺失并在 candidate 中形成显式默认/空状态，但已存在却损坏或截断不能按“空数据成功”处理。尤其不能为了新事务实现而把历史上合法的不含 keyframes 项目改成加载失败。
6. 只有项目 XML、volume 以及按上述契约解析的 lowres、preferences、TF、关键帧、几何和必要渲染资源全部准备完成后，才能统一提交项目路径、窗口标题和 UI。任一必需资源失败或任一已存在资源解析失败，都必须短路后续加载并销毁 detached candidate，不得把部分候选内容泄漏到活动状态。
7. 测试必须在一个已加载、含几何/TF/关键帧/相机/偏好状态且可正常渲染的项目上执行。失败负例除非法 volume 和 OOM/准入拒绝外，还要覆盖项目文件无法打开、截断/畸形 XML、未知 volume type、缺失必需文件列表、RGB/RGBA 空列表，以及已存在但损坏/截断的 lowres、TF、preferences 和 keyframe sidecar/节点；每次失败前后逐项比较旧数据/状态（可序列化部分比较字节或哈希），并继续渲染一帧。对契约定义为可选的资源还要测试“缺失但项目合法”的成功路径，提交后应得到新项目的显式默认/空状态，而不是泄漏旧项目内容。旧工作集、偏好、当前项目/标题或 renderability 在失败路径任一变化即失败；随后能再加载小样只能作为附加检查，不能替代不变量证明。

### P1：共享内存和交互性能策略

实时物理内存/Commit 准入、Checkpoint、异步 mask 保存、多 slab 事务、生命周期失败传播和 Undo 已按 4.9–4.11 完成编码。以下清单同时标明目标机验证项和真正剩余的性能工作；不要重复实现已经闭环的保存/Undo 契约。

- 在目标机复核 Import、Paint 整卷加载、GraphCut、六个 ITK 插件和 8 个 Paint 原生入口的实时物理内存/Commit 准入，并把同一策略扩展到 LiveWire、其余旧形态学和实际 OpenGL staging；当前模型尚未读取驱动实时占用，也未让所有算法支持 out-of-core。
- 对尚未覆盖的整卷算法建立操作级峰值模型；任一保守峰值超过可用物理内存或 Commit 余量时，自动选择 out-of-core/brick/ROI，或在分配前明确拒绝。不得再用“预计未超过总 RAM”作为允许整卷入内存的条件。
- 默认纹理预算不超过物理 RAM 的约 5%–10%，并保留足够 CPU 解码、PVL、FBO、驱动 staging 和系统余量；8/16/32 GiB 必须使用不同默认档。
- 固定 slab 已移除 RGB/RGBA 的完整 CPU 降采样整卷，但 512 MiB 共享纹理、当前 CPU slab、驱动 staging、FBO 和系统进程仍可能形成接近或超过 1 GiB 的增量压力。8 GiB 应从 128 MiB 验收，16 GiB 从 256 MiB 验收，不要直接提高预算。
- Paint 的 512 MiB 是 data + 16-bit mask 的常驻纹理数据上限，不是进程峰值上限；`glTexImage3D` 返回前可能同时存在 CPU 暂存、已分配纹理和驱动 staging。目标机仍应记录 Commit/Peak Private，并在 8–16 GiB RAM 设备上优先从 256 MiB 验收。
- 拖动时降低 viewport 尺寸、ray step 和阴影质量；停止交互 150–300 ms 后恢复静止质量。
- 根据最近帧耗时闭环调整渲染比例，目标交互 15–30 FPS，而不是固定高分辨率硬撑。
- 大体积优先 brick/子采样；预算不足时预先提示，不要先申请到系统换页。
- GraphCut 已有预算和事务式失败恢复；继续用同一模式审计形态学、连通域、LiveWire 和其他大数组算法，不得由未捕获 `std::bad_alloc` 终止整个 Paint。
- 把 GraphCut 求解移到可取消的 worker；worker 只处理快照，完成后回到 GUI 线程事务提交结果。运行期间显示进度/取消，不允许后台线程直接访问 QWidget 或可变的 `m_tags`。
- **已完成编码，目标机验证**：压缩 mask 保存使用 350 ms debounce、generation 合并、单 worker、不可变磁盘 snapshot 和显式 flush；后台不读取正在修改的 `m_volData`。在目标机测量连续画笔、显式保存、切卷和退出时的最长 GUI 停顿，并注入磁盘写满/目录暂时不可达，确认 dirty 状态可重试。
- **已完成编码，仍需演进**：`flushPendingChanges()`、worker stop、`reset()` 和 `setMemMapped()` 均传播失败，失败时禁止释放 dirty buffer 或改变旧映射；后台无进展和关闭等待均有 30 秒上限。剩余工作是把 GUI 线程整卷原始 snapshot 演进为 dirty-chunk/分块不可变快照，并清扫异常退出遗留的 snapshot 临时文件。
- **已完成编码，目标机故障注入**：非压缩多 slab 保存使用全体 staging、`STAGING`/`PREPARED`/`COMMITTED` journal 和统一提交/回滚，且 `STAGING` 在首个 stage 前落盘。继续注入第二个及后续 slab 失败与各崩溃点，验证三种 journal 状态的恢复。
- **已完成编码，目标机验证**：Checkpoint 和一层 Undo 均使用 staging 校验与事务提交；`paint3DStart()` 在 flush/createUndo 失败时禁止整次笔触。继续覆盖 8/16-bit、大 mask、损坏/截断/OOM/保存失败，并记录每次大 mask 笔触开始时的 flush 和文件 copy 延迟。
- **已完成编码，目标机验证**：六个 ITK/VED 插件和 8 个 Paint 原生三维入口已有保守 `Peak Commit` 模型；用正式 ITK 构建并在阈值两侧验证拒绝与成功路径，同时对异常注入证明 mask/旧输出不变。LiveWire 与未列出的旧算法仍需同样处理。
- CPU 网格画笔先建立大网格基准；若连续笔触不能达到目标，再增加空间索引或只更新命中顶点范围，避免每个事件扫描全部顶点。
- 旧 PLY、netCDF 和视频帧分配代码仍有进程级 `exit()`。对损坏/罕见输入的“全部菜单可靠性”要求，必须把库级终止改成错误返回；若本轮不改，应明确列为不支持的边缘路径，而不是计入 **88%–93%** 的核心口径。
- `bin/assets/scripts/paint` 中的 UNet/UNet3+ 路径依赖 TensorFlow。它们可能回退到 CPU，但训练或大图推理是否达到可接受速度不属于 OpenGL 核显兼容性；必须单独记录 TensorFlow 版本、执行设备、峰值 RAM 和耗时，不能用常规 Paint 通过来代表 ML 脚本通过。

### P2：不是当前任务的项目

当前活动路径没有 compute dispatch，网格画笔也已经 CPU 化。因此不要为本轮再设计 compute fallback，也不要尝试把主程序体渲染改成 CPU 软件光栅化。若未来重新启用仓库中的旧 compute factory，应作为新功能重新做能力探测和验收。

## 7. 目标设备实施顺序

### 阶段 A：只读基线

记录：

```text
Windows 版本：
CPU / RAM：
页面文件：系统管理 / 固定 / 关闭，Commit Limit：
GPU 名称和驱动版本：
当前 Drishti 安装来源、版本和文件时间：
源码 commit：
Qt / MSVC / libtiff 版本：
问题数据所在介质：本地 SSD / 外置盘 / 网络盘 / 同步目录：
实际导入菜单：Grayscale TIFF / Image Stack / 其他：
Explorer 是否打开问题目录、预览窗格或缩略图：
Defender/同步客户端是否同时读取问题文件（只记录，不要为测试关闭安全防护）：
```

使用 `tiffinfo` 或最小探针只读全部图片元数据。不要分配完整像素缓冲。输出每个文件/页面的 width、height、bits/sample、samples/pixel、sample format、planar config、photometric、orientation、compression、是否 tiled、页数和预计解码字节。

先把一份只读测试副本放到短路径的本地 SSD 目录，并关闭该目录的 Explorer 预览窗格，再与原位置各测一次。若只有 Explorer 打开目录或同步/网络位置时才复现，应另外抓取 Process Monitor/WPR I/O 证据；这与 DrishtiImport 自身的 Commit/Hard Fault 曲线必须分开判断。

### 阶段 B：先构建导入插件，再构建 Import EXE

使用目标安装相同的 Qt 5.15/MSVC ABI。示例中的路径必须替换成目标机真实路径：

```powershell
Import-Module 'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell `
  -VsInstallPath 'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools' `
  -DevCmdArguments '-arch=amd64 -host_arch=amd64'

$qmake = 'C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe'
$buildDir = 'C:\DrishtiBuild\tiff-plugin'
$deployDir = 'C:\DrishtiBuild\deploy'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
Set-Location $buildDir

& $qmake `
  'C:\src\drishti\tools\import\plugins\tiff\tiff.pro' `
  "DRISHTI_BIN_DIR=$deployDir" `
  'DRISHTI_VCPKG_ROOT=C:\Apps\vcpkg' `
  'DRISHTI_VCPKG_TRIPLET=x64-windows' `
  'TIFF_INCLUDE_PATH=C:\Apps\vcpkg\installed\x64-windows\include' `
  'TIFF_LIBRARY_PATH=C:\Apps\vcpkg\installed\x64-windows\lib'
nmake /NOLOGO release
```

检查生成 Makefile：指定 TIFF include/lib 必须在其他 libtiff 路径之前，`DESTDIR_TARGET` 必须是 `$deployDir\importplugins\tiffplugin.dll`。用 `dumpbin /dependents $deployDir\importplugins\tiffplugin.dll` 检查 Qt 和 TIFF 依赖。备份正式 `bin\importplugins\tiffplugin.dll` 后再替换。

普通 Image Stack 也按同一 ABI 单独构建：

```powershell
& $qmake C:\src\drishti\tools\import\plugins\imagestack\imagestack.pro `
  "DRISHTI_BIN_DIR=$deployDir"
nmake /NOLOGO release
```

插件通过选择/扫描小样后，再按 4.12 配置 Python/OpenVDB/Gmsh/VDB，构建包含 `importmemoryadmission.cpp/.h` 的完整 `tools/import/import.pro`：

```powershell
$importBuild = 'C:\DrishtiBuild\import-app'
New-Item -ItemType Directory -Force -Path $importBuild | Out-Null
Set-Location $importBuild
& $qmake C:\src\drishti\tools\import\import.pro `
  "DRISHTI_BIN_DIR=$deployDir" `
  'DRISHTI_VCPKG_ROOT=C:\Apps\vcpkg' `
  'DRISHTI_PYTHON_ROOT=C:\Python311' `
  'DRISHTI_PYTHON_LIB=python311.lib'
nmake /NOLOGO release
```

只替换插件不能获得 `raw2pvl.cpp`、`volumedata.cpp` 的端到端准入；必须分别记录“插件选择/统计”和“保存 PVL/转换”两个阶段的 Peak Commit。

### 阶段 C：导入现场验收

依次运行：

1. 合成 8/16 位灰度小样。
2. 合成 4096² uint16 的 10 张和 40 张固定内存压力样本。
3. 原始数据中的 1 张。
4. 原始数据的 2 张、4 张。
5. 最后才是原来的 10 张。
6. 取消一次，再重新成功导入一次。
7. 预览中连续查询像素 1000 次。
8. 保存 PVL 并重新加载，校验方向、min/max 和随机像素。

每次记录：

```text
Peak Working Set
Peak Private Bytes / Commit Size
系统 Committed / Commit Limit
Hard Faults/sec
数据盘与页面文件盘 Active Time
CPU 占用
最长 UI 无响应时间
结果文件和输入随机像素校验
```

通过条件：

- 10 张和 40 张测试的峰值只由固定数量的当前切片/扫描行缓冲决定，不出现 `depth * sliceBytes` 增长。
- 导入窗口持续响应，取消可用；Explorer 不受影响。
- TIFF 插件对损坏、RGB、混合尺寸/位深、tiled、`MINISWHITE` 和非 `TOPLEFT` 输入在解码前或安全错误点拒绝。
- Image Stack 对灰度、RGB、RGBA 分别成功导入；损坏图片和混合尺寸在安全错误点拒绝。
- 连续像素查询后 Private Bytes 不阶梯增长。

### 阶段 D：同步既有内存安全改动并实现两阶段加载

P0-2、P0-3 以及 4.10–4.11 已在本工作树完成。目标智能体应按 4.7–4.11 对照目标源码，缺失时移植整条接口和调用链；已有时不要重复重写。随后先做不创建 OpenGL 上下文的定向测试：

1. 极端但合法的宽、高、深组合只计算/拒绝，不得发生 `int` 溢出或按最大轴平方过度分配。
2. 缺文件、截断文件、只读文件、seek/read/write 短操作必须返回失败，不能返回旧切片或成功状态。
3. RGB/RGBA 和 2/3/4 卷在 1/多 slab 下记录每次 CPU buffer 容量，峰值必须由固定 slab 决定，不随完整体深度增长。
4. 先完成 P0-6，再在已有可渲染项目上注入非法/OOM 单通道、RGB/RGBA、2/3/4 卷、DummyVolume 和项目候选，并覆盖无法打开、截断/畸形 XML、未知 volume type、缺失必需文件列表、RGB/RGBA 空列表，以及已存在但损坏/截断的 lowres、TF、preferences、keyframe sidecar/节点。失败前后的 `Volume::valid()`、`Global::volumeType()`、体数据、几何、TF、关键帧、相机、偏好、当前项目/标题和可渲染性必须逐项、状态对状态保持不变；可序列化数据比较字节或哈希，并继续渲染一帧。另测缺失可选资源的合法项目，成功提交后使用新项目的显式默认/空状态；随后成功加载小样只是失败路径的附加条件。
5. Paint 对截断 PVL/mask、损坏 Blosc block、只读目录和写满磁盘必须返回错误；显式保存或关闭时不得丢失最后一代修改。用典型 mask 记录一次画笔结束、自动保存和退出保存的最长 UI 停顿。
6. ITK、Mesh/MeshPaint、masked RAW 和 Paint 提取分别注入中途短读/短写；已有同名输出必须保留，失败只清理本轮新建的临时或部分文件。
7. 在小卷、阈值附近和超阈值数据上记录实时物理/Commit 预算、系统/核显预留和最终模式；当前 `setFile()` 在预算不足时必须于大分配前明确拒绝，显式 offload 路径也不得偷偷整卷分配，不能再出现持续换页。不要把安全拒绝记为大卷功能通过。
8. 对 8/16-bit mask 分别创建、加载、删除 Checkpoint，并测试损坏 FAT、offset、block size、截断和 OOM；失败前后活动 mask 与正式文件哈希必须不变。Undo 必须证明操作边界确实生成快照，损坏 Undo 也不得覆盖正式 mask。

### 阶段 E：构建和验证三套 OpenGL 程序

使用正式依赖构建 `drishti`、`drishtipaint` 和 `drishtimesh`，并完成第 6 节 P0 验证。启动时保存：

```text
Vendor
Renderer
OpenGL version
GLSL version
Context profile mask（十六进制数值 + compatibility/core 解析）
GL_MAX_TEXTURE_SIZE
GL_MAX_3D_TEXTURE_SIZE
GL_MAX_ARRAY_TEXTURE_LAYERS
所有失败 shader 的最终源码和 compile/link log
所有启动 FBO 状态
```

在 Windows GUI 子系统看不到 `qInfo()` 时，使用 DebugView，或在目标构建中安装 Qt message handler 写入 `%LOCALAPPDATA%\Drishti\logs\opengl.log`。

### 阶段 F：完整核心工作流

至少完成：

```text
TIFF/Image Stack -> DrishtiImport -> PVL
PVL -> Drishti 主程序 -> 低/高分辨率显示 -> 截图
PVL -> Drishti Paint -> 新建/修改 mask -> 保存并重新加载
体数据/表面 -> Drishti Mesh -> 显示/编辑 -> 导出网格
短视频或图像序列导出（验证 FFmpeg ABI）
```

测试三档数据：小样、典型数据、接近目标上限的数据。只有小样通过不能证明共享内存预算正确。

## 8. 核显验收矩阵

| 模块 | 必测操作 | 通过标准 |
|---|---|---|
| 启动 | 三个 OpenGL 程序各启动 5 次；另测通用/软件 Renderer 拒绝分支 | 实际 Renderer 为 Intel/AMD；profile mask 数值及解析符合请求；`GDI Generic`/`Microsoft Basic Render Driver`/远程软件实现进入错误页且不创建 shader/FBO；无随机黑屏或启动崩溃 |
| Shader | 主渲染、光照、crop/prune、Paint raycast、Mesh display | compile/link 全成功；失败能定位到稳定标签和语义功能 |
| FBO | 窗口缩放、截图、低/高质量切换；三处已预算路径（含 Paint Viewer preview）及未预算的 Drishti/Mesh Viewer、`DrawHiresVolume`、lighting、prune、`RcViewer`；用非零绑定/状态哨兵覆盖成功、预算拒绝、GL allocation/FBO incomplete 和旧对象替换 | 每次重建 complete 或明确降级；不继续访问空资源；原 GL 状态按修改面恢复且不绑定已删除对象；日志含 candidate、旧资源与 staging 峰值；所有 FBO 有受检上限，不能用旧算术 smoke 代替 |
| TIFF 导入 | 1/2/4/10 张原数据；RGB/tiled/混合布局/损坏负例 | UI 响应，内存不按深度线性增长，灰度结果一致；不支持布局明确拒绝 |
| Image Stack | 灰度、RGB、RGBA；混合尺寸和损坏图片 | 支持类型结果一致；错误输入安全拒绝；取消后可重试 |
| Import -> PVL | 无滤波、最大 spread、padding、Merge、Quick RAW；预算阈值上下各一组 | 峰值模型包含所有并存缓冲和滤波窗口；超预算在打开输出/分配前拒绝；成功 PVL 随机体素一致 |
| 体数据 I/O | 截断 PVL、缺失 slab、只读输出、短读/短写及后续 slab 提交失败注入 | 明确失败、清理部分输出、状态可重试；不复用旧切片；旧多 slab 卷不出现混合代际 |
| Drishti 候选加载 | 在已填充项目上加载非法/OOM 的单通道、RGB/RGBA、2/3/4 卷、DummyVolume；另测无法打开/畸形或截断项目 XML、未知类型、缺失必需列表、已存在但损坏的 lowres/TF/preferences/keyframe，以及缺失可选资源的合法项目 | 解析阶段无副作用；失败立即短路且旧 volume、几何、TF、关键帧、相机、偏好、当前项目/标题逐项不变并仍能渲染；不得退到空 `DummyVolume`；合法缺失可选资源提交为新项目默认/空状态；成功提交不出现新旧混合状态 |
| RGB/多卷 | RGB、RGBA、2/3/4 卷各测单/多 slab | CPU Peak Private 由固定 slab 决定；不构建整卷高分辨率缓存 |
| Paint | 涂画、GraphCut、8 个已准入三维算法、LiveWire、保存/退出失败、8/16-bit Checkpoint、Undo、损坏已有 mask | 算法完成且结果重载一致；超预算在分配前拒绝；故障注入后 mask 哈希/旧文件不变；非零 ROI 的三个 Watershed 不写 ROI 外；记录最长保存停顿 |
| ITK/VED | 六个已准入插件的小卷、阈值上下、16-bit 输入和输出失败 | 正式 ITK ABI 下编译运行；拒绝发生在整卷 ITK 分配前；失败保留旧输出；不得用 profile smoke 代替插件运行 |
| Mesh | 生成、CPU 画笔局部编辑、导出 | 画笔结果正确且交互期间系统响应；导出网格可重新加载 |
| 压力 | 连续成功/取消/失败各 20 次 | Commit、句柄和 GL 资源不持续增长 |
| 性能 | 典型数据旋转、缩放、调整传输函数 | 记录 15–30 FPS 目标；低于目标则降低预算/LOD/viewport 后重测，自动帧时闭环仍属于 P1 |

## 9. 部署和回滚

1. 导入插件与渲染程序分开部署、分开记录 hash。
2. 每个被替换的 EXE/DLL 先复制到带日期的本地备份目录。
3. 不要混用 Anaconda Qt、官方 Qt、MinGW 和 MSVC 产物。
4. 首轮只替换 `tiffplugin.dll`/`imagestackplugin.dll`。导入问题关闭后，再部署三套渲染程序。
5. 若新插件不能加载，立即恢复同目录备份并检查 `dumpbin /dependents`，不要继续复制随机 DLL 补依赖。
6. 若核显版出现 shader/FBO 失败，保留导入修复，单独回滚渲染 EXE；两者没有运行时依赖。

## 10. 目标设备回传模板

```text
设备 / RAM / 页面文件：
GPU / 驱动：
源码 commit：
正式 Qt / MSVC / libtiff：
实际 Renderer / OpenGL / GLSL / profile：
通用/软件 Renderer 拒绝结果：

导入菜单和问题数据元数据摘要：
修复前 1/2/4/10 张 Peak Private：
修复后 1/2/4/10 张 Peak Private：
修复后 UI 最长无响应：
取消与 rawValue 压力结果：

主程序 shader/FBO：
Paint shader/FBO：
Mesh shader/FBO/CPU 画笔：
128/256/512 MiB 档位的 Peak Commit：
RGB/RGBA/2–4 卷固定 slab 峰值：
候选加载失败后旧工作集不变量/哈希：
小样/典型/上限数据 FPS：

完整核心工作流：通过 / 未通过
未通过步骤和首个错误日志：
剩余限制：
部署文件 SHA-256：
回滚位置：
```

## 11. 最终判断

对 i7-13700H 和近五年的 Intel/AMD 核显，**CPU 负责导入、标注和构建，核显负责 Desktop OpenGL 显示**是现实路线；架构本身的可行性约 **90%–95%**。当前典型灰度核心链路约 **82%–88%**，中心估计 **85%**；目标机用正式依赖、真实 Intel/AMD Renderer 和用户数据跑通矩阵后，核心工作流约 **90%–94%**，中心估计 **92%**。

当前已经修复多项与现场症状相符的确定缺陷，包括导入 GUI 阻塞、泄漏、输入边界、codec 前准入、Import -> PVL 大缓冲、底层卷文件 I/O 与候选对象内部失败传播、RGB/RGBA 与多卷固定 slab、已审计 shader/FBO 子路径的失败降级、CPU 网格画笔、GraphCut、六个 ITK 插件和 8 个 Paint 原生三维算法。此后又补充了 Paint 固定大体积门槛移除、curves 短读/有界解析、正式 PVL Save As 的 16-bit padding/空间预检/旧尾 slab 清理、RAW+padding 明确拒绝，以及 Drishti 命令行选项消费。profile-mask 日志、软件 Renderer 主动拒绝、非 Trisets FBO 预算/状态恢复仍未完成，不能包含在“已修复”口径内。**Drishti 应用层项目资源的完整事务式加载尚未完成：低清晰度、preferences、TF、keyframe 和纹理资源仍由旧的就地接口处理，必须先关闭 P0-7 的剩余边界才能把它列为已修复。**同时没有目标 EXE、原图和资源曲线，仍不能声称找到了现场唯一首因。基线 `b53bd97` 的 TIFF 是 GUI 线程顺序单切片实现；`BATCH_SIZE=32` 只是后来撤销的实验实现，只有目标二进制证据匹配时才能列为首因。

剩余不确定性和未完成项主要是 Drishti 两阶段候选/项目加载、目标 Intel/AMD 驱动上的全部延迟 shader/FBO、未纳入预算的 Viewer/volume/lighting/prune FBO、旧 + candidate + 驱动 staging 并存峰值、Trisets 守卫未覆盖状态的实机哨兵验证、正式第三方依赖的完整链接、用户原图、单次第三方 codec 无硬中断、GUI 线程整卷原始 snapshot、snapshot 临时文件清扫、真正的 out-of-core 覆盖、未审计算法、GraphCut 等 CPU 同步长任务，以及 CPU 画笔的大网格性能和 ridged 图案等价性。`meshvertexbuffer.h` 的 packed-count 乘法已在运算前提升到 `qint64` 并通过独立 MSVC 编译。所有当前 Blosc 写路径均为 level 3；后台无进展和关闭等待已有 30 秒上限；多 slab 在首个 stage 前已有 `STAGING` journal。

按当前证据，原来约 10 张图片端到端不再拖死系统约 **80%–90%**；字面意义上所有菜单、ITK/VED/ML/OpenVR 和罕见格式都能运行约 **55%–70%**，中心 **62%**；典型尺寸日常任务较流畅约 **65%–80%**；大 mask、复杂网格和接近内存上限时仍全部流畅只有 **30%–50%**。不要把这些数字解读为无限数据规模或固定 30 FPS。
## 2026-08-13 后续阶段实施同步

当前源码继续保留并扩展此前 CPU + 核显安全边界：TIFF 混合 orientation 拒绝、显式/目录自然排序、单体素 ROI 安全映射、PVL Save As nearest 锚点、Save Images 批次 staging、Time Series 输出重名拒绝和 Drishti 项目/keyframe 前置校验已落地。它们只代表源码实现边界，不代表正式 GUI、OpenGL、目标 i7-13700H/AMD 核显或最终便携包已验收。

当前源码已补充 Drishti geometry/camera/LightHandler candidate、Time Series 切帧失败后的旧帧恢复、batch/MHD 采样契约以及 Paint curves/mask 的同目录持久 journal；这些路径仍需故障注入和正式运行时验证，不能把静态实现当成证据。XML/PVL/RAW/Time Series/sidecar 共享同一底层 `RecoveryJournal` 协议，但跨产品域的崩溃恢复组合也仍需在核验阶段证明。Paint/Drishti GUI 闭环、净包和硬件验收属于后续核验工作。主问题基线以 `DRISHTI_END_TO_END_WORKFLOW_FIX_HANDOFF_2026-08-12.md` 第 14.23 节为准。

2026-08-14 代码收口补充：TIFF/Image Stack 通过独立可选 `SourceFilesProvider` 返回实际有序切片，避免修改既有 `VolInterface` 插件虚表；显式多文件的用户确认顺序不再被 TIFF 插件重排，目录导入 provenance 不再只记录目录名。Paint `VolumeFileManager` 在切换映射状态、创建新卷和加载新卷后会丢弃旧 dirty-chunk baseline，避免跨卷复用 snapshot。相关 qmake 生成、PVL `sourceorder` smoke 断言和 `git diff --check` 已完成；完整 C++ 构建、GUI/故障注入、Intel/AMD 核显和净包仍是后续核验工作，不能由这些静态证据替代。

2026-08-14 代码/局部构建补充：TIFF decode helper 的 Windows stdout 已切换为 `_O_BINARY`，修复像素 `0x0A` 在 CRT 文本模式下被转换导致的输出长度错误。VS2019 BuildTools 14.29 + Anaconda Qt 5.15.2 下，PVL manifest、TIFF orientation/order/provider、Image Stack transactional、RecoveryJournal/Slab、GraphCut/Algorithm/Import memory admission、Framebuffer budget、Paint slice ordering、Sampling contract 和 Binary PLY writer 独立 smoke 已实际编译运行通过；helper 产物为 `C:\bin\tiffdecodehelper.exe`。完整 Import 工程仍被 `pybind11/embed.h`、`pybind11/pybind11.h` 和默认路径缺失 `tiffio.h` 阻断，VFM lifecycle smoke 被 `QGLViewer/qglviewer.h` 阻断；TIFF 独立插件在显式传入 Anaconda TIFF include/lib 路径后构建通过。以上证据只覆盖独立组件，不代表主工程、GUI、OpenGL、目标核显或便携包验收完成。

进入核验阶段时采用八段式证据顺序：1）MSVC/Qt/第三方依赖闭包；2）确定性 fixture 与独立 smoke；3）Import 正式 GUI；4）Paint 保存、重开、取消与故障注入；5）Drishti 项目加载、低/高分辨率与渲染；6）短写、磁盘满、rename/delete、异常退出；7）Intel i7-13700H 与 AMD 核显/OpenGL/FBO；8）同一源码快照生成净便携包、依赖闭包、manifest 和 SHA-256。每段分别记录命令、源码快照、环境、输入 fixture、日志和通过/阻断结论；在第 3 至 8 段完成前，不得把局部 smoke 或 qmake 结果写成全链路通过。

2026-08-14 VFM 运行边界补充：VFM lifecycle smoke 原先用 `QCoreApplication`，但保存快照路径会创建 `QProgressDialog`，在 Windows 上触发 Qt5Core `0xC0000409`；现已改为 offscreen `QApplication`。`VolumeFileManager::createSaveSnapshot()` 的 immutable snapshot 物化也从未诊断的 `QFile::copy()` 改为受检分块读写/flush/尺寸校验，并报告源目标错误。修补后在 VS2019 + Anaconda Qt/blosc 实际编译运行输出 `VFM lifecycle smoke passed`、返回码 0。该结果只关闭 Paint 生命周期组件阻断，Import 完整依赖、GUI/OpenGL/硬件/净包和全链路证据仍按八段式规划后置。

2026-08-14 阶段 2 smoke 复核：PVL manifest、TIFF orientation/order/provider、Image Stack transactional、Slab journal、VFM lifecycle、GraphCut/Algorithm/Import memory admission、Framebuffer budget、Paint slice ordering、Sampling contract 和 Binary PLY writer 均已在当前 DLL/辅助程序路径下返回 0。Image Stack 损坏 PNG 的 libpng 错误属于预期负例，事务回滚通过；TIFF smoke 显式使用 `DRISHTI_TIFF_HELPER=C:\bin\tiffdecodehelper.exe`。这些证据覆盖组件契约，不替代正式 Import/Paint/Drishti GUI、OpenGL、Intel/AMD 核显和净包验收。

2026-08-14 阶段 3 Import 构建记录：用 VS2019 x64 + Anaconda Qt 5.15.2，显式接入 `.lab-agent/deps/pybind11/include`、Anaconda Python 和 TIFF include/lib 后，Import qmake 生成通过；本轮同时补齐 `tools/import/import.pro` 的 `TIFF_INCLUDE_PATH`/`TIFF_LIBRARY_PATH` 接入，实际编译已越过 `tiffpagevalidation.cpp`，当前首个阻断为缺少 `openvdb/openvdb.h`。插件级 TIFF/Image Stack smoke 已通过，正式 `drishtiimport` GUI 尚未启动；必须先固定完整 OpenVDB/VDB/Gmsh/Imath/TIFF/pybind11/Python ABI 闭包。

2026-08-14 阶段 4 Paint 构建记录：使用 VS2019 x64 + Anaconda Qt 5.15.2、QGLViewer 头文件和 Anaconda blosc include/lib 生成 `tools/paint/paint.pro` 通过，但 Release 构建在 `PRE_TARGETDEPS` 处被缺少 `common/lib/vdb.lib` 阻断。独立 VFM/Slab/内存准入/GraphCut smoke 已通过，`drishtipaint` 正式 GUI、Paint 保存重开和完整算法矩阵仍未执行；需先补齐同一 MSVC/Qt ABI 的 OpenVDB/VDB/Gmsh/Imath/Blosc/QGLViewer。

2026-08-14 阶段 5 Drishti 构建记录：接入 `.lab-agent/deps/glew-release/glew-2.2.0/include` 和 QGLViewer 头文件后，`drishti/drishti.pro` qmake 通过并实际编译越过 `GL/glew.h`，当前首个阻断为缺少 `common/src/videoencoder/ffmpeg.h` 所需的 `libavcodec/avcodec.h`。`drishti.exe` 尚未链接/启动，项目候选加载、shader/FBO、低/高分辨率和最终渲染仍未取得证据。
