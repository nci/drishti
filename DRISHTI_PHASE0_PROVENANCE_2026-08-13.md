# Drishti 阶段 0 Provenance 与提交边界

审计时间：2026-08-13  
工作区：`C:\saveproject\LBJ-workspace\_external\drishti`  
当前分支：`codex/cpu-igpu-worktree-checkpoint-20260813`  
当前基线 HEAD：`e731972d2c8663b001ce06384b9b4074854d0c00` (`checkpoint: preserve CPU and iGPU adaptation work`)

## 1. 目的和结论

本文件冻结阶段 0 的来源、边界和提交规则。当前工作区是混合工作树，不能整体暂存、整体提交，也不能把其中已有的代码数量当作阶段完成证明。后续实施必须从明确基线建立可审查的独立提交序列；本目录只作为来源和证据，不是可直接提交的 PR 快照。

阶段 0 的结论：基线、分支和工作区状态已盘点；产品文件、阶段 1 候选、后续阶段遗留和构建/代理产物已分类；阶段 1 的目标提交边界已定义。阶段 1 开始前仍须以本清单逐项复核源码和测试，不得沿用未验证的历史“已完成”声明。

## 2. 工作区状态

- 当前分支：`codex/cpu-igpu-worktree-checkpoint-20260813`
- 当前 HEAD：`e731972d2c8663b001ce06384b9b4074854d0c00`
- 交接文档记录的历史审计基线为 `b53bd979...`；它与当前 HEAD 不同，因此历史二进制、日志和结论不能直接为当前工作树背书。
- 当前状态包含 23 个已跟踪修改文件和大量未跟踪路径，其中绝大多数来自 `.lab-agent` 构建/会话目录。
- 本阶段没有执行 `git add -A`，也没有把混合工作树作为一个提交。

## 3. 逐文件来源与处理决定

### 3.1 阶段 1 候选：共享 PVL schema/manifest

| 文件 | 来源批次 | 对应问题 | 阶段 0 决定 | 行为契约/测试 |
|---|---|---|---|---|
| `common/src/pvlmanifest.h` | 本轮工作树新增候选 | P0-P1、P0-P2、P0-P3、P0-P4、P0-P14 | 保留，阶段 1 单独审查 | 统一解析 PVL schema、名称、几何、分片和文件校验；`pvl_manifest_smoke` |
| `common/src/pvlmanifest.cpp` | 本轮工作树新增候选 | 同上 | 保留，不能直接宣称完成 | 结构化名称须保留空格/Unicode；旧格式仅兼容；严格校验需覆盖 PVL/RAW 契约 |
| `tools/import/tests/pvl_manifest_smoke.cpp` | 本轮工作树新增候选 | P0-P1 至 P0-P4 | 保留并扩展 | 覆盖合法路径、Unicode、fallback、重复/缺失/截断/错误头/错误几何/名称数量 |
| `tools/import/tests/pvl_manifest_smoke.pro` | 本轮工作树新增候选 | 阶段 1 构建 | 保留 | 隔离 qmake 构建；不提交生成的 Makefile/OBJ/EXE |
| `drishti/xmlheaderfunctions.cpp` | 既有 CPU/iGPU/链路混合修改 | P0-P1、P0-P4 | 保留来源，阶段 1 逐调用点复核 | Drishti 读取必须使用统一 parser；失败返回可观察 |
| `drishti/volumebase.cpp` | 既有底层 I/O 与 PVL 接入修改 | P0-P1、P0-P4 | 保留来源，阶段 1 复核 | 底层错误传播和分片边界不能回退 |
| `drishti/drishti.pro` | 既有构建接入修改 | 阶段 1 构建 | 保留，按依赖单独移植 | 共享 parser 源码可构建 |
| `tools/import/import.pro` | 既有 Import 接入修改 | P0-P1 | 保留，阶段 1 复核 | Import writer/reader 与 parser schema 一致 |
| `tools/import/raw2pvl.cpp` | 既有 writer 接入修改 | P0-P1、P0-P4 | 保留，阶段 1 只审 schema；总事务留阶段 6 | writer 输出结构化 manifest；不把 Save As 事务冒充阶段 1 |
| `tools/paint/volume.cpp` | 既有 Paint reader 接入修改 | P0-P1 至 P0-P5 | 保留，阶段 1 复核入口 | Paint 读取统一 parser；内存准入和两阶段加载留后续阶段 |
| `tools/paint/paint.pro` | 既有构建接入修改 | 阶段 1 构建 | 保留，按依赖单独移植 | parser 接入可构建 |
| `tools/paint/staticfunctions.cpp` | 既有 Paint/PVL 接入修改 | P0-P1、P0-P3、P0-P4 | 保留，阶段 1 只复核 parser 使用 | Python/提取路径不得另造 manifest 语义 |
| `tools/paint/pywidget.cpp` | 既有 Paint/PVL 接入修改 | P0-P1 | 保留，阶段 1 复核 | Python 工具得到的路径/几何与共享 parser 一致 |
| `tools/paint/drishtipaint.cpp` | 既有 Paint/PVL 接入修改 | P0-P1、P0-P2 | 保留，阶段 1 复核 | 自生成 header 使用同一 schema；保存事务留阶段 5 |

### 3.2 后续阶段遗留：不得在阶段 1 中冒充关闭

| 文件/范围 | 对应问题 | 阶段 0 决定 |
|---|---|---|
| `tools/import/raw2pvl.cpp` 的 Save As 总事务、ROI/Z、采样、padding、批次提交 | P0-I2 至 P2-I15 | 保留代码来源，但阶段 6 单独处理；阶段 1 只记录 writer schema |
| `tools/paint/curves.cpp`, `curves.h`, `curveswidget.cpp`, `curveswidget.h` | P0-P9、P0-P10、P1-P13 | 保留用户已有修改；不在阶段 1修复曲线事务/EOF/取消 |
| `tools/paint/volumemask.cpp`, `volumemask.h` | P0-P8、P0-P10、P1-P12 | 保留用户已有修改；保存错误传播和同步 I/O 留阶段 5 |
| `drishti/mainwindow.cpp`, `mainwindow.h`, `drishti/staticfunctions.cpp`, `drishti/volume.cpp`, `volume.h` | P0-P6、P0-P7、P0-P15、P1-P16 | 保留用户已有修改；候选加载、时间序列和启动参数留后续阶段 |
| `CPU_IGPU_IMPLEMENTATION_HANDOFF.md` | 历史实现和用户补充背景 | 必须保留用户修改；不以历史文档代替当前源码/测试证据 |

### 3.3 明确排除

以下内容不得进入产品提交：`.lab-agent/`、`aqtinstall.log`、`__pycache__/`、`rgbbase_equivalence.obj`、根目录名为 `-` 的临时文件、`.lab-agent/build` 下的 Makefile/OBJ/EXE/MOC/PDB/日志、下载缓存、会话 JSON、旧 ZIP、运行日志和审计临时数据。`tools/*/tests` 只允许提交可复现的测试源码及 `.pro`，不提交其构建输出。

## 4. 阶段 1 行为契约

共享 reader/writer 必须使用同一 PVL schema。严格解析至少包括：正确 root；唯一且为正数的 `gridsize`/`slabsize`；支持的 voxel type；合法 header size；结构化 `<pvlnames>`/`<rawnames>`；名称中的空格、Unicode 和长路径保持完整；旧式空格列表仅作兼容；名称数量与 slab 数一致；重复/缺失 slab 拒绝；每个 slab 的实际 slice 数、几何、二进制 13 字节头、payload 文件大小一致。嵌套字段、未知结构和不一致 RAW manifest 不能被静默接受。

阶段 1 不负责关闭 Save As 总事务、Paint/Drishti 两阶段加载、mask/tag/curves 事务、导入批次回滚、目标硬件 GUI 验收或最终便携包。

## 5. 阶段 1 测试门槛

1. 在隔离构建目录 qmake/build `pvl_manifest_smoke`，不使用 `.lab-agent/build` 的旧输出作为证据。
2. smoke 必须覆盖：结构化名称、路径含空格、Unicode、fallback、重复 slab、缺失 slab、截断 slab、错误二进制头、错误 gridsize、长/短名称列表，并覆盖 RAW manifest/geometry 契约。
3. 检查 Import、Paint、Drishti 的 reader/writer 是否都走统一 parser；对未接入或只读取元数据的调用点登记为未完成。
4. 运行 `git diff --check`，并记录 qmake/compiler/Qt 版本及测试快照。

## 6. 提交边界

阶段 0 不产生产品代码提交；本文件本身是审计交接记录。阶段 1 目标提交只包含共享 parser、最小可复现 smoke、必要的项目文件和经复核的 reader/writer 接入。不得夹带 Paint 曲线、Save As 总事务、候选加载、便携包或 `.lab-agent` 产物。提交前必须能从明确基线独立构建，并提供问题编号到代码/测试的映射。

## 7. 审计签字规则

只有在阶段 1 parser、所有接入点和 smoke 逐项复核并在隔离构建中通过后，才能把阶段 1标记为“完成”。现有工作树中已经存在的代码只能标记为“候选/部分完成”，不能因为文件存在或历史日志通过而提前关闭问题。
