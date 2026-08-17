# Drishti 运行时全链路排查与修复交接

首次整理：2026-08-12

背景补全：2026-08-13

基线复核：2026-08-13（按原始卡死、CPU + 核显和 PR 目标重构结论）

排查目标：在开始修改前，完整记录当前链路中已经证明正常、确认需要修改、被审计环境阻断和只能在外部环境验证的内容。

目标链路：`TIFF -> DrishtiImport -> ROI/Z 裁剪 -> Save As PVL -> DrishtiPaint -> mask/tag names/curves 保存与重开 -> Drishti -> 便携包`
目标平台：近五年 Intel/AMD 核显设备，同时保留独立显卡加速能力。

当前交接状态：本文已登记 42 个明确编号的问题/风险，并给出已有实现继承矩阵、P0 代码入口和验收门槛；可作为修复智能体的主问题基线。正式 GUI 运行时链路和目标硬件仍可能发现新增问题，故本文不是“所有运行时缺陷已穷尽”的声明。

## 0. 任务背景、用户目标与演进

### 0.1 最初故障和目标设备

这次任务不是一般性的“让 Drishti 支持 CPU”。起因是一台没有独立显卡、使用 Intel Core i7-13700H 核显的笔记本在 DrishtiImport 导入显微 CT TIFF 时出现严重卡死：用户反馈即使只导入约 10 张图片也可能无响应，严重时连 Windows 资源管理器一起卡死，只能重启机器。

用户希望覆盖的设备是近五年发布的 Intel/AMD 核显平台。准确目标是 **CPU + 核显可用，同时保留独显加速**，不是纯 CPU 软件渲染版、不是强制所有设备只用核显，也不是删除独显路径。

本次开发/审计机器不是发生故障的笔记本。当前机器是 i9-14900K、Intel UHD Graphics 770、两张 RTX 3090 和约 64 GiB RAM。强制创建 Intel UHD 770 OpenGL 上下文只能证明桌面 Intel 核显路径可建立，不能等价模拟 i7-13700H 的共享内存压力、驱动、功耗限制和整条 GUI 工作流。最终结论必须回到目标笔记本验证，并补一台近五年 AMD 核显设备。

### 0.2 用户真实工作方式和最终成功定义

用户实际需要的不是“导入后能看到预览”这一单点能力，而是下面的生产流程：

1. 在 DrishtiImport 中把连续 TIFF 切片作为一个三维体积导入。
2. 使用左侧范围滑块选择连续 Z 切片范围。
3. 在中间预览中拖动图像边界，裁出目标组织的 X/Y 区域。
4. 通过 Save As/Export 导出这个 ROI 的连续三维体积。
5. 在 DrishtiPaint 中打开导出的 PVL，进行标注、分割和 mask 编辑。
6. 保存 mask、标签名称和三方向 curves，关闭后重新打开仍应一致。
7. 最后在 Drishti 中加载同一体积和处理结果进行渲染；Mesh 入口用于网格相关任务。

用户后来发现导出结果变成“一张输入图对应一套输出”，且这些输出在 Paint 中打不开。这使任务从“解决 Import 在核显机器上的卡死”升级为 **完整数据链路兼容性修复**。四个 exe 能启动、TIFF 插件能解码、或某个 smoke 通过，都不能单独算任务完成。

正常的连续体积输出应是一份逻辑体积：

```text
volume.pvl.nc
volume.pvl.nc.001
[volume.pvl.nc.002 ...]
```

每张 TIFF 分别得到 PVL，通常说明走了 Time Series；逐张得到 TIFF/RAW，通常说明走了 Save Images。这两个入口都不能冒充给 Paint 使用的普通三维 Save As。

### 0.3 产品与便携包目标

期望版本的职责边界如下：

- TIFF/DICOM/RAW 解码、直方图、ROI 裁剪、PVL 写盘、mask 压缩和多数构建工作由 CPU/磁盘流式完成，不依赖独显。
- 交互渲染继续使用硬件 OpenGL；Intel/AMD 核显可运行，有独显时仍可使用独显。
- 软件 OpenGL 只能作为诊断或启动兜底，不能被称为“核显硬件适配”。
- 最终 Windows 包应可直接解压运行，不要求目标机器安装 Qt、Visual Studio、vcpkg 或编译依赖。
- 包的核心范围包括 Drishti、Import、Paint、Mesh、声明包含的插件、Qt 运行时、assets、docs 和 Import 所需内置 Python/NumPy。
- Paint/Mesh 的 GrabCut、UNet 等可选脚本若依赖系统 Python、OpenCV、TensorFlow 等大型外部包，必须清楚标为可选能力，不能含混承诺为离线便携核心功能。

用户记忆中的“三个入口”是核心体数据工作流：DrishtiImport、DrishtiPaint 和 Drishti。当前完整构建另外包含 `drishtimesh.exe`，用于加载、查看和处理表面网格/模型；它不是替代 Paint 的第四个体标注阶段。是否把 Mesh 称为可选组件，不影响前三者之间的 TIFF -> PVL -> 标注 -> 渲染主链路必须闭合。

编译工作区需要预留几十 GiB，而最终便携运行包可以小得多，两者并不矛盾。构建空间还包括 Qt/vcpkg/第三方源码和下载缓存、编译器对象文件、静态/导入库、多个测试与隔离构建、staging 和备份；便携包只保留最终 exe、运行 DLL、插件和资源。目标机器解压运行不应携带这些编译依赖，也不应要求重新编译。

### 0.4 协作和交付约束

用户不是该 GitHub 仓库作者，不能假定拥有上游仓库直推权限，但仍可通过 fork/分支提交 Pull Request；发生故障的目标笔记本也不在当前环境。因此本地工作的交付物必须同时包含：

- 可由另一台机器智能体继续实施的源码交接文档；
- 可复现的构建、依赖和打包信息；
- 最终同一构建快照的便携 ZIP、哈希和测试报告；
- 需要在目标 Intel/AMD 核显机器执行的明确验收步骤。

不能只留下当前机器上的口头结论或混用不同时间的二进制。

### 0.5 任务演进时间线

1. **可行性评估**：检查 Drishti 是否可把导入、构建和标注放在 CPU 上完成，并让渲染兼容近年 Intel/AMD 核显；结论是可行，但完整 Paint 大体积编辑受内存架构限制，不能靠一个 OpenGL 开关解决。
2. **外部修改包比较**：用户提供另一台机器智能体修改的 `drishti.zip` 作为参考。该包属于另一修改/编译快照，只能用于比较，不能与本地后续全量构建的测试证据混用。
3. **本地完整构建准备**：在 D 盘下载和准备 Qt、vcpkg 及项目依赖，随后构建完整程序和插件，而不仅是单个修改 DLL。构建依赖占用远大于最终 ZIP，是因为包含源码、包缓存、中间文件、调试/发布对象和 staging。
4. **真实卡死数据引入**：用户提供 `Raw Tiffs-Living Ant-1_0.4X-LE1-40kv-2w-3s-11.998-1601-DR.zip`，要求在当前双独显机器上显式检查 Intel 核显路径。该数据族随后用于 10/100/1024 张 TIFF 的读取和分片测试。
5. **第一轮便携包迭代**：生成过 `drishti-cpu-igpu-release-2026-08-11-importfix.zip` 等历史包，随后围绕缺 DLL、Qt/Python 运行时、scripts/assets、README、插件和四个入口继续补包。这些都是中间产物，不是当前发布候选。
6. **orientation=4 真实回归**：另一台机器用 100 张 Living Ant TIFF 测出早期 `tiffplugin.dll` 只接受 orientation 1，错误拒绝合法的 orientation 4。随后放宽到接受 1/4，并加入真实数据和方向 smoke；同时审计发现混合 1/4 尚无明确语义，仍需修复。
7. **目标笔记本启动与包布局问题**：目标核显机器曾反馈 Import 打不开或提示 `No scripts found under ...`，暴露出早期包的 assets/scripts 路径或内容不完整。后续构建虽补充了脚本和内置 Python，但尚未从当前源码快照生成并在净目标机器验证最终 ZIP。
8. **审计测试弹窗干扰**：排查期间多个 smoke 在用户可见桌面运行并弹出 Qt 平台插件错误，容易被误认为产品错误。最近的 `volume_file_transaction_smoke` 已改为启动前定位 `qoffscreen.dll`，两套构建副本在清空 Qt 插件环境变量后均通过且无残留进程。
9. **工作流问题升级**：用户展示 Import 的 Z 范围和 X/Y 裁剪操作，指出导出被拆成多份且 Paint 无法打开，并要求不只修这一点，而是全面排查整条链路。于是当前阶段转为对 Import -> PVL -> Paint -> Drishti -> 便携包的证据分级审计。

### 0.6 真实数据、历史包和快照不能混用

本任务出现过多个数据子集和二进制快照：

- 外部反馈使用 Living Ant 100 张子集，证明 orientation 4 被早期插件错误拒绝。
- 本机审计使用同一数据族的 10/100/1024 张路径；1024 张测试证明当前指定快照可解码和写 VFM 分片，但不是正式 GUI Save As。
- Living Ant 文件名中的 `1601` 不能自动证明本机已经对 1601 张完成闭环；只能按每条日志实际记录的切片数表述。
- `drishti-cpu-igpu-release-2026-08-11-importfix.zip`、`portablefix4.zip`、当前共享 bin 和隔离源码构建是不同快照。
- 旧日志生成后又有 exe/DLL 被重编译，因此旧测试不能给当前二进制或未来 ZIP 背书。

本文后续所有“已通过”都限定到明确数据、入口和快照。没有明确对应关系时，一律按未验证处理。

### 0.7 当前阶段边界

整个长期任务此前已经产生大量 CPU/iGPU、插件、导入安全、内存准入、打包和测试相关修改，当前工作树因此非常脏。本文最后一次“全链路审计阶段”只做运行排查、静态审计、测试 Harness 修正和交接整理，**没有完成后续 P0/P1 业务修复，也没有生成新的发布 ZIP**。

上句中的“后续 P0/P1 业务修复”只指本文第 5、6、9 节在本轮新确认的端到端缺陷，不表示此前 CPU/iGPU 加固尚未编码。当前工作树已经存在 TIFF/Image Stack 安全导入、内存与 Commit 准入、底层 VFM I/O、部分事务、Undo/Checkpoint、OpenGL/iGPU 和 CPU mesh paint 等大量实现；它们必须按第 4 节继承矩阵逐项保留、复核或继续修改，不能由后续智能体从零重写，也不能仅凭历史交接宣称通过。

当前可以确认：

- 最近的 `volume_file_transaction_smoke` Qt 弹窗已经修复并复测。
- TIFF orientation=4 和当前指定快照的真实数据解码已有部件级证据。
- Intel UHD 770 Desktop OpenGL 4.5 上下文已有证据。
- Import 正式 PVL -> Paint、Paint 保存重开、Drishti 最终消费仍有确认缺陷或缺少正式闭环。
- 旧便携 ZIP 不能继续作为发布候选。

## 1. 结论和口径

### 1.1 排查是否完成

截至本次交接，**修复前静态问题清单和本机部件级运行证据已经形成可实施初版，但正式 GUI 运行时全链路排查没有完成**。已经结合当前二进制 smoke、当前源码隔离构建、隐藏 Windows Desktop 启动、确定性 Harness、真实 Living Ant 数据和静态调用链盘点问题；这些证据足以开始修复已确认 P0，但不足以宣称所有运行时问题已经发现。

这不等于“全链路运行通过”，也不等于“可以发布”。以下环节仍没有形成正式端到端通过证据：

- 正式 GUI `Save As -> Raw2Pvl::savePvl()`；
- 正式 ROI/Z 裁剪、取消、磁盘满、只读和覆盖场景；
- Paint GUI 加载 Import 输出、标注、Save Work、关闭、重开及逐项校验；
- Drishti 的最终渲染正确性、失败后旧状态保持和干净退出；
- 从当前同一源码快照生成的新 stage/ZIP，在净环境和目标硬件上的验证。

这些未执行项已经逐项登记在第 7、8 节。其中正式 GUI Save As、Paint 保存重开和 Drishti 最终消费属于本机尚未执行的排查缺口，不是外部环境不可达；它们可能继续发现新的修改项。后续智能体可以先修复已经确认的阻断项，但不得把当前清单称为绝对穷尽；每次正式链路暴露新问题时，必须先追加本文，再实施修复和回归。

### 1.2 最初卡死的归因状态

原始故障发生在 `drishtiimport` 选择/读取 TIFF 的 CPU、文件 I/O 和内存链路。`tools/import/import.pro` 不启用 Qt OpenGL，图片导入阶段不创建 OpenGL 上下文，因此可以排除“Intel 核显 shader 性能直接导致 TIFF 导入卡死”；CPU + 核显适配仍然重要，但它解决的是三套显示程序的 OpenGL 兼容性和共享内存预算，不是 Import 解码本身的图形依赖。

| 判断 | 当前状态 | 依据和限制 |
|---|---|---|
| 上游基线在 GUI 线程同步解码/统计，长 codec 或 I/O 会令窗口长期无响应 | 已确认风险链路 | 当前调用链和历史基线源码 |
| TIFF 布局、边界、返回值和旧像素查询资源管理存在安全缺陷 | 已确认历史缺陷，当前已有多项加固 | 当前工作树必须继续复核，不能退回上游行为 |
| 后来实验性 `BATCH_SIZE=32` 可造成大切片批量预分配和 Commit 风暴 | 已确认实验补丁风险，但不是已证现场首因 | 上游基线 `b53bd97` 不含该实现；该实验已撤销 |
| “Explorer 一起卡死”的唯一现场机制 | 外部未确认 | 缺目标笔记本当时 exe/plugin 哈希、RAM/页面文件、Commit/Hard Fault、磁盘曲线和转储 |
| 当前加固已解决原始目标笔记本问题 | 未验证 | 必须在 i7-13700H 笔记本用原始数据和同一正式构建复测 |

因此后续 PR 必须区分三种声明：确认修复的源码缺陷、针对尚未锁定现场首因的防护、目标硬件尚未验证。不得写成“已经找到并修复唯一根因”。`DRISHTI_IMPORT_FREEZE_HANDOFF.md` 保存根因审计细节，`CPU_IGPU_IMPLEMENTATION_HANDOFF.md` 保存此前实现历史；若其描述与当前源码或本文冲突，以当前源码和可复核运行证据为准。

### 1.3 五类状态的严格定义

| 状态 | 含义 | 能否作为发布通过证据 |
|---|---|---|
| 已通过 | 对指定快照、数据、入口和断言实际运行成功 | 只能证明该行所述范围 |
| 确认缺陷/需要修改 | 由运行结果或当前源码调用链确认 | 否，必须修复并回归 |
| Harness/环境阻断 | 审计程序、交互入口或本机条件使结果无法归因 | 否，不能写成产品成功或产品失败 |
| 本机尚未执行 | 当前机器理论可执行，但本轮没有完成正式操作或自动化 | 否；不能伪装成环境不可达，修复后必须补测 |
| 外部未测试 | 当前没有目标硬件、净包或故障条件 | 否，必须在相应外部环境验收 |

### 1.4 证据优先级

发生矛盾时按以下顺序裁决：

1. 当前工作树源码、同一源码快照构建出的二进制和可复核运行日志；
2. 本文记录的当前全链路静态/运行审计；
3. `CPU_IGPU_IMPLEMENTATION_HANDOFF.md` 的历史实施声明；
4. `DRISHTI_IMPORT_FREEZE_HANDOFF.md` 的修复前根因审计；
5. 旧 ZIP、口头结论和没有快照关联的日志。

历史文档中的“已完成编码”不是当前通过证据。例如历史文档曾笼统声称部分 tag 保存错误已传播，但当前源码中 `VolumeMask::saveTagNames()` 仍返回 `void`；此类冲突必须按当前源码登记为待修改。

### 1.5 发布判断

当前构建 **不能发布**，旧 ZIP 也不能继续作为当前源码的发布候选。原因不只两个：PVL 契约、Paint 内存误拒绝、保存错误传播、候选加载破坏旧工作集、Import 总事务、路径与分片校验、便携布局和正式 GUI 闭环均未关闭。

这里的“本轮”专指最后一次全链路审计阶段。整个长期任务此前已有大量业务源码、构建和打包修改；但本文新发现的后续 P0/P1 问题尚未实施。审计阶段只修正了测试 Harness 的 Qt 平台插件启动问题，没有据此生成新发布包。

## 2. 审计快照与方法

### 2.1 仓库和工作区

- 仓库分支：`master`
- HEAD：`b53bd9790829dbeb38cbe0f160e79a95349905d2`
- 2026-08-13 复核时，`git status --porcelain=v1 --untracked-files=all` 共 1399 条：210 个已跟踪改动（207 `M`、3 `D`）和 1189 个未跟踪路径；它们视为用户现有内容，本轮未回退。
- 本文记录的是一个脏工作树的当前状态，不可只用 HEAD 复现。
- 当前状态不能整体暂存、整体提交或直接作为 PR；第 11 节规定了必须先做的 provenance 和干净基线移植。

### 2.2 二进制快照边界

以下快照不同，不能互相背书：

| 快照 | 位置/哈希 | 允许支持的结论 |
|---|---|---|
| 当前共享 bin | `D:\drishti-deps\build\drishti-release\bin` | 当前插件 smoke、PE/依赖与文件布局审计 |
| 当前 bin `drishti.exe` | `B707568B1CBB7FF3DC8DED0B774BB7D868924CB3ECC5DF44FB88FF229C4FD19C` | 仅当前 bin 对应检查 |
| 当前 bin `drishtiimport.exe` | `CDF75145D1A32E105FE7030CD168986B0EA23C0B4F9A44C0989C8BC82FDAC45F` | 仅当前 bin 对应检查 |
| 当前 bin `drishtipaint.exe` | `14880CDC7F6F020E3DC191D0A48F026FA3D071FAB673BA895A02DE52504D8D5D` | 仅当前 bin 对应检查 |
| 当前 bin `drishtimesh.exe` | `558BDA302C979C1B69F83DFE945FD13E343451B8976060FA23E5B502E7ABBC73` | 仅当前 bin 对应检查 |
| 当前 bin `tiffplugin.dll` | `5D53929AE3BE26A259ED8EC1C62229A3B9B71D843D131CC397262884A0E6F404` | 当前 TIFF 运行审计 |
| 隔离当前源码 Import | `D:\drishti-deps\build\runtime-audit-current\import-current-bin\drishtiimport.exe`，`02E2A40678796FD3BCB3918D1621626BE9B982A16AC2B4227EA4608794EACA12` | 隔离启动及与源码一致的静态/部件验证 |
| 隔离当前源码 Drishti | `D:\drishti-deps\build\runtime-audit-20260812\drishti-current-bin\drishti.exe`，`94390B1F93B52223A35C8968C75410A0D80A90E1FDB4F6660079539E8F2A20F6` | 隐藏桌面 Drishti 加载审计 |
| PVL 加载 Harness | `pvl_load_audit.exe`，`8664AC0B69A35BA8926CEA263415229FC235FCB34A62958863CBA3A08EC995E3` | 只按日志中 15 条断言逐项判读 |
| 隐藏 Desktop 启动器 | `hidden_desktop_launcher.exe`，`41C9241D0078DEE1A9314F2D1F237B484FA580E261E9D71B431CEC82839CDE17` | 只证明观察期存活及是否出现 `Error` 标题窗口 |
| 旧 ZIP | `D:\drishti-deps\package\drishti-cpu-igpu-release-2026-08-11-portablefix4.zip`，`9169D2F0E0341106C624EA92BA06214152CC8ADEE523CE3B422B69FF60BD6A7C` | 只能说明历史包，不代表当前源码/bin |

关键日志位于：

- `D:\drishti-deps\build\runtime-audit-20260812`
- `D:\drishti-deps\build\runtime-audit-current`
- `D:\drishti-deps\build\smoke-audited-final-20260812`

### 2.3 本机环境

- Windows 11 `10.0.26100` x64
- Intel Core i9-14900K
- 约 64 GiB RAM
- Intel UHD Graphics 770，驱动 `32.0.101.6129`
- NVIDIA RTX 3090 两张
- 本机不是目标 i7-13700H 笔记本，且无 AMD 核显。

### 2.4 审计方法及限制

- smoke/Harness 验证部件契约、故障注入和确定性数据；它们不自动等价于正式 GUI 工作流。
- GUI 只采用隐藏 Windows Desktop 中真实 `windows` 平台插件的启动结果；限时存活后由 Harness 强制回收不等于干净退出。
- 静态结论用于定位明确调用链缺陷，不冒充运行通过。
- 隔离 Drishti 的 `qt.conf` 实际仍指向共享 `drishti-release\bin`，所以可执行文件来自当前源码隔离构建，但运行依赖并未完全隔离。
- 隔离 Drishti 构建日志没有最终链接成功摘要，审计目录也没有保存完整编排脚本；可复现性不足本身按 Harness 限制记录。
- 当前所有可见桌面 GUI 测试已停止。

### 2.5 文档职责和修改智能体的读取顺序

本文是下一阶段改代码智能体的主问题基线，历史交接只用于追溯设计原因。实施前按以下顺序工作：

1. 先读第 0、1 节，固定真实目标、完成口径和证据优先级；
2. 按第 4 节确认已有实现，不得用上游旧代码覆盖已存在的安全加固；
3. 按第 5、6、9 节处理确认缺陷，并把第 7、8 节当作未关闭风险；
4. 按第 10 节进行实现和回归；
5. 按第 11 节从干净基线形成可审查提交，不得直接提交当前脏工作树。

任何修改都必须留下四项映射：问题编号、修改文件/函数、回归用例、通过的源码/二进制快照。历史文档没有编号映射的“已完成”描述不能直接作为关闭依据。

## 3. 总览矩阵

| 链路段 | 状态 | 当前证据 | 仍需处理 |
|---|---|---|---|
| Import 插件装载/lifecycle | 已通过 | 当前 16 个插件 empty-input + lifecycle `32/32` | 正式 UI 动态插件选择仍随最终包验收 |
| 主要 Import 数据格式 | 已通过 | DICOM、ImageStack、JP2、RAW、RAW slices/slabs、NIfTI、NRRD、TXM、Analyze、GRD、MetaImage、NC4、TOM、VGI、TIFF | 只覆盖各 Harness 定义的数据路径 |
| Living Ant TIFF 读取 | 已通过 | 10/100/1024 张；1024 张 `1024^3`、1,073,741,824 体素 | 正式 GUI 入口和非方形方向夹具未闭合 |
| TIFF -> VFM 分片 | 已通过（手工桥接） | 1024 层写 28 slabs，内容哈希一致 | 不是正式 `Raw2Pvl::savePvl()`；28 slabs 是 Harness 的 `slabSize=37` |
| Import 正式 ROI/Save As | Harness/交互阻断 | 源码调用链已审计 | 必须在修复后跑正式 GUI 与故障注入 |
| Paint 低层 mask/undo/VFM | 已通过 | mask import、slice order、事务、8/16-bit undo、VFM lifecycle | 不代表完整 Paint GUI 工作流 |
| Paint 完整保存/重开 | Harness 阻断且有确认缺陷 | 半集成 Harness 到第二次析构前完成一部分 | 先修 PVL/保存/加载状态，再跑正式闭环 |
| Drishti PVL 解析/加载 | 部分通过且有确认缺陷 | fallback、Unicode 无空格、缺失/截断拒绝；隐藏桌面启动 | 空格、重复/长短列表、头一致性、旧状态保护需修 |
| Intel 桌面核显 OpenGL | 已通过 | UHD 770 OpenGL 4.5 Compatibility | 仅本机上下文，不是目标笔记本闭环 |
| AMD/目标 Intel 硬件 | 外部未测试 | 无对应机器 | 必须在目标设备运行完整链路 |
| 当前 bin PE 依赖 | 已通过（静态闭包） | 187 个 PE、1841 依赖边、全部 x64、非系统缺失 0、Qt6 0 | 动态实例化及净 PATH 仍未测 |
| 最终便携 ZIP | 外部未测试/尚不存在 | 只有不同快照的旧 ZIP | 修复后从同一通过快照重新打包验收 |

### 3.1 原目标的三条独立判定线

| 判定线 | 当前结论 | 进入发布前必须补齐 |
|---|---|---|
| 原始 Import/Explorer 卡死 | 风险链路已确认，唯一现场首因未锁定 | 目标笔记本原数据、同一正式构建、响应性与 Commit/Hard Fault/磁盘曲线 |
| CPU + Intel/AMD 核显适配 | 本机 Intel UHD 770 上下文和若干部件已通过，完整适配未验证 | 三套 OpenGL 程序在目标 Intel 和至少一台 AMD iGPU 上执行 shader/FBO/交互工作流 |
| Import -> Paint -> Drishti 数据链路 | 已发现多个 P0，正式 GUI 链路未通过 | 修复后用同一 PVL 完成保存、标注重开、渲染和正常退出 |

三条判定线相互关联但不能互相背书。Import 解码通过不证明 OpenGL 适配，OpenGL 上下文创建不证明数据链路，手工 VFM 分片也不证明正式 Save As。

## 4. 已实际运行通过的范围

### 4.1 Import 和真实 TIFF

- 当前 16 个 Import 插件全部完成 empty-input/reload 和 lifecycle smoke，共 `32/32` 通过。
- 主要数据路径通过：DICOM、ImageStack、JP2、原生 RAW、RAW slices、RAW slabs、NIfTI、NRRD、TXM、Analyze、GRD、MetaImage、NC4、TOM、VGI、TIFF。
- Portable Python/NumPy runtime、Import RAW/NumPy scripts、Unicode VDB 路径、TIFF 输入路由、插件返回值校验、直方图和值映射通过。
- TIFF 目录入口与显式文件列表入口均对 Living Ant 前 10 张和前 100 张运行；同一组输入内容哈希一致。
- Living Ant 1024 张全部解码通过：
  - 尺寸：`1024 x 1024 x 1024`
  - 体素数：`1,073,741,824`
  - 当前插件存储扫描线顺序 SHA-256：`6617f52f316bb796ceb187f3983614a8a5eb5336e2ad55f876e773c4088b5be2`
- 手工桥接当前 TIFF 插件和 Import `VolumeFileManager`，以 Harness 指定 `slabSize=37` 写出 28 个 slabs；回读内容哈希同为 `6617f52f...b5be2`。
- 单组 `VolumeFileManager` 创建、提交、回滚、备份保留/清理故障注入通过。

重要限制：正式 `Raw2Pvl::savePvl()` 在保存对话框中把 `slabSize` 强制设为最终深度，通常只写一个 slab；28 slabs 只证明 VFM 数据桥接，不能写成正式 Save As 产物。

### 4.2 Living Ant 哈希语义纠正

旧日志/旧文档中的：

```text
ceb1e998c616a73c743a53b64a1a6fd7e01be4103762dff872d844e54a7c3cb4
```

精确对应“每张二维 TIFF 做 X/Y 转置后”的数据序列。当前插件保留 TIFF 存储扫描线顺序，正确哈希是：

```text
6617f52f316bb796ceb187f3983614a8a5eb5336e2ad55f876e773c4088b5be2
```

这不是随机损坏，而是坐标/存储契约发生变化。Living Ant 切片为方形，无法单独揭示 X/Y 是否转置；必须新增非方形、已知坐标值夹具，并明确 `Z/Y/X`、ROI 和显示方向契约。

### 4.3 Paint、Mesh 和共享部件

以下低层 smoke 已通过：

- mask import 与切片顺序；
- slab 保存事务和崩溃恢复模型；
- 8-bit/16-bit mask undo，及损坏 undo 拒绝；
- Paint `VolumeFileManager` 生命周期；
- Volume operations、GraphCut、ITK 的内存准入；
- framebuffer 预算；
- MeshTools I/O、binary PLY writer、video encoder。

这些结果只证明相应部件，不证明 Paint 的 GUI 标注、tag names/curves 和关闭重开闭环。

### 4.4 Drishti PVL 加载

确定性 PVL Harness 的逐项日志确认：

- 缺 `<pvlnames>` 时，按头文件 base 推导 `.001/.002...` 的 fallback 可读；
- 显式 Unicode 且文件名无空格可读；
- 缺失 slab 和截断 slab 被拒绝。

隐藏 Desktop 中，隔离当前源码 Drishti：

- 正常显式 PVL 进入主窗口并保持运行；
- 无 `<pvlnames>` fallback PVL 进入主窗口并保持运行；
- Unicode 无空格显式名进入主窗口；
- 缺 slab 和显式空格路径出现 `Error` 窗口。

这些进程均由 Harness 限时强制回收，故未证明干净退出或渲染像素正确。

### 4.5 GUI 启动、OpenGL 和依赖

- 独立隐藏 Windows Desktop 中，Import、Drishti、Paint、Mesh 都能创建主窗口并存活到审计超时。
- Intel UHD 770 创建 Desktop OpenGL 4.5 Compatibility Context 成功；renderer 明确为 Intel UHD 770。
- 当前 bin 递归静态审计：187 个 PE、1841 条依赖边，全部 x64，未解析非系统依赖 0，Qt6 依赖 0。
- 当前 bin 检出 16 个 Import 插件、6 个 Render 插件和 1 个 MOP 插件。

### 4.6 既有 CPU/iGPU 实现继承矩阵

下表描述当前工作树已经存在、后续修改必须继承的实现。状态“已有编码”只表示源码中存在相应路径，不等于目标硬件或正式 GUI 已通过。

| 区域 | 当前工作树已有编码 | 本轮仍需修改或验证 |
|---|---|---|
| TIFF 插件 | `TIFFOpenW`、全栈元数据/布局预检、受检 64 位容量、逐 scanline 解码、初始统计使用单个后台 worker、`rawValue()` 行级读取、错误传播 | 全栈 IFD 枚举、正式 Save/preview/orthogonal/`rawValue()` codec 调用仍在 GUI 调用线程；Save As 只能切片间取消；单次 codec 无硬超时；orientation 见 P1-I4 |
| Image Stack | `QImageReader::size()` 预检、实时内存/Commit 准入、受检 RGB/RGBA 像素契约、RAII、取消与错误传播 | 正式 GUI 与恶意/极慢 codec；目录/显式排序和方向契约；入口误用提示 |
| Import 内存 | `ImportMemoryAdmission`、受检峰值、物理内存和 Windows Commit 双门控、系统与 iGPU 余量、危险阶段分配前拒绝 | 准入是时点估算而非配额；并发任务、OpenVDB/Qt/驱动内部占用仍只能保守估算；目标 8/16/32 GiB 验证 |
| Import 保存 | 单个 VFM 管理的同一 base 全组 slabs 具有进程内 staging、backup/rollback、exact I/O 和取消传播；batch/merge/MHD 已有持久 `BatchPathJournal`；header 使用 `QSaveFile`；多个调用点检查错误 | XML/PVL/RAW/Time Series 仍未统一为一个全局跨对象 journal；VFM/磁盘预检、旧尾 slabs 和正式 Save As 仍需故障注入与 GUI 验收 |
| Drishti 底层 I/O | VFM 受检容量/I/O、`VolumeBase` 阶段错误传播、候选对象内部回滚、加载失败可退到 `DummyVolume` | 应用层 `preLoadVolume()` 仍先清旧场景，不能保留用户原工作集；时间序列和共享 manifest 见 P0-P4/P0-P7/P0-P15 |
| Paint mask/VFM | 受检 VFM、压缩 mask 单 worker、350 ms debounce、generation 合并、不可变 snapshot、dirty 保留、多 slab journal、Undo、Checkpoint | GUI 线程仍同步写整卷原始 snapshot；临时 snapshot 清扫、tag names/curves/Save Work 总事务和正式重开未闭合 |
| Paint 算法内存 | 实时物理/Commit 模型、GraphCut、六个 ITK profile 和八个原生三维入口的部分准入/回滚 | 固定 `LargeVolume` 门槛仍误拒绝；GraphCut/多项算法仍同步阻塞；LiveWire、其余旧算法、正式 ITK 构建/运行和目标机峰值未闭合 |
| OpenGL/iGPU | Desktop OpenGL Compatibility profile、renderer 状态、shader/FBO 错误传播、延迟资源、固定 slab、两套 `ScopedTrisetGlState`、CPU mesh paint；Highres shadow、LightHandler、Drishti Viewer、Mesh Viewer 和 RcViewer 已有 candidate/旧资源并存预算及 GL 状态恢复 | 预算/失败路径尚未真实注入；目标 Intel/AMD 驱动上的全部延迟 shader/FBO 和画面验证仍未闭合 |
| CPU mesh paint | 活动路径已用 `CpuMeshPaint`，不依赖 compute shader；VBO 布局和部分失败路径已加固 | 每次画笔仍可能扫描全部顶点；ridged 图案与旧 GPU simplex noise 不等价；大网格性能与 VBO 失败后状态一致性需验证 |
| 打包/构建 | qmake/依赖路径和历史 stage 已有大量修改，当前 bin 静态 PE 闭包通过 | 当前快照没有新 ZIP；`qt.conf`、help、assets、动态插件、净 PATH 和最终 notices 仍需同一快照验收 |

### 4.7 既有实现仍需登记的风险

以下风险不应因第 5、6 节聚焦主数据链路而从 PR 基线消失。它们可以拆到后续依赖 PR，但必须有明确 owner 和关闭条件。

### P1-H1：极慢 TIFF IFD 枚举和单次 codec 调用没有硬超时

只有初始 histogram/statistics 使用单个后台 worker。Standard Image 自动路由和 TIFF plugin `setFile()` 会同步枚举全栈 IFD；正式 Save、preview、orthogonal 和 `rawValue()` 解码仍在 GUI 调用线程。Save As 的 `getDepthSlice()` 没有 codec 内 cancel flag，只能在外层切片之间取消；单次 `TIFFReadScanline()` 也不能安全强杀。需要去重路由/插件的重复枚举，把 IFD 与正式转换移出 GUI 线程并增加阶段进度；若产品要求对损坏 codec 提供硬超时，应使用可回收 worker process，而不是在线程内强制终止。不得把“初始统计后台化”外推成“正式 Save 已后台化”。

### P1-H2：内存准入不是全局配额

Import/Paint 的物理内存与 Commit 检查是操作开始时的快照。多个任务可以同时获批，OpenVDB、Qt、OpenGL 驱动 staging 和其他进程随后仍会争用余量。需要为本进程危险任务建立共享 reservation/释放模型，或禁止高峰任务并发；所有拒绝必须发生在大分配和输出创建之前。

### P1-H3：Paint 长算法覆盖和取消仍不完整

GraphCut、LiveWire、部分形态学及旧整卷算法仍可能在 GUI 线程长时间运行；不是所有入口都有同等级峰值模型、算法内取消和失败后字节级回滚。需要建立入口清单，逐个记录峰值、准入位置、取消粒度和事务边界；ITK/VED 的 profile smoke 不能替代正式插件构建与运行。

### P1-H4：mask snapshot 仍会同步写整卷并遗留临时文件

压缩工作已在 worker 中，当前过时表述“压缩在 GUI 线程”不得继续使用；真正同步的是 GUI 线程把整卷原始 mask 分段写入 `.drishti-mask-snapshot-*.tmp`。其停顿随 mask 与磁盘速度增长，异常退出/删除失败可遗留临时文件。需要演进为 dirty-chunk/分块不可变快照，并在安全时清理可识别的孤儿临时文件。

### P1-H5：CPU mesh paint 的性能和语义等价性未关闭

当前活动画笔不再要求 compute shader，但每次事件仍可能 `O(total vertices)` 扫描；ridged 分支使用 value noise，与旧 GPU 的 3D simplex `snoise()` 不逐语义等价。需要大网格交互基准和 golden corpus；若接受视觉变化，必须在 PR 中明确说明，不能称为透明 CPU fallback。

### P2-H6：遗留库仍有进程级退出路径

当前树的多份旧 PLY C 实现和 netCDF v2 兼容代码仍包含 `exit()`；这些边缘路径一旦由用户输入触发可能终止整个进程。需要确认实际可达性，将可达路径改成错误返回/异常边界，并为损坏输入增加负向测试；不可达副本应从构建图移除或明确隔离。

### P1-H7：OpenGL 状态守卫覆盖已扩展，但预算和实机失败矩阵未闭合

当前 Drishti/Mesh 两套 `ScopedTrisetGlState` 已覆盖 draw/read FBO、RBO、VAO/VBO、active texture、0-7 及当前 texture unit 的 2D/rectangle binding/enable、viewport/program、blend/depth/cull、write mask/function/polygon，并能映射被替换对象；历史交接所说这些状态“尚未恢复”已经过时。仍需检查 fallback 涉及的 `GL_LIGHTING`、Mesh line mode 的 `GL_LINE_SMOOTH/line width`，并用非零哨兵验证成功、预算拒绝和 GL allocation 失败三条路径。当前统一 FBO 预算证据主要覆盖 Drishti/Mesh Trisets 与 Paint preview；Drishti Viewer image/save、Mesh Viewer、`drawhiresvolume`、`lighthandler`、`prunehandler` 等分配点仍需逐项盘点和目标核显验证。

## 5. 确认缺陷/需要修改：Import 与导出

### P0-I1：正式 PVL 输出与 Paint 输入契约不一致

`tools/import/raw2pvl.cpp:1035` 的 `Raw2Pvl::savePvlHeader()` 不写 `<pvlnames>`；Paint 主加载和另一体提取却要求列表非空。Drishti fallback Harness 接受了确定性无 `<pvlnames>` 样本，而 Paint 会拒绝同类契约；正式 Import GUI 输出尚未由 Drishti/Paint 完整打开，不能把 Harness 外推成正式链路通过。

需要修改：建立共享的 PVL manifest/分片解析契约；缺 `<pvlnames>` 时按 `gridsize + slabsize + header` 推导并完整校验；旧式显式列表保持兼容；新显式格式不得再用空格拼接文本。

### P0-I2：Save As 的 XML、PVL slabs 和可选 RAW slabs 不是总事务

当前 VFM、单个 header 和若干输出调用点已经有临时文件、backup/rollback、`QSaveFile` 和短写检查；缺陷不是“完全没有事务”。问题是这些局部事务没有组成覆盖 XML header、全部 PVL slabs、可选 RAW slabs 和批次 manifest 的单一原子提交。分别提交时，任一步失败仍可能留下新头/旧数据、PVL 已提交/RAW 未提交，或半个 Time Series 批次。

需要修改：以一个 manifest 和 staging 目录覆盖头、PVL slabs、RAW slabs；先完整写入、flush、尺寸/哈希校验，最后原子切换头；失败统一回滚，并报告可恢复备份。

### P0-I3：没有磁盘空间预检，创建阶段先全量写零再覆盖

`VolumeFileManager::createFile()` 先写完整零数据，之后转换循环再次写真实数据；实际 I/O 约为数据量两倍，并且还可能同时需要 staging/backup 空间。

需要修改：保存前按最终体积、slabs、RAW 可选项、staging 和 backup 计算最坏空间；检查每个目标卷可用空间；优先使用稀疏/定长或单遍 staging 写入。

### P1-I4：TIFF orientation 1/4 混合堆栈被接受

校验允许 orientation 1 和 4，但布局一致性没有比较 orientation，读取也不进行方向归一化。因此混合 1/4 会被当作同一堆栈。

需要修改：先定义物理方向契约。最低限度应拒绝混合 orientation 并列出首个不一致文件；若做归一化，必须同步定义 Z/Y/X、ROI 和 Paint/Drishti 显示方向。

### P1-I5：目录与显式多选的 Z 顺序不一致

目录入口使用 `QCollator` 自然排序；显式多选保留文件对话框返回顺序，且 UI 没有让用户确认/调整完整 Z 顺序。

需要修改：两个入口采用同一自然排序默认值，并在导入前展示可调整列表；记录最终顺序到审计日志/项目元数据。

### P1-I6：非整除降采样静默丢尾

输出尺寸以 `dsz/factor`、`wsz/factor`、`hsz/factor` 整数除法计算，后续循环只覆盖完整块，尾部不足 factor 的 Z/Y/X 被静默丢弃。

需要修改：使用向上取整并按边界块实际样本数归一化，或在 UI 明确阻止不可整除选择；不得静默改变 ROI。

### P1-I7：`No Interpolation` 选择没有形成清晰、独立的算法契约

UI 设置 `filterType=1` 后实际只把 `spread=0`；`filterType` 后续不再读取。无滤波路径在 Z 取块首层，而 XY 循环会让块内最后一个像素覆盖结果，Z/XY 锚点不一致。

需要修改：实现并测试明确的 nearest-neighbor 锚点规则，或删除无效 `filterType`；Z/Y/X 必须采用一致坐标映射。

### P1-I8：16-bit padding 用字节 `memset`

`memset(final_val, pad_value, bytes)` 在 16-bit 输出中把 padding 值 1 写成 `0x0101`（257），首尾 Z padding 和 XY padding 都受影响。

需要修改：按输出元素类型填充并对 8/16-bit、正负边界值做确定性回归。

### P1-I9：保存 RAW + padding 时几何声明不一致

PVL manager/头使用 padding 后的最终尺寸；RAW manager 仍使用未 padding 的 `dsz2/wsz2/hsz2`，但与同一输出头关联。

需要修改：头、PVL 和 RAW 共享同一个最终几何描述，或显式禁止 RAW + padding 组合。

### P1-I10：原点 `1 x 1 x 1` ROI 被 Quick RAW 哨兵误判

`RemapWidget::saveTrimmedResult()` 用 `dmax==0 && wmax==0 && hmax==0` 判断 Quick RAW，合法的原点单体素 ROI 会误走该路径。

需要修改：用显式操作枚举/参数区分 Quick RAW 和普通 Save As。

### P1-I11：ROI 浮点边界直接截断，单体素轴存在除零风险

拖拽框坐标通过浮点乘尺寸后隐式截断，包含端点规则不清晰；部分映射使用 `dimension-1`，单体素轴需要专门保护。

需要修改：定义 floor/ceil、闭/开区间和 clamp 规则；覆盖非零起点、末端、反向拖拽和 1 体素轴。

### P1-I12：覆盖后旧尾 slabs 不清理

当新输出 slab 数少于旧输出时，事务只处理新 manifest 中的文件，旧 `.002/.003...` 可残留并误导后续工具。

需要修改：提交时比较旧/新 manifest，只清理同一目标拥有的多余分片，并确保回滚可恢复。

### P1-I13：Time Series 可半批提交且 basename 会冲突

每个时间点独立保存；后续失败或取消会保留前半批。输出名来自 `completeBaseName()`，不同目录同名输入可覆盖同一目标。

需要修改：预先生成并校验完整输出 manifest、解决/拒绝重名，整批 staging 后提交，或明确提供可恢复的批次状态。

### P2-I14：Save Images 整批非事务

单张使用 `QSaveFile`，但取消/失败会留下已提交的前半批；缩短范围重导也不会清理旧切片。

需要修改：使用批次 staging 目录和 manifest 提交，或在 UI 明确列出并可清理本批输出。

### P2-I15：Time Series/Save Images 容易被误认为普通连续体积 Save As

普通 TIFF 堆栈应生成一套 `volume.pvl.nc + slabs`；Time Series 会逐输入生成体积，Save Images 会逐张输出图像。

需要修改：重命名菜单、对大量同尺寸单页 TIFF 给出入口警告，并在 UI 明确“二维序列不可直接供 Paint 使用”。

## 6. 确认缺陷/需要修改：Paint、Drishti 与共享 PVL

### P0-P1：Paint 主加载、提取和 Python 工具没有统一的 PVL 分片解析器

- `tools/paint/volume.cpp:437`：主加载强制 `<pvlnames>` 非空；
- `tools/paint/drishtipaint.cpp:6550`：另一体提取同样强制非空；
- `tools/paint/pywidget.cpp:30`：显式首个名字仍无条件追加 `.001`，mask 路径也由错误 base 推导。

需要修改：Paint、Drishti、Import 输出和 Python 工具复用同一解析/校验模块，返回规范化绝对路径、预期数量和明确错误。

### P0-P2：Paint 自己生成的 PVL 也可能无法由 Paint 重开

`DrishtiPaint::savePvlHeader()` 和 `StaticFunctions::savePvlHeader()` 不写 `<pvlnames>`，而 Paint 主加载要求非空。因此 Paint 的提取/局部厚度等自产物也存在不可重开契约。

需要修改：所有 PVL writer 与 reader 使用同一规范，并添加“自产物立即关闭重开”回归。

### P0-P3：旧式 `<pvlnames>` 以空格拆分，合法路径被破坏

Paint `StaticFunctions::getPvlNamesFromHeader()` 和 Drishti `XmlHeaderFunctions::getPvlNamesFromHeader()` 对文本 `simplified().split(" ")`。运行探针已证明带空格显式名被拆成多个不存在的文件；Unicode 无空格可读。

需要修改：兼容旧格式时给出明确限制/迁移；新格式使用重复 XML 子元素或可转义 manifest，不能继续空格拼接。

### P0-P4：PVL manifest 与二进制头校验不完整

Drishti 探针确认当前行为包括：

- 重复 slab 路径被接受；
- 显式列表短于预期时混用 fallback；
- 多余显式名字被忽略；
- manager 复用后，无列表头会保留陈旧显式列表；
- XML 几何与 slab 的 13-byte 二进制头不一致仍被接受；
- 未知/缺失 voxel type 默认成 unsigned char；非法 header size 文本变成 0；重复 `<pvlnames>` 取第一项。

需要修改：严格解析必填字段和枚举；根据 `depth/slabsize` 要求精确文件数；拒绝重复规范化路径；逐 slab 校验 type、slice count、width、height、文件长度；每次配置前清空旧列表；拒绝重复关键 XML 元素。

### P0-P5：Paint 固定 `min(RAM/8, 2 GiB)` 大体积阈值在实际预算前误拒绝

`tools/paint/getmemorysize.cpp` 在实际物理内存/Commit 预算之前，只要 resident volume 达到固定阈值就返回 `LargeVolume`。合法且本机可容纳的约 2 GiB 工作集也会被拒绝。

需要修改：删除该无条件门槛；保留整数溢出、地址空间、实际可用物理内存、Commit 和 iGPU/系统余量判断；每个 GraphCut/ITK/Volume operation 继续做峰值准入。完整 `1024 x 1024 x 1601 x 16-bit` 仍可能需要真正 out-of-core mask，不能仅放开限制。

### P0-P6：Paint 候选加载失败前已卸载旧工作集

`DrishtiPaint::setFile()` 先保存/清空当前 mask、curves、TF 和 volume，再验证及加载候选；`Volume::setFile()` 自身也先 `reset()`。坏文件、权限或内存失败会丢失当前上下文，并可能让 viewer/image 静态指针暂时指向已释放内存。

需要修改：两阶段加载到候选对象；完成 XML、manifest、文件、内存、volume、mask 和 histogram 校验后再原子切换 UI 指针和活动状态。

### P0-P7：Drishti 候选加载失败前清空场景和旧体积

`MainWindow::preLoadVolume()` 清空 RawVolume、灯光、几何对象和关键帧，随后才调用 `m_Volume->loadVolume()`；失败恢复会清空体积，而不是恢复旧工作集。`MainWindow::loadProject()` 还有第二层缺陷：它先设置 current project、清空文件列表，调用返回 `void` 的 volume load 包装后不检查成功，随后继续 reset bricks/clip、加载 Lowres/preferences/TF/keyframes，最终仍显示 `Project loaded`。坏项目引用可以把旧状态和新项目状态混合。

需要修改：候选 volume/lowres/项目资源先独立准备，所有加载 API 返回结构化结果，成功后一次提交；失败必须保持 current project、旧体积、几何、TF、关键帧、相机和 UI 状态，并且不得显示成功。

### P0-P8：tag names 保存失败不可见、非原子

`VolumeMask::saveTagNames()` 返回 `void`，忽略 XML 解析、打开、写入和 flush 失败，并直接覆盖 mask PVL 头。

需要修改：返回结构化 `bool/error`，用 `QSaveFile` 原子保存，验证 XML 和完整写入，并向 Save Work/退出传播。

### P0-P9：curves 读写不验证 I/O，损坏输入可无限循环或巨额分配

读取循环不检查 EOF/短读/未知关键字，计数可为负或超大；保存不检查 open/write/flush，无曲线时直接删除旧文件。

需要修改：版本化格式；所有关键字、计数、剩余字节和分配上限严格校验；保存使用临时文件原子提交；清空曲线是显式事务。

### P0-P10：Save Work、换体积和退出只以 mask 结果决定成功

curves 和 tag names 的失败不能传播。Save Work 甚至在 curves 无返回值后只检查 mask `exiting()` 并显示 `Saved`；退出也可在 curves/tag names 未保存时接受关闭。

需要修改：mask、tag names、三方向 curves 和相关头文件组成一个工作保存事务；任何失败都阻止成功提示和退出，并给出逐项错误与恢复位置。

### P1-P11：首次加载强制在源目录旁创建 mask sidecar

Paint 由源 PVL 推导 `.mask.sc/.mask.pvl.nc`，只读源目录、共享介质或无权限目录会失败。

需要修改：加载前检查可写性，支持用户选择独立 workspace/sidecar 根目录，并持久化源体积到工作目录的映射。

### P1-P12：大 mask 初始化和 snapshot 同步 I/O 阻塞 UI

当前压缩本身已经交给单 worker；仍会在 GUI 线程同步执行的是首次创建/内存映射、把整卷原始 mask 分段写入不可变 snapshot，以及 Undo/Checkpoint 的部分完整文件复制。这些步骤会随 mask 大小和磁盘速度长时间阻塞，取消和 I/O 错误反馈仍不足。

需要修改：把大体积 snapshot/初始化演进为可取消的 dirty-chunk 或分块流水线，提供确定进度；取消要回滚 sidecar，不得留下看似有效的半成品；压缩 worker、generation 合并和 dirty 失败保留机制必须保留。

### P1-P13：提取对话框取消语义不完整，长循环取消不足

部分 `QInputDialog::getInt()` 未接收 `ok`，取消会返回默认值并继续；部分长循环的 progress 没有真正 Cancel 路径或检查粒度不足。

需要修改：所有交互结果显式检查 accepted；耗时循环统一 cancellation token 和事务回滚。

### P0-P14：短 `gridsize` 会越界访问

Drishti `XmlHeaderFunctions::getDimensionsFromHeader()` 和 `VolumeInformation::volInfo()` 在未检查元素数量时直接访问 `str[0..2]`。`<gridsize>1</gridsize>` 等畸形头存在越界/崩溃风险。

需要修改：共享严格解析器必须要求恰好三个可表示的正整数，并对缺失、短、长、非数字和溢出字段返回结构化错误。审计期间曾观察到的 `0xC0000005` 没有落盘证据，不作为运行失败结论；本项依据源码静态确认。

### P0-P15：Drishti 时间序列只完整加载首帧，切换先改状态且无回滚

`VolumeSingle::loadVolume()` 只对 `vfiles[0]` 调用完整 `VolumeBase::loadVolume()`。后续时间点切换先写入 `m_volnum`、尺寸和 manager 配置，再由后续纹理读取发现问题；没有预先 `exists()` 和失败回滚。若后续头缺 `<pvlnames>`，`setBasicInformation()` 还会继承上一时间点的显式列表。

需要修改：打开时间序列时预检每个时间点的 schema、manifest、分片和兼容几何；切换时使用候选 manager/texture，成功后提交索引和尺寸，失败保持当前帧。

### P1-P16：`-drishti` 启动参数会污染文件参数

`main.cpp` 只用 `-drishti` 跳过 launcher，没有从 `qApp->arguments()` 移除；`mainwindow.cpp` 只特殊处理 `-stereo`。因此 `-drishti file.pvl.nc` 不加载，`file.pvl.nc -drishti` 会把选项当成第二个时间点。

需要修改：使用正式命令行解析器，消费已识别选项、拒绝未知选项，并为 launcher bypass、stereo、单体积和时间序列组合添加测试。

## 7. Harness/审计环境阻断

### 7.1 Paint 半集成 Harness 未归因终止

半集成 Harness 已执行到：旧式显式 `<pvlnames>`、空格/中文父目录、`3 x 2 x 2` 两分片、自动 mask、16-bit 标签 513、Unicode tag names 保存和一次重开准备。随后第二个 `Volume` 析构时出现 `0xC0000005`。

该异常尚未归因到 Harness 生命周期、静态全局、测试构造方式或产品代码，必须标为 **Harness 阻断**。不能写成“Paint 产品崩溃”，也不能写成“Paint 保存重开通过”。修复后应以正式应用进程中的 GUI 闭环替代该证据。

### 7.2 Drishti 隐藏桌面证据边界

- 只有 5 个非空 launch log 可用于正式表述；后续 6 个 child log 为空且无 launch log，不能据其文件名宣称运行成功或失败。
- PVL Harness 主程序无条件返回 0，结果只能按日志中的每项 `PASS/FAIL` 判读；当前采用的是 15 条 `PASS` 的逐项内容。
- 短 `gridsize` 用例的 Harness 日志只记录“样本创建”，不是加载成功或崩溃测试；曾观察到的 `0xC0000005` 未持久化，不得升级成运行证据。
- Harness 日志中的中文路径出现 `U+FFFD` 替换字符；只能证明磁盘样本/无空格 Unicode 用例的断言，不能从该日志还原精确中文文件名。
- 五个隐藏桌面日志只记录窗口标题，没有 argv、等待时长和错误正文；`0xA11D17` 是启动器强制回收代码，不是 Drishti 崩溃码。
- 隐藏桌面测试均限时强制回收，未验证干净关闭。

### 7.3 本轮可见弹窗是审计环境错误

排查期间用户桌面先后出现过审计程序标题，包括：

- `stream_redirect_smoke`
- `python_script_plugin_smoke`
- `volume_file_transaction_smoke`

这些弹窗的共同信息是 `no Qt platform plugin could be initialized`。它们来自测试进程没有可靠注入 Qt 平台插件路径，不是 DrishtiImport/Paint/Drishti/Mesh 的产品窗口，也不是用户数据错误。

最近一次 `volume_file_transaction_smoke` 已修复：测试会在创建 `QApplication` 前设置 `QT_QPA_PLATFORM=offscreen`，并从测试目录、Drishti 构建目录和 Qt 插件目录定位 `qoffscreen.dll`；找不到时只向控制台返回明确错误。两份历史构建目录的测试程序在清空 `QT_QPA_PLATFORM`、`QT_QPA_PLATFORM_PLUGIN_PATH`、`QT_PLUGIN_PATH` 后均输出：

```text
Volume file transaction smoke passed
EXIT_CODE=0
LEFTOVER_COUNT=0
```

修改位置为 `tools/import/tests/volume_file_transaction_smoke.cpp` 和对应 `.pro`。测试不应进入最终便携包。较早两个测试标题仅作为审计环境事件记录，不能据此声称对应产品路径已通过或失败。

后续 GUI 审计只允许在独立隐藏 Desktop，或在已验证 `qwindows/qoffscreen` 路径的环境运行。不能把这些测试启动失败纳入产品缺陷统计。

### 7.4 本轮未执行的正式交互入口

Import Save As 需要连续多个原生/Qt 对话框，当前没有稳定的自动化控制接口。为避免误操作用户桌面，本轮选择不运行正式保存；这条路径在本机可以人工执行，不能归类为外部环境不可达或技术阻断。应在修复时增加测试入口/依赖注入，使相同业务函数可由确定性 Harness 提供参数，再用少量 GUI 测试验证控件到参数的连线。

## 8. 未完成排查与当前不能声称的内容

### 8.1 本机尚未执行的正式工作流

- 正式 TIFF Stack/Directory 选择到 `Save As` 的完整 UI；
- 非零 X/Y/Z ROI、Z 范围、非整除降采样、padding 和可选 RAW 组合；
- 保存中取消、磁盘满、无权限目录、只读介质、覆盖后分片减少；
- Time Series 重名/半批失败和 Save Images 批次失败；
- Paint 正式加载 Import 无 `<pvlnames>` 输出；
- Paint 真实标注、8/16-bit mask、Unicode tag names、三方向 curves 的 Save Work/关闭/重开逐项一致；
- Drishti 对同一 PVL 的渲染方向、像素/体素正确性、失败后旧状态保持和正常退出；
- Drishti 后续时间点损坏、缺 `<pvlnames>` 或几何不一致时的切换与回滚；
- `-drishti/-stereo` 与单体积/时间序列参数组合；
- 帮助入口实际点击和全部动态插件实例化。

### 8.2 硬件/系统矩阵未测试

- 目标 i7-13700H Intel 核显笔记本的完整 Living Ant 闭环；
- 任一近五年 AMD iGPU 的完整闭环；
- 16/32/64 GiB、页面文件开/关、低 Commit、显存/共享内存压力；
- 仅核显、仅独显、多 GPU 选择与驱动恢复；
- 长路径、网络盘、真实只读介质和磁盘满。

### 8.3 最终包未测试

- 当前源码尚未生成同一快照的新 stage/ZIP；
- 尚未在净 PATH、无开发 Qt/Python/vcpkg 的机器解压运行；
- 尚未测试敌对 `QT_PLUGIN_PATH/QT_QPA_PLATFORM_PLUGIN_PATH/PYTHONHOME/PYTHONPATH`；
- 尚未生成最终 manifest、关键二进制哈希和测试报告。

因此不能声称“全链路已通过”“CPU + Intel/AMD 核显适配完成”“便携包可发布”或“所有插件均可用”。

## 9. 包、帮助与可选依赖问题

### P1-K1：当前 bin 不是完整可证明的便携布局

当前 bin 根目录缺 `qt.conf`。打包脚本会复制仓库根 `qt.conf`，但尚未从当前快照生成新 stage/ZIP 并在净环境验证。

需要修改/验证：从唯一通过的构建产物生成 stage；校验 `assets`、`docs`、`python`、Qt plugins、Import/Render/MOP plugins 和运行库；净环境动态启动四个 exe 和插件。

### P1-K2：外部帮助入口与打包布局契约失配

当前 docs 只有 5 个 PDF，没有 QHC/QCH。Drishti 和 Import 在 Windows 从 exe 上级的 `docs/drishti`、`docs/import` 查找 `drishti.qhc/drishtiimport.qhc`，与当前 bin/打包 PDF 布局不一致。Paint 主帮助使用内嵌资源，不能笼统写成“Paint 帮助缺 QHC”。

需要修改：统一帮助入口和包布局；找不到 QHC 时直接打开对应 PDF 或给出明确离线帮助，避免反复要求用户选目录。

### P2-K3：可选 Python 功能不是全部便携

Import 内置 Python/NumPy 仅覆盖 Import scripts。Paint/Mesh 的可选脚本仍可能从系统 PATH 查 Python；GrabCut 需要 OpenCV，UNet++/UNet3Plus 等需要 TensorFlow、blosc2、Pillow 等大型依赖。

需要修改：UI 和 README 区分内置、可选外部和未打包功能；启动时做能力探测，缺依赖时禁用并给出准确安装说明。

### P2-K4：offscreen smoke 暴露字体目录警告

部分 offscreen 插件 smoke 报 `QFontDatabase: Cannot find font directory .../lib/fonts`。测试本身通过，但该布局依赖应在最终 stage 中验证，避免净机器字体/文本显示异常。

## 10. 修复顺序与验收门槛

### 10.1 推荐实施顺序

0. 在干净基线建立实施分支，先完成第 11 节 provenance；逐项移植当前工作树既有安全加固，不得直接整体暂存当前树。
1. 设计并实现共享 PVL schema/manifest 解析器：fallback、显式列表、空格/Unicode、数量、唯一性、二进制头一致性。
2. 接入 Paint 主加载、提取、PyWidget、Drishti 和所有 PVL writer，覆盖严格 `gridsize`/类型/重复元素解析，先关闭“Import/Paint 自产物不能重开”。
3. 把 Paint、Drishti 和 Drishti 时间点切换改为两阶段候选加载，失败保留旧工作集/当前帧；保留已有底层 VFM/VolumeBase 错误传播。
4. 将 Paint mask、tag names、curves 和退出整合为可传播错误的原子保存事务；保留已有 mask worker、journal、Undo 和 Checkpoint。
5. 修复 Paint 内存固定阈值，同时保留真实峰值/物理内存/Commit/iGPU 余量和算法级准入；再处理 H2/H3 的全局 reservation 与算法覆盖。
6. 修复 Import Save As 总事务、磁盘预检、旧尾 slabs、Time Series/Save Images 批次语义；复用已有局部 VFM/QSaveFile 事务。
7. 修复 orientation、排序、非整除采样、nearest 锚点、16-bit padding、RAW 几何、ROI 哨兵/边界。
8. 修复只读 sidecar、可取消长任务、命令行解析、帮助布局和可选依赖能力声明；H1 的重复 IFD 枚举/正式 Save GUI 阻塞属于原始卡死主范围，不得延期；按 PR 范围决定 H5/H6/H7 的非主链路项是本次关闭还是登记为依赖项。
9. 先跑小型确定性正式 GUI 闭环和多时间点切换，再跑 Living Ant；只从相同通过快照打最终 ZIP。

### 10.2 P0 实施入口映射

下表给出首批代码入口，不替代实现时的完整调用链搜索。行号会随修改漂移，交接以函数名为准。

| 问题 | 首要源码入口 | 必须证明的结果 |
|---|---|---|
| P0-I1 / P0-P1 / P0-P2 / P0-P3 / P0-P4 / P0-P14 | `tools/import/raw2pvl.cpp::savePvlHeader`、`tools/paint/volume.cpp::setFile`、`tools/paint/staticfunctions.cpp::getPvlNamesFromHeader`、`tools/paint/drishtipaint.cpp::savePvlHeader`、`tools/paint/pywidget.cpp`、`drishti/xmlheaderfunctions.cpp`、各 `volumefilemanager.cpp` | 一个共享 schema/manifest 契约；所有 reader/writer 使用；坏数量/重复/头不一致严格拒绝；无空格拆分 |
| P0-I2 / P0-I3 | `tools/import/raw2pvl.cpp::savePvl`/`batchProcess`、`tools/import/volumefilemanager.*` | XML + PVL + 可选 RAW 总事务；空间预检；失败不改变旧代 |
| P0-P5 | `tools/paint/getmemorysize.cpp::evaluatePaintMemoryAdmission`、`tools/paint/volume.cpp` | 不再由固定阈值误拒绝；仍按受检峰值、物理、Commit 和 iGPU 余量准入 |
| P0-P6 | `tools/paint/drishtipaint.cpp::setFile`、`tools/paint/volume.cpp::setFile/reset` | 坏候选后旧 volume/mask/curves/TF/UI 指针逐项不变 |
| P0-P7 | `drishti/mainwindow.cpp::preLoadVolume/loadVolume/loadProject`、`drishti/volume.cpp::loadVolume`、`drishti/volumebase.cpp` | 坏体积或坏项目后 current project、旧体积、几何、TF、关键帧、相机/UI 逐项不变且不显示成功 |
| P0-P8 / P0-P9 / P0-P10 | `tools/paint/volumemask.cpp::saveTagNames`、`tools/paint/curveswidget.cpp::saveCurves/loadCurves`、`tools/paint/drishtipaint.cpp::on_saveWork_triggered` 及关闭/换卷路径 | mask/tag names/三方向 curves 同一成功语义；任一失败不显示 Saved、不退出且可重试 |
| P0-P15 | `drishti/volumesingle.cpp::loadVolume/setBasicInformation/setVolumeNumber` | 所有时间点预检；切换候选提交；坏后续帧不改变当前 index/manager/尺寸/纹理 |

### 10.3 必须新增的最小端到端回归

建立非方形、已知坐标值的小体积，自动完成：

```text
TIFF directory / shuffled explicit list
-> 确认最终 Z 顺序和 orientation
-> 非零起点 X/Y/Z ROI
-> 可整除与非整除降采样
-> 8/16-bit 非零 padding
-> 正式 Save As PVL
-> 校验 XML/manifest/二进制头/尺寸/哈希/旧文件清理
-> Paint 加载
-> 写入 8-bit 和 16-bit 标签、Unicode tag names、三方向 curves
-> Save Work -> 正常关闭 -> 重开 -> 逐项校验
-> Drishti 加载 -> 校验方向和确定体素渲染 -> 正常退出
```

路径至少覆盖 ASCII、空格、中文、长路径；PVL 至少覆盖无显式列表、旧式列表、新式 manifest、重复/短/长列表、短 `gridsize` 和坏二进制头；命令行覆盖 `-drishti/-stereo` 的合法及错误组合。

### 10.4 故障注入门槛

- 取消发生在创建、写入、提交各阶段；
- 磁盘空间不足、打开/写入/flush/rename/delete 失败；
- 源只读、输出无权限、sidecar 在独立 workspace；
- 覆盖现有体积且新 slabs 更少；
- XML、slab、mask、tag names、curves 截断/损坏；
- 候选加载失败后旧 Paint/Drishti 工作集逐项不变；
- 项目引用缺失/损坏体积时，不提交 current project，不继续混入 Lowres/preferences/TF/keyframes，也不显示 `Project loaded`；
- Drishti 坏的后续时间点切换失败后仍停留在原帧且 manager/纹理/尺寸不变；
- Time Series 重名和中途失败不产生半批成功假象。

### 10.5 原始卡死和核显专项验收

目标 i7-13700H 笔记本必须记录：正式 exe/plugin SHA-256、RAM、页面文件、驱动、输入介质、Explorer 预览状态、1/2/4/10 张和完整数据的 UI 响应、Working Set、Private/Commit、Hard Fault、CPU、磁盘吞吐与取消结果。先用只读元数据和小批次，避免直接重现整机失去响应。只有现场证据才能决定原始故障的首因分支。

Intel/AMD 核显验收必须在非远程桌面会话记录 Vendor、Renderer、OpenGL、GLSL 和 profile。Renderer 不得是 `GDI Generic`、`Microsoft Basic Render Driver` 或软件实现；Drishti 需实际执行 4.5 compatibility 的主渲染 shader/FBO，Paint/Mesh 需实际执行 4.2 compatibility 路径。窗口创建或版本字符串本身不算完整通过。

### 10.6 发布门槛

以下全部满足前不得发布：

- 小型确定性正式 GUI 闭环在同一二进制快照通过；
- Living Ant 在该快照完成 Import Save As、Paint 保存重开、Drishti 加载；
- Intel 目标笔记本与 AMD iGPU 各至少一台完成硬件 OpenGL 和全链路；
- 失败注入不会产生新头/旧数据或丢失现有工作集；
- 最终 stage/ZIP 在净 PATH 和敌对环境变量下启动；
- PE 静态闭包和动态插件实例化均无缺失/Qt6 泄漏；
- ZIP 包含声明的 assets/docs/Python/plugins/Qt 运行时和 `qt.conf`；
- 记录 ZIP SHA-256、关键 exe/DLL 哈希、manifest、硬件/驱动和完整测试报告；测试后不得替换二进制。

## 11. PR 整理与交付要求

### 11.1 当前工作树不能直接提交

2026-08-13 状态有 210 个已跟踪改动和 1189 个未跟踪路径，混有长期业务修改、测试 Harness、构建输出、日志、数据和历史包。改代码智能体不得执行 `git add -A`、整体复制工作树或以当前脏树生成单一巨型 PR。

实施前建立逐文件 provenance 清单，至少包含：文件、来源批次、对应问题编号、保留/修改/丢弃决定、行为契约、测试、目标提交/PR。然后从明确上游基线建立干净分支或独立 worktree，按清单移植有意修改；本目录只作为证据源，不作为可直接提交的成品。

以下内容默认不得进入 PR：`.lab-agent`、本机代理/编辑器状态、构建目录、Makefile、OBJ/PDB/EXE/DLL、运行日志、审计临时数据、旧 ZIP、`__pycache__` 和下载缓存。`tools/*/tests` 中真正可复现的源码测试可以进入相应代码提交，但测试二进制和运行输出不能进入。

### 11.2 建议的提交/PR 边界

建议按依赖关系拆分为可审查提交或多个 PR，而不是按文件夹机械拆分：

1. Import TIFF/Image Stack 卡死防护、输入安全和回归；
2. 共享 VFM 受检 I/O、局部事务和内存准入基础设施；
3. Desktop OpenGL/iGPU 启动、资源预算、shader/FBO 失败传播与 CPU mesh paint；
4. 共享 PVL schema/manifest，以及 Import/Paint/Drishti reader/writer 接入；
5. Paint 两阶段加载、mask/tag names/curves 保存事务和算法安全；
6. Import Save As 总事务、ROI/采样/padding/批次正确性；
7. 打包脚本、`qt.conf`、help/assets、README/notices 和目标机验收材料。

若上游维护者更适合接收一个 PR，也应保持同样的独立提交序列。每个提交必须可构建，不能让中间提交写出新格式但 reader 尚未兼容。共享 schema 的格式演进需要先加入兼容 reader，再切 writer，最后删除不再支持的临时代码。

### 11.3 PR 声明和产物要求

PR 描述必须分别列出：确认修复的缺陷、针对未确认现场首因的防护、行为/格式变化、仍未验证的目标硬件、性能与可选功能限制。不得使用“全面支持所有 Intel/AMD 核显”或“原始卡死已彻底解决”，除非第 10.5、10.6 节证据已经完成。

最终交付至少包括：干净分支/提交序列、每个问题编号到 commit/test 的映射、构建命令和依赖版本、同一快照 stage/ZIP、manifest、关键二进制和 ZIP SHA-256、测试报告、目标 Intel/AMD 现场回传报告。`package_windows_portable.ps1`、`qt.conf`、README/notices 等源文件必须单独审查，不能用本机构建产物代替。

## 12. 实施时禁止采用的捷径

- 不要只给 Import 补空格分隔 `<pvlnames>`；空格路径、重复和数量错误仍会坏。
- 不要把 smoke/Harness 的部件通过写成正式 GUI 端到端通过。
- 不要把 28 slabs 手工桥接写成正式 Save As 的默认行为。
- 不要直接删除所有内存保护；完整大体积仍可能耗尽 Windows Commit。
- 不要用强制软件 OpenGL 代替核显硬件适配。
- 不要把 Save Images 或 Time Series 输出当作普通连续体积。
- 不要用旧 ZIP、旧日志或当前共享 bin 给后来隔离构建/未来 ZIP 背书。
- 不要把空 child log、强制终止或未归因 `0xC0000005` 写成产品成功/失败。
- 不要在正式端到端与目标硬件/净包门槛通过前承诺“全部功能可用”。
- 不要按历史交接中的“已完成”盲目重复实现或关闭问题；先按第 1.4 节证据优先级核对当前源码。
- 不要整体暂存当前 1399 条脏工作区状态；必须按第 11 节建立 provenance 并从干净基线移植。
- 不要把初始 TIFF 统计的后台 worker 描述成正式 Save/preview 解码已经后台化。
- 不要把单个 VFM 对同一 base 全组 slabs 的进程内回滚描述成 XML + PVL + RAW 的总事务或崩溃恢复。

## 13. 当前交接声明

本文已经达到“让改代码智能体有一份统一主基线”的交接条件：背景、当前证据、42 个明确问题/风险、既有实现继承关系、P0 代码入口、PR 边界和关闭门槛均已登记。后续智能体可以开始修复已确认问题。

本文没有声称正式 GUI 运行时全链路排查已完成，也没有声称所有缺陷已经穷尽。本机尚未执行正式 Save As、Paint 保存重开和 Drishti 最终渲染；目标 i7-13700H 首因、AMD/Intel 核显完整链路和最终净包也仍未验证。修复阶段必须同时关闭这些运行缺口，并允许它们产生新的问题编号。

修复过程中如发现新问题，应追加到相应章节，并保留“发现证据 -> 修改位置 -> 回归用例 -> 关闭快照”四项记录；不得只在提交说明中口头关闭。

## 14. 分阶段实施追踪（2026-08-13）

本节用于纠正此前“按混合工作树直接推进”的执行偏差。后续实施必须严格按第 11.2 节的依赖顺序推进；已有文件或历史修改不能作为阶段完成证明。

### 14.1 阶段 0：provenance、基线和提交边界

状态：**完成**。

已记录在 `DRISHTI_PHASE0_PROVENANCE_2026-08-13.md`：

- 当前分支 `codex/cpu-igpu-worktree-checkpoint-20260813`、基线 `e731972d2c8663b001ce06384b9b4074854d0c00` 和审计时间；
- 既有 CPU/iGPU、Import/Paint/Drishti 混合修改的来源批次和问题编号；
- 必须保留的用户文档修改；
- 阶段 1 候选与阶段 5/6/7 后续遗留的提交边界；
- `.lab-agent`、构建输出、日志、缓存、旧包和 `__pycache__` 的排除规则；
- 阶段 1 的行为契约、测试门槛和不得整体暂存的规则。

阶段 0 没有产生产品代码提交，也没有执行 `git add -A`。

### 14.2 阶段 1：共享 PVL schema/manifest

状态：**实现完成；阶段验收部分关闭**。

本轮新增/修改的候选文件：

- `common/src/pvlmanifest.h/.cpp`：增加直接子节点字段读取；支持 `<pvlvoxeltype>` 与旧式 `<voxeltype>` 兼容；记录 RAW voxel type/rawfile；校验 PVL/RAW 名称数量、重复路径、缺失文件、几何、13 字节二进制头和 payload 文件大小；保留结构化名称中的空格和 Unicode；
- `tools/import/tests/pvl_manifest_smoke.cpp/.pro`：覆盖结构化路径、Unicode、fallback、短/重复清单、嵌套字段、混合名称、RAW manifest、缺失/截断 slab、错误二进制头、映射长度和 RGB/RGBA 契约；
- Import、Paint、Drishti、Mesh 的 PVL reader 接入点已静态复核，正式加载入口使用统一 parser；元数据查询保留 `validateFiles=false`，不作为文件可读证明；
- RGB/RGBA ImageStack writer 只输出 `<channelnames>`，禁止 scalar `<pvlnames>`；Paint 提取 writer 不再复制源体积的 `rawfile` 引用；Mesh 的遗留空 `savePvlHeader()` 已补为结构化、原子写入实现；mask sidecar 保持独立格式边界，不纳入通用 PVL 校验。

验证快照：隔离目录 `D:\drishti-deps\build\phase1-pvl-manifest-20260813`，Qt 5.15.2、MSVC 14.29.30133；最新源码重新 qmake/build 后运行 `pvl_manifest_smoke.exe`，输出 `PVL manifest smoke passed`。该证据覆盖 parser/fixture，不等于正式 GUI Save As 或 Paint/Drishti 全链路通过。

阶段 1实现侧已关闭：

1. 共享 parser/schema、结构化名称、RGB/RGBA 专用契约、RAW/PVL 文件校验和所有已接入 reader/writer 的静态实现已完成；不能再把这些写成“尚未实现”。
2. `rawfile` 自动推导多 slab 仍只支持带 `.001` 编号的命名；非编号 RAW 必须显式提供 `<rawnames>`，这是既定格式限制，后续 writer/Save As 仍需继续核验。
3. 三个 ITK writer 已静态确认输出 13 字节头、实际尺寸、`slabsize=m_nX+1` 的单 slab sidecar；本机未能对插件 GUI writer 做运行时回归。

阶段 1验收仍未关闭：

4. Import/Paint/Drishti/Mesh 全工程当前仍未完成链接：本次真实 MSVC 单目标编译分别被缺少 `pybind11/embed.h`、`tiffio.h`、`common/lib/vdb.lib`、`QGLViewer/qglviewer.h`/`vec.h` 和 `GL/glew.h` 阻断。这些是构建环境阻断，不能归为源码通过。
5. 尚未执行正式 GUI Save As、Paint 保存重开和 Drishti 最终渲染；parser smoke 不等于 Import -> Paint -> Drishti 运行时闭环。
6. Save As 的 PVL/RAW/manifest 总事务、旧尾 slab 清理、Time Series 批次原子提交仍属于阶段 6，不能在阶段 1关闭。

因此阶段 1当前只能报告为：**共享 parser/schema 和阶段 1代码接入已完成，隔离 smoke 已通过；全工程构建和正式 GUI/运行时验收仍未关闭**。不得称 Import -> Paint -> Drishti 已闭环，也不得把当前工作树直接作为产品提交。

### 14.3 阶段 2：运行时候选加载、时间序列和项目解析

状态：**源码实现进行中；验收未关闭，不能称完整事务加载或 GUI 全链路通过。**

本轮已落地的代码边界：

- `tools/paint/drishtipaint.cpp::setFile` 先构造并验证独立 `Volume` candidate，再保存旧 mask/curves 并提交活动指针；坏候选不会先 reset 旧 Paint volume。候选失败会恢复 Paint bytes-per-voxel，并报告错误。
- `drishti/volume.cpp` 的单卷、2/3/4 卷、RGB/RGBA 和 DummyVolume 入口统一使用 candidate；失败恢复 `Global::volumeType`、PVL voxel type、LOD、relative scaling 和四个 `VolumeInformation` 槽位；candidate 析构导致的 DummyVolume 临时状态在成功交换后也会恢复。
- `MainWindow::preLoadVolume()` 不再清旧场景；RawVolume、灯光、几何和 keyframe 清理移到成功的 `postLoadVolume()`/DummyVolume 提交段。失败恢复不再调用 `m_Volume->clearVolumes()`，也不再把旧工作集退成 DummyVolume。
- `MainWindow::loadVolumeFromProject` 改为 `bool + volume type + 四组局部文件列表` 输出；XML 无法打开/解析或未知类型直接失败，解析期间不修改 `m_volFiles*`、current project、previous directory、Global、Viewer 或 Preferences。
- `VolumeSingle::loadVolume` 打开时间序列时预检所有时间点的 manifest、slab、几何、slab size 和 voxel type；`setSubvolume` 在变更活动索引前再次校验目标时间点。`VolumeRGB::setSubvolume` 先用局部 channel managers 验证 RGB/RGBA 所有通道，再提交索引、范围和 manager 配置。

当前明确未关闭的部分：

1. 项目 XML 中 geometry/camera/LightHandler 的候选提交现在由已验证的 detached keyframe candidate 在显式边界应用；preferences、lowres、文件列表/current project/QAction 已有失败快照恢复，TF 已有容器级候选交换。仍需运行时故障注入确认 setter/GL 失败时的整体外部行为。
2. `postLoadVolume()`/`DrawLowresVolume::loadVolume()`/`DrawHiresVolume::loadVolume()` 仍是旧的 void/就地资源接口；必须把低清晰度、shader、纹理和必要渲染资源纳入可回滚提交，才能关闭 P0-P7。
3. 时间序列切换的 manager/texture 运行时故障注入和当前帧哈希不变量尚未在 GUI 上验证；当前源码预检覆盖文件/schema/几何，不等于显卡纹理切换事务。
4. Paint、Drishti、Mesh 全工程 MSVC 构建仍受本机依赖阻断；本轮已执行 qmake 工程生成和受影响目标的真实编译尝试，但未执行正式 GUI、OpenGL 或目标核显验收。

本轮继续实现并复核了以下边界：

- Paint 内存准入不再在物理/Commit 双预算之前使用固定 2 GiB resident-volume 门槛；保留受检峰值、地址空间、系统和 iGPU 余量判断；
- Paint curves 保存继续使用 `QSaveFile`，读取对 EOF、未知关键字、负数/超大点数、短读和分配失败进行拒绝，并将失败传播到拖放加载和项目重开路径；
- Drishti `-drishti`/`-stereo` 参数在入口消费，文件参数不再被当作选项或时间点；keyframe 失败会短路项目成功提示；
- Import 正式 PVL Save As 修正 16-bit padding 元素填充、非整除 ROI 块的边界采样归一化、磁盘空间预检、RAW+padding 明确拒绝和成功提交后的旧尾 slab 清理；
- Import 的 Quick RAW 与普通 Save As 改用显式操作状态，原点 `1 x 1 x 1` ROI 不再被坐标哨兵误判；
- qmake 已对 Import/Paint/Drishti 重新生成；真实 MSVC 编译已启动，但当前构建配置分别受 `pybind11`/`tiffio.h`、`common/lib/vdb.lib` 和 `GL/glew.h` 外部依赖阻断，不能记为全工程构建通过。

因此阶段 2当前仍应报告为：**候选边界、时间序列文件预检、项目 lowres/preferences/file-list/UI 回滚、keyframe 解析 candidate、保存/曲线错误传播和部分 Import 正确性已编码；geometry/camera/LightHandler 的完整项目 detached candidate、完整 XML/PVL/RAW/sidecar 崩溃恢复事务、Time Series/Save Images 的跨对象崩溃恢复、GUI 故障注入和正式运行时验收仍未关闭。**

### 14.4 阶段 4-6：保存事务、算法准入和批次输出

状态：**部分实现；验收未关闭。**

已落地的源码边界：

- `VolumeFileManager::createFile()` 在 staged generation 前按数据体积、rollback backup 和安全余量执行 `QStorageInfo` 可用空间预检；
- 正式 `Raw2Pvl::savePvl()` 的 PVL header 现在写结构化 `<pvlnames>`，padding 使用元素宽度正确的填充值；RAW 伴随 padding 暂时显式拒绝，避免 RAW/PVL 几何声明与写入缓冲不一致；
- 旧尾 slab 仅在当前 PVL/RAW 新一代成功提交后按同一 base 清理；中途失败保留已有事务回滚路径；
- Paint tag names 使用 `QSaveFile` 并返回错误，Save Work/退出/换卷路径已检查 curves、tag names 和 mask 的结果。

尚未关闭：

1. XML、全部 PVL slabs、可选 RAW slabs 和 Time Series 整批仍不是一个跨对象崩溃恢复事务；
2. Save Images 已增加同目录 staging、整批提交和旧尾切片清理；MHD 仍需独立验证边界块契约。`batchProcess`/`mergeVolumes` 已接入持久 `BatchPathJournal`，能在下一次同目录操作开始前恢复未完成批次；这仍不是 XML/PVL/RAW/sidecar 的全局崩溃恢复协议；
3. curves 旧格式虽已有读取上限和短读检查，但尚未升级为版本化格式，正式 GUI 损坏文件回归未执行；
4. 非整除采样的 batch/MHD 入口已在 14.9 迁移到共享 nearest/边界块契约；仍需短写、磁盘满、rename/delete 和异常退出故障注入，不能把源码完成写成运行时通过；
5. 磁盘空间预检是时点估算，仍需在真实磁盘满、rename/delete 失败和覆盖旧尾 slab 场景验证。

阶段 4-6 不得称为完成；当前只可称为相关 P0/P1 入口已有部分防护和实现。

### 14.5 本轮继续实施（阶段 3/6/7/9 的源码边界）

状态：**源码边界继续收紧；构建、正式 GUI 和跨硬件验收仍未关闭。**

本轮新增实现：

- TIFF `samePageLayout()` 现在比较 orientation；同一堆栈混合 orientation 1/4 会在候选提交前拒绝，并通过 smoke 断言拒绝后保留原活动栈；单一 top-left/bottom-left 仍保持既有支持。
- TIFF 显式多文件入口与目录入口统一使用大小写不敏感、数字感知的自然文件名排序，避免文件对话框返回顺序改变 Z 轴。
- Import ROI 显示在单体素轴上使用安全归一化分母；ROI 提交使用 clamp 后的 `floor(min)/ceil(max)` 闭区间，覆盖非零起点、反向拖拽和单体素轴，避免浮点边界直接截断。
- 正式 PVL Save As 的 `No Interpolation` 路径固定为每个采样块首体素 nearest 锚点，后续块内像素不会覆盖该锚点；MHD/batch 仍保留旧整块契约，不能把 PVL Save As 的向上取整外推到它们。
- Save Images 改为批次 staging 目录：所有图像/RAW 切片先以 `QSaveFile` 写入临时目录，完整成功后再统一备份、重命名提交；取消、编码/写入失败或提交失败清理临时批次并恢复已有目标。
- Time Series/Batch Process 在开始前预生成输出 basename 清单，规范化路径后拒绝重名，避免不同目录同名输入互相覆盖；batch/merge 的持久 journal 可在后续同目录操作前恢复未完成输出，但跨时间点 XML/PVL/RAW 总事务仍未实现。
- Drishti 项目解析增加根节点、唯一 `volumetype`、非空 volume file 子节点和按体积类型的文件组数量校验；keyframe sidecar 除魔数外增加最小长度、512 MiB 上限、`keyframes`/`done` 终止标记预检，坏项目在候选体积提交前失败。项目失败回滚另保存 lowres、偏好、当前项目、文件列表和关键 QAction 状态，但 geometry/camera/LightHandler 仍未 detached。
- Drishti/Import/Paint 的 Windows 帮助目录探测在缺少 QHC 时优先回退到包内 PDF 目录（必要时从 `docs/<app>` 回退到 `docs`），避免便携包启动后必然弹目录选择；实际帮助点击、PDF 名称匹配和净包验证仍未执行。
- `MainWindow::postLoadVolume()` 现返回 `bool`，低清晰度/高清晰度纹理或 shader 资源创建失败会向 Dummy、RGB、单卷和多卷入口传播，阻止后续“Volume loaded”成功提示；项目加载也直接准备 detached candidate 的渲染资源，不再调用会提前提交元数据的菜单包装；Drishti qmake 工程已重新生成。
- `Volume` 现在保留 pending old state；普通体积入口只有在低清晰度/高清晰度资源成功后才提交，项目加载可延迟提交并在 sidecar 失败时恢复 CPU 体积、渲染资源和关键 Global/TF 状态。普通单卷/多卷菜单入口也只在成功后清理旧几何和关键帧。
- Time Series/Batch Process 增加 `BatchPathJournal`：开始前保护预期 PVL/RAW header、slab 和旧尾文件，整批成功才删除备份，取消/解码/写入失败时恢复旧输出；这关闭了正常运行中的整批半提交，但仍不是崩溃恢复 journal。
- `TransferFunctionManager::load()` 先解析到独立 `TransferFunctionContainer`，成功后才交换到活动容器；坏 TF XML 不再先清空当前 TF。
- Paint `CurveGroup` 支持候选交换；曲线读取先解析到临时组，成功后才替换当前三方向曲线。`DrishtiPaint::saveCurvesTransaction()` 使用与目标同目录的临时批次、旧文件备份和失败恢复，已接入 Save Work、退出和换卷路径，避免三方向曲线半批提交。

本轮仍明确未关闭：

1. geometry 和 keyframe 的 detached project candidate 已建立并在显式提交边界应用；preferences 的背景色、步长、Gamma、眼间距、刻度、轴标签和 tag colors 已加入项目失败回滚快照。仍不能把未执行的运行时故障注入写成项目整体原子提交证据。TF 已完成容器级候选交换，lowres/highres 资源和 CPU 体积支持延迟提交/失败回滚。
2. XML、全部 PVL slabs、可选 RAW slabs、Time Series 和 Save Images 尚未形成统一跨对象崩溃恢复 journal；batch/merge 的 `BatchPathJournal` 已覆盖同目录下一次新操作开始前的残留恢复，Paint Save Images 的 staging/backup 事务覆盖正常运行中的取消、解码、写入和提交失败回滚，但不等于全局崩溃恢复。
3. batch/MHD 非整除采样和 nearest/边界契约已在 14.9 完成源码迁移；正式 PVL Save As 的修复与其统一，但运行时故障注入仍未完成。
4. 真实 GUI Save As、Paint Save Work 重开、Drishti 最终渲染、时间序列纹理/manager 故障注入、目标 Intel i7-13700H/AMD 核显和当前源码快照便携 ZIP 仍未验收。

本轮验证：`git diff --check`、受影响源码接口静态审计和 Import/Drishti/Paint qmake 生成通过；显式调用 VS BuildTools 后 Drishti 目标在缺少 `GL/glew.h` 处阻断，Paint 目标在缺少 `common/lib/vdb.lib` 处阻断，既有全工程构建仍受 `pybind11`、`tiffio.h`、`vdb.lib`、`QGLViewer`、`glew` 等外部依赖阻断。已有 PVL manifest 与 VFM smoke 在补齐 Qt runtime 环境变量后均返回 0。不得据此声明全工程链接或全链路运行通过。

### 14.7 本轮代码收口（2026-08-14）

本轮继续完成了以下源码工作，仍不包含正式构建、GUI、核显或净包验收：

- `MainWindow::loadKeyFrames()` 不再在 `commit=true` 时隐式提交旧的 pending candidate；新增显式 `commitPendingKeyFrames()`，项目加载只有在已预检并确认当前 candidate 存在时才提交，避免 stale candidate 被误套用。
- `KeyFrame::load()` 现在检查零号保存帧、每个普通帧的分配和流状态；单帧截断/坏记录会拒绝整个 candidate，不再只依赖外层魔数和终止标记。
- Paint curves 写入增加 `Drishti Curves 2` 版本头；reader 同时兼容版本 2 和历史无版本头格式，原有 EOF、未知关键字、点数上限、短读和候选交换检查继续生效。
- Import `BatchPathJournal` 升级为同目录持久 journal：记录 header、PVL/RAW slabs 和旧代保护副本；启动新的 batch/merge 前恢复残留未提交 journal，已提交 journal 只清理备份。journal 使用版本 2，提交标记先于备份清理写入。
- `Raw2Pvl::mergeVolumes()` 接入同一 journal，旧 header、旧尾 slabs 与新 generation 统一保护；先写总 journal 提交标记，再释放 VFM 备份，避免 header/slab 半提交。
- `Raw2Pvl::batchProcess()` 对 `.pvl.nc`/`.pvl` stem 做显式去扩展名，时间序列输出不再因扩展名重复造成别名；完成提示前必须成功终结 journal。
- Drishti project rollback snapshot 现在还覆盖 lowres showing/current-volume、TF dock、Empty-space-skip QAction、current project、previous directory 和四组项目文件列表；坏项目不会只恢复 CPU volume 而留下新项目 UI 元数据。

### 14.8 本轮继续收口（2026-08-14）

- `DrawHiresVolume::loadTextureMemory()` 和 `postUpdateSubvolume()` 现在返回明确的成功/失败结果；Time Series/多卷切帧在 CPU 解码、纹理句柄、数组纹理或拖拽纹理上传失败时，尝试恢复切帧前的时间点和旧边界，并重建旧高分辨率资源。该逻辑关闭了“切帧失败只留下新 CPU 状态”的源码缺口，但仍需正式 GUI/显存故障注入验证。
- `Volume::currentVolumeNumber()` 暴露当前已提交时间点，回滚使用实际当前帧而不是全局请求帧，避免旧帧编号在 cycle/wave 模式下被错误映射。
- Paint 曲线三文件保存改为复用 `SlabSaveTransaction` 的同目录持久 journal；生成的 `d/w/h` 文件统一 staging，正常提交前不会替换目标，进程在切换阶段退出时下一次同目标操作可按 journal 恢复。无曲线方向用 `stagePresent=false` 显式记录。原有 `Drishti Curves 2` 格式与旧格式读取兼容保持不变。
- `SlabSaveTransaction` 增加 `stagePresent` 字段和空阶段提交语义，保留旧 journal（缺字段时按存在处理）兼容；Paint curves 的 journal 与 mask slab journal 共用同一恢复实现。
- 本轮将 geometry/camera/LightHandler 的 keyframe 应用收口为显式 detached candidate 提交；仍需运行时故障注入验证对象 setter/GL 错误传播，不能把静态路径当作运行时通过。

本轮静态验证：三工程 qmake 生成和受影响文件 `git diff --check` 通过。当前仍没有可用的 `cl.exe`/`nmake.exe` 和完整外部依赖，因此未声称 MSVC 链接通过。

本轮仍明确未关闭：

1. 项目 geometry/camera/LightHandler/clipplanes 等对象现在通过已验证 keyframe detached candidate 的显式提交边界写入活动对象；仍需可注入 setter/GL 故障确认跨对象原子行为。
2. Time Series 当前帧切换源码已有失败后恢复旧帧路径，但仍缺少 manager/texture 失败后的 GUI 级故障注入证据；文件/schema 预检和静态回滚逻辑不等于显卡运行时通过。
3. journal 已具备 batch/merge 以及 Paint curves/mask 各自正常异常退出后的下一次同目标恢复路径，但 XML/PVL/RAW 跨对象恢复、Save Images 与项目 sidecar 尚未统一成一个全局 journal 协议。
4. 磁盘满/rename/delete 失败注入、正式 GUI Save As/Paint 重开/Drishti 渲染、Intel/AMD 核显和净包验收仍后置；batch/MHD 的源码采样契约已在 14.9 收口，但尚未做运行时故障注入。

### 14.9 本轮代码完成收口（2026-08-14）

本轮在不进行正式构建和验收的前提下，又完成了以下源码项：

- `Raw2Pvl` 的 batch 和 MHD 路径现在使用 `SamplingContract` 的向上取整输出尺寸与实际边界块样本数；无滤波路径统一使用块首体素 nearest 锚点，不再由块内最后一个像素覆盖结果。
- MHD 的 `.mhd` 与 `.raw` 目标现在在写入前注册到持久 `BatchPathJournal`，旧文件由 journal 保护；任一 `QSaveFile` 提交失败时由析构回滚，两个文件都提交后才写入 committed 状态并清理备份。
- `DrawHiresVolume::initShadowBuffers()` 现在按 candidate 与旧 shadow 附件的并存峰值预算准入，OpenGL 创建/附件校验失败时恢复旧尺寸和全部旧句柄，并恢复调用前 framebuffer、active texture 和 rectangle texture 绑定。
- `LightHandler::genBuffers()` 现在完整创建并验证 opacity/prune/final/work/emissive candidate 后才交换活动资源；candidate 失败只清理 candidate，不销毁仍可用的旧光照资源，并恢复调用前 framebuffer、active texture 和 rectangle texture 绑定。
- Import batch/MHD 中已清理不再使用的采样和 header 局部变量，避免把旧整除契约误留在代码中。

本轮静态验证：`git diff --check`、`qmake drishti/drishti.pro`、`qmake tools/import/import.pro`、`qmake tools/paint/paint.pro` 和 `qmake tools/import/tests/sampling_contract_smoke.pro` 均通过。qmake 只证明工程文件和生成阶段通过，不等于 C++ 编译、链接或运行时通过。

本轮仍未关闭的验收边界：

1. geometry/camera/LightHandler 的 detached renderer candidate 已建立并在显式提交边界应用；仍需运行时 setter/GL 故障注入确认跨对象原子行为。
2. XML、PVL、RAW、Time Series、Save Images、Paint sidecar 尚未统一为一个跨对象全局崩溃恢复 journal；现有 batch/MHD、Paint mask/curves journal 仍是各自目标域恢复。
3. `LightHandler`、`DrawHiresVolume`、Drishti Viewer、Mesh Viewer 和 RcViewer 已形成 candidate/旧资源并存峰值预算和状态恢复；预算数字与失败回滚尚未经过真实 OpenGL allocation/FBO incomplete 故障注入。
4. batch/MHD 的实现尚未经过短写、磁盘满、rename/delete 失败、覆盖旧尾 slab 和异常退出故障注入；正式 GUI Save As、Paint 保存重开、Drishti 最终渲染仍未执行。
5. MSVC 完整编译链接、目标 i7-13700H、AMD 核显、净环境便携 ZIP 和最终发布包仍未验收。

### 14.10 本轮继续代码收口（2026-08-14，已由 14.11 增量更新）

本轮又关闭了以下源码缺口：

- `KeyFrame::commitRendererCandidate()` 现在只接受已完成校验的深拷贝 candidate，并通过显式 `applyRendererCandidate()` 提交；旧的 `playSavedKeyFrame()` 仅作为常规播放入口转发，不再被项目加载器当作候选解析阶段。项目加载仍需在正式运行时注入 setter/GL 失败，确认跨对象提交后没有外部副作用。
- Drishti Viewer 和 Mesh Viewer 的图像 FBO、低清 FBO 及 movie readback buffer 现在先候选创建、完整性校验、大小/预算准入，再替换旧资源；resize 或 readback 分配失败时保持旧 FBO、旧尺寸和旧 buffer。FBO 校验恢复调用前 framebuffer。
- Drishti `RcViewer::createFBO()` 不再先删除旧 raycast FBO；新的深度附件和 4 个 RGBA32F 入口/出口纹理全部创建并检查完整后才交换，并恢复 framebuffer、renderbuffer 和 rectangle texture 状态。候选失败只清理候选资源。
- 项目保存 journal 继续覆盖两个物理目标 XML + `.keyframes`；`.lowres`、`.preferences`、`.tf` 是嵌入 XML 的节点，随 XML staging 一起原子提交。加载项目会先恢复残留两目标 journal，保存时 XML 与二进制 sidecar 全部 staging 后才统一提交。

本轮源码静态核验：`git diff --check`、Drishti/Mesh qmake 生成通过。以上不等于 C++ 编译、链接或运行时通过。

截至该轮，仍只剩以下必须通过构建/运行时证据关闭的事项；14.11 对其中的源码边界继续做了增量收口：

1. 完整 MSVC 编译和链接，以及缺失的 `GL/glew.h`、`QGLViewer`、`vdb.lib`、`tiffio.h`、`pybind11` 等外部依赖环境补齐；
2. Import Save As、Time Series/Batch/MHD、Paint Save Work/关闭重开、Drishti 项目加载/最终渲染的 GUI 验收；
3. 磁盘满、短写、取消、rename/delete 失败、OpenGL allocation/FBO incomplete 和时间序列纹理/manager 失败注入；
4. Intel i7-13700H 核显、至少一台 AMD 核显、共享内存/Commit 压力和非远程桌面 OpenGL profile/renderer 记录；
5. 净环境便携 ZIP、Qt/Python/plugin 动态依赖、帮助入口和四个程序的启动及插件实例化。

### 14.6 本轮增量（2026-08-13）

- 修正 `MainWindow::loadProject()` 的事务顺序：先捕获旧体积/渲染/偏好状态，再进入延迟提交；此前相反顺序会让 `captureVolumeLoadRollback()` 因延迟标志直接跳过快照。
- `PreferencesWidget::State` 现在保存并恢复背景色、still/drag stepsize、Gamma、眼间距、tick size/step、X/Y/Z 标签和 256 个 tag colors；项目 sidecar 失败或 lowres/highres 资源创建失败时恢复这些字段。
- 普通加载成功提交后清除偏好快照；失败恢复统一走完整 `rollbackProjectVolumeLoad()`，不再只回滚 CPU volume。
- `Raw2Pvl::batchProcess()` 在显示 `Done` 前提交 `BatchPathJournal`，确保用户看到成功提示时旧代备份已完成正常提交清理。该 journal 仍不提供进程崩溃后的恢复能力。
- Paint `ImageWidget::saveImageSequence()` 现在先将全部切片写入同目录 staging，再统一备份/提交，并清理缩短批次留下的旧尾切片；取消、编码/写入失败或提交失败会删除新代并恢复旧文件。该路径仍未做正式 GUI 磁盘满/权限故障注入。
- Paint 单张 `ImageWidget::saveImage()` 现使用 `QSaveFile` 并检查编码、打开和提交结果；失败不再显示 `Done`。
- Import `Raw2Pvl::getSettings()` 的两个内存设置输入现在显式检查取消；取消不再把默认值当成用户确认值继续批处理。上层仍需在正式 GUI 中验证取消后的整体提示语义。
- 三个工程的 qmake 重新生成均通过：`drishti/drishti.pro`、`tools/import/import.pro`、`tools/paint/paint.pro`；本机仍没有可直接调用的 `cl.exe`/`nmake.exe`，所以未升级为 MSVC 链接通过。
- 本轮只通过 `git diff --check`、三工程 qmake 重新生成和静态引用核验；没有新增正式 GUI、OpenGL、核显或净包验收证据。项目加载 RGB/RGBA 候选路径已改为直接调用 `Volume` candidate，不能把它等同于 RGB 项目完整 GUI 验收。

### 14.11 本轮代码完成度更新（2026-08-14）

本轮继续按“先完成源码、后统一构建和验收”的要求收口，新增以下实现：

- 正式 `Raw2Pvl::savePvl()` 在写入前创建覆盖 XML header、PVL/RAW slabs 和旧尾 slabs 的同目录 `BatchPathJournal`；任一写入、提交、旧尾清理或取消失败时由 journal 恢复旧代，全部输出完成后才写 committed 标记。Time Series 输出路径同时做规范化去重，检测到 basename 冲突直接拒绝。
- Save As 增加跨 PVL/RAW/header/staging/backup 的峰值磁盘空间预检，并对尺寸乘加溢出做拒绝；`VolumeFileManager` 原有单卷空间预检仍保留，二者不能互相替代。
- 为保证 journal 的同目录原子语义，Save As 现在拒绝 PVL 与 RAW 位于不同目录的组合；跨卷/跨文件系统输出不再伪装成可回滚的全局事务。
- Paint 只读既有 mask 或只读 tag sidecar 不再仅因目录可写而误用源目录；此类源卷统一路由到基于源路径哈希的用户可写 workspace。普通 Save As 的 `1 x 1 x 1` 原点 ROI 不再触发 Quick RAW；DICOM/Mimics 的 Quick RAW 改为独立显式入口。
- Help 路径继续保留 QHC 优先、包内 PDF 回退；Paint 内嵌帮助、Import/Drishti PDF fallback 与当前 `bin/docs` 布局的实际点击验证仍待验收。
- Windows 帮助目录探测现在兼容 `docs/<app>` 和便携包 `bin/docs` 平铺布局；批处理参数解析失败后立即返回，不再在失败解析后继续执行空项目流程。

本轮静态验证：`qmake drishti/drishti.pro`、`qmake tools/import/import.pro`、`qmake tools/paint/paint.pro` 和 `git diff --check` 均通过。没有执行完整 C++ 编译/链接、正式 GUI Save As、Paint 保存重开、Drishti 最终渲染、OpenGL 故障注入、Intel/AMD 核显或净环境便携包验收。

当前代码状态应准确表述为：正式普通 Save As 及其 Time Series 输出的 XML/PVL/RAW/旧尾 slab 已进入同一持久 journal 事务；Save Images、Paint mask/curves sidecar、项目保存 sidecar 仍是各自输出域的恢复协议，尚未合并成覆盖所有产品域的单一崩溃恢复协议。剩余工作主要是构建依赖补齐、运行时故障注入和目标环境验收；不能把本轮 qmake 或静态检查写成“全链路通过”。

本节覆盖并修正 14.4、14.5、14.8、14.9 中早于本轮的“Save As XML/PVL/RAW 总事务尚未实现”描述：这些历史记录保留发现过程，但截至 14.11，正式 `savePvl()` 的源码实现以本节为准；仍未关闭的是实际 GUI/崩溃恢复故障注入证据。

### 14.12 本轮代码边界继续收口（2026-08-14）

在不启动正式构建和运行时验收的前提下，本轮又完成以下源码修补：

- Drishti 与 Mesh 的 `ScopedTrisetGlState` 现在额外保存/恢复固定功能状态 `GL_LIGHTING`、`GL_LINE_SMOOTH` 和 `GL_LINE_WIDTH`；此前 FBO、纹理单元、VAO/VBO、混合、深度、裁剪、颜色/深度写掩码和 polygon mode 的恢复保持不变。该修改只扩大状态守卫覆盖面，仍需 OpenGL 哨兵状态和失败路径注入验证。
- Paint `VolumeFileManager` 在创建新的 `.drishti-mask-snapshot-*.tmp` 前清理同目录超过 24 小时的同名孤儿快照；当前活动或其他进程近期产生的快照不会被删除。清理失败不影响当前快照创建，但残留清扫和异常退出行为仍需在真实只读/权限场景验证。
- Drishti 批处理参数解析改为显式消费 `-drishti/-stereo`、支持的 `project/renderframes/plugin/image/movie/framerate/imagemode/imagesize/stepsize` 和无值开关；未知选项、空值、重复/非法数值、非法图像模式、坏 `file=` 配置和非 PVL/XML 位置参数立即返回失败，不再继续空项目流程或把坏参数当作时间点。正式批处理组合仍需 GUI/进程级回归。

本轮静态验证：`qmake drishti/drishti.pro`、`qmake tools/paint/paint.pro` 与 `git diff --check` 通过。没有执行 C++ 编译/链接、正式 GUI Save As、Paint 保存重开、Drishti 最终渲染、真实磁盘/短写/rename/delete 故障注入、OpenGL allocation/FBO incomplete 注入、Intel/AMD 核显或净环境便携包验收。

截至 14.12，源码层面仍有以下明确边界，不能写成“代码已 100% 通过验收”：

1. Import/Save As、Time Series/Batch/MHD、Paint Save Work/关闭重开和 Drishti 项目/渲染闭环仍需正式运行证据；各自 journal 已覆盖对应输出域，但尚未合并为全产品单一崩溃恢复协议。
2. Paint 内存准入仍是时点快照，不是跨任务全局 reservation；长算法取消、TIFF 正式 Save/preview/orthogonal 的 GUI 线程解码和单次 codec 硬超时仍需按产品范围决定实现或登记依赖项。
3. OpenGL candidate/旧资源并存预算、状态恢复和失败回滚仍需真实驱动故障注入；`GL_LIGHTING`/线状态的本轮补全不能替代 Intel/AMD 实机画面证据。
4. 当前工作树包含大量历史改动、生成物和审计文件，不能直接作为单一 PR；后续构建/验收必须从明确源码快照建立 provenance、排除生成物并记录哈希。

### 14.13 本轮继续源码收口（2026-08-14）

本轮仍未启动正式构建、GUI、OpenGL、Intel/AMD 核显或便携包验收，新增关闭以下确定性源码缺口：

- Import 内嵌 NetCDF C++ 包装层 `NcError::set_err()` 不再执行进程级 `exit()`；默认错误策略改为 `verbose_nonfatal`，错误状态继续通过返回值传播。
- Import 内嵌 NetCDF v2 兼容层 `nc_advise()` 不再在 `NC_FATAL` 分支退出进程，默认 `ncopts` 改为非 fatal；损坏/不兼容输入只能使调用失败，不能终止 DrishtiImport 宿主 GUI。
- Paint 中此前未接收 `ok` 的数值对话框已补齐取消短路：曲线批量删除、Volume/标签抽取、跨卷抽取、Shrinkwrap/Shell/Tubes、原始体修改、纹理 subsampling、Hatch、Smooth、连通域、分水岭和旋转图像序列等入口。取消不再把默认值当成用户确认值继续执行。
- Paint 形态/连通域/分水岭入口的阈值和起始标签输入增加显式范围与取消检查；已有 `QFutureWatcher` 的算法取消连接保持不变，未宣称其所有内层循环都具备硬中断粒度。

本轮静态检查：`git diff --check` 通过；仍需执行三工程 C++ 编译/链接以发现平台相关接口问题。由于当前机器缺少完整 MSVC/Qt/OpenGL/第三方依赖，本轮不把 qmake 或静态检查写成构建通过。

截至 14.13，以下仍明确后置：TIFF 正式 Save/preview/orthogonal 的 GUI 线程解码和单次 codec 硬超时、跨任务全局内存 reservation、长算法的完整硬取消语义、全产品统一崩溃恢复 journal、真实磁盘/短写/rename/delete、OpenGL allocation/FBO 故障注入、Intel/AMD 核显和净便携包验收。源码收口完成不等于端到端验收完成。

### 14.14 本轮继续源码收口（2026-08-14）

本轮继续不启动正式构建、GUI、OpenGL、Intel/AMD 核显或便携包验收，新增关闭以下源码缺口：

- 新增 `common/src/memoryreservation.h/.cpp` 进程级线程安全 reservation。Import、Paint 和 TIFF 共享同一 reservation 计数；在快照准入通过后、危险缓冲创建前进行原子占用，超出已占用预算时拒绝本次操作，RAII 生命周期结束后自动释放。
- Import `Raw2Pvl` 的 conversion、VDB、merge、Quick RAW 和 mesh-color 导出缓冲接入 reservation；相关 `.pro` 和独立 memory-admission smoke 工程显式编入共享实现。reservation 覆盖对应局部操作，不改变现有 QSaveFile/BatchPathJournal 提交语义。
- TIFF 单 slice 解码在 `loadTiffImage()` 和行级 `loadTiffRow()` 前接入 reservation；preview/width/height/rawValue 仍沿用既有受检错误传播。该修改关闭的是同进程并发准入竞争，不等同于正式 GUI 解码后台化或单次 codec 硬超时。
- Paint 体积加载和三维算法准入接入同一 reservation；体积加载只在实际加载阶段持有峰值 reservation，算法入口在整个算法调用期间持有 reservation，避免 Import/Paint 并发任务各自通过独立快照。
- 所有引用 `importmemoryadmission.cpp` 或 `getmemorysize.cpp` 的插件/smoke 工程补齐共享实现源文件，避免链接阶段缺失 reservation 符号。

本轮静态检查：三主工程、TIFF 插件、Import/Paint memory-admission 相关 smoke 工程的 `qmake` 均通过；`git diff --check` 通过。没有执行完整 C++ 编译/链接、正式 GUI Save As、Paint 保存重开、Drishti 最终渲染、真实磁盘/短写/rename/delete、OpenGL allocation/FBO 注入、Intel/AMD 核显或净环境便携包验收。

截至 14.14，仍明确后置：Paint 长算法的内层硬取消和所有入口逐项事务覆盖、TIFF 正式 Save/preview/orthogonal 的 GUI 线程后台化及单次 codec 硬超时、XML/PVL/RAW/Time Series/Save Images/Paint sidecar 的全产品统一崩溃恢复协议、OpenGL 真实失败矩阵、目标硬件画面和最终净包。源码 reservation 已完成，但只能作为运行时验收的前置保障，不能写成全链路已通过。

### 14.15 本轮 Paint 取消传播收口（2026-08-14）

本轮继续只做源码修改和静态/qmake 检查，没有启动正式构建、GUI、OpenGL、Intel/AMD 核显或便携包验收：

- `VolumeOperations` 增加线程局部 `PaintCancellationScope`。带事务的长算法在事件泵观察到取消后立即返回，已有 `MaskRegionTransaction` 析构会恢复本次区域的原始 mask 字节；取消不会显示完成提示或提交事务。
- 连通域、移除最大/最小连通域、分水岭、优先队列分水岭、距离变换、局部厚度及 ROI/形态相关入口接入取消作用域；分水岭的体素/路径内层增加周期性检查，避免只在 Z 层边界响应。
- 并行 `getVisibleRegion()` 增加共享原子取消标志；worker 在行级检查取消，取消后不更新 visibility cache，调用方得到清空的临时 bitmask。外层事务取消会传播给嵌套的可见区域计算。
- 进度对话框统一显示 `Cancel`，避免已有 `QFutureWatcher::cancel()` 只停止调度而 worker 继续写共享结果；取消作用域只影响当前调用栈，不改变正常完成路径。

本轮静态验证：`qmake tools/paint/paint.pro` 和 `qmake tools/import/import.pro` 均通过，`git diff --check -- tools/paint/volumeoperations.cpp` 通过。未执行完整 C++ 编译/链接；当前仍受本机 Qt/OpenGL/VDB/插件依赖阻断。

本轮后仍明确后置：未纳入 `MaskRegionTransaction` 的旧形态学、GraphCut/LiveWire、部分整卷循环和并行 worker 的逐入口事务核验；TIFF 正式 Save/preview/orthogonal 后台化及单次 codec 超时；全产品统一崩溃恢复 journal；真实磁盘/短写/rename/delete、OpenGL 故障注入、目标 Intel/AMD 核显和净便携包验收。PLY 旧 C API 中的 `exit(-1)` 仍需按实际构建调用边界单独改造，未做全局机械替换。

### 14.16 本轮 TIFF/长算法继续收口（2026-08-14）

- TIFF `getDepthSlice()`、`getWidthSlice()`、`getHeightSlice()` 现在通过 `QtConcurrent` worker 执行实际 IFD/scanline 解码，GUI 线程只负责取消、等待和结果提交；共享原子取消标志在 slice/row 边界传播，失败时输出缓冲清零并通过 `lastError()/wasCanceled()` 返回错误。
- `loadTiffRow()` 增加取消参数，`rawValue()` 保持同步单点读取但沿用受检错误和 reservation；单次 `TIFFReadScanline()` 仍无法在损坏 codec 内部被强杀，硬超时仍需进程隔离方案，不能把 worker 化写成硬超时已解决。
- Paint 的最大/最小连通域和连通域标注阶段增加事务进度和周期性取消检查；取消发生在 mask 写入前后都由 `MaskRegionTransaction` 回滚。qmake 生成和 `git diff --check` 通过。

本轮仍未执行完整 C++ 编译/链接、正式 GUI Save/preview/orthogonal、OpenGL/核显或净包验收；PLY 旧 C API 的 `exit(-1)` 仍按构建边界单独处理。

### 14.17 本轮旧 Paint 入口与 PLY 错误传播收口（2026-08-14）

本轮继续只修改源码并做 qmake/静态检查，新增以下实现：

- Paint `roiOperation()`、`hatchConnectedRegion()`、`connectedRegion()` 现在在 mask 写入前建立 ROI 范围快照；写入循环按层检查取消，成功才提交快照，取消或异常由 `MaskRegionTransaction` 恢复原始 mask。嵌套 `getVisibleRegion()` 的取消仍通过线程局部作用域传播。
- Paint 连通域/最大组件/旧形态入口的进度对话框统一允许取消，移除组件和标注的统计、重映射、写回阶段增加周期性事件泵检查；取消不会显示完成提示。GraphCut 和 LiveWire 仍保持现有 2D 候选结果提交/交互模型，本轮未把它们错误描述为三维 mask 事务。
- 实际构建 `meshpaint.pro` 和 `meshsimplify.pro` 的两个旧 PLY C API 副本不再执行 `exit(-1)`；增加 `ply_clear_error()`、`ply_last_error()` 和错误消息查询，非法元素/类型、截断读取、规则错误和 close/flush 失败均进入错误状态。meshpaint/meshsimplify 的 PLY 读写入口检查文件、对象、元素、错误状态和写入关闭结果，失败只返回插件操作失败，不终止宿主进程。未修改不在当前插件构建路径中的 `drishti/ply.c`、`tools/mesh/ply.c` 副本。

本轮静态验证：`qmake tools/paint/paint.pro`、`qmake drishti/plugins/meshpaint/meshpaint.pro`、`qmake drishti/plugins/meshsimplify/meshsimplify.pro` 和 `git diff --check` 通过。当前环境仍没有可直接调用的完整 C++ 编译/链接工具链，因此没有把 qmake 通过升级为构建通过。

截至 14.17，源码层面仍明确后置：TIFF IFD 全栈枚举移出 GUI、单次 codec 硬超时进程隔离、所有 Paint 旧整卷/并行入口逐项事务审计、全产品统一崩溃恢复 journal、真实磁盘/短写/rename/delete 和 OpenGL 故障注入、Intel/AMD 核显画面、正式 Import -> Paint -> Drishti GUI 闭环、净环境便携包和最终发布快照。上述代码收口不能替代后续统一构建与验收。

### 14.18 本轮 Paint 并行旧入口与 PLY 生命周期继续收口（2026-08-14）

本轮继续只做源码修改和静态/qmake 检查，没有启动正式构建、GUI、OpenGL、Intel/AMD 核显或便携包验收：

- `VolumeOperations::resetT()` 现在在并行 worker 写入 mask 前建立范围快照，使用共享原子取消标志传播进度对话框取消；worker 完成后只有在全部任务成功且未取消时才提交，取消/worker 取消由 `MaskRegionTransaction` 恢复原始 mask。入口增加边界合法性检查。
- `getConnectedRegionFromBitmask()`、`getRegionConnectedToROI()` 的洪泛循环增加周期性取消检查；`getTransparentRegion()` 的并行 worker 增加共享原子取消传播，取消后清空临时 bitmask，避免上层把部分结果继续用于写回。`openCloseBitmask()` 和 `_dilatebitmask()` 在距离变换和逐 voxel 扫描之间增加取消检查；这些辅助函数仍属于临时 bitmask 计算，不等同于所有调用入口都已具备字节级事务。
- PLY 实际构建路径的 `meshpaint/meshsimplify` C API 对空输入、坏 header、坏 format、输出文件打开/分配失败补充错误状态；`open_for_reading_ply()` 在解析失败时关闭已打开的文件，避免失败路径泄漏。此前的 `exit(-1)` 移除、错误查询接口和插件入口 close/flush 检查保持不变。

本轮静态验证：`qmake tools/paint/paint.pro`、`qmake drishti/plugins/meshpaint/meshpaint.pro`、`qmake drishti/plugins/meshsimplify/meshsimplify.pro` 和 `git diff --check` 通过。当前环境没有可直接调用的完整 C++ 编译/链接工具链，因此 qmake 通过不记为 C++ 构建通过；本轮还未执行 GUI、驱动或真实故障注入。

截至 14.18，源码层面仍明确后置：Paint 其余旧整卷入口和所有并行写入点的逐入口事务核验、GraphCut/LiveWire 的完整取消/失败语义、TIFF IFD 全栈枚举移出 GUI和单次 codec 硬超时进程隔离、全产品统一崩溃恢复 journal、真实磁盘/短写/rename/delete、OpenGL allocation/FBO 故障注入、Intel/AMD 核显画面、正式 Import -> Paint -> Drishti GUI 闭环、净环境便携包和最终发布快照。上述源码收口仍不能替代后续统一构建与验收。

### 14.19 本轮代码收口（2026-08-14，构建与验收后置）

本轮继续只完成可由源码确认的修改，没有把 qmake 生成升级为 C++ 构建，也没有启动正式 GUI、OpenGL、Intel/AMD 核显、故障注入或便携包验收：

- `VolumeOperations::stepTags()`、`mergeTags()` 现在具有明确的取消作用域和 `MaskRegionTransaction`；取消或异常由析构回滚，成功才提交。此前 `mergeTags()` 末尾残留的未声明 `maskTransaction.commit()` 已修正。
- 旧整卷形态学入口 `dilateAll()`、`openAll()`、`closeAll()`、`erodeAll()` 和并行 `writeToMask()` 已补齐区域快照、嵌套取消传播及成功提交；并行 worker 通过共享原子取消标志停止继续写入，取消后事务回滚。
- `dilateAllTags()`、`tagTubes()` 的写回阶段增加事务提交和取消检查；`modifyOriginalVolume()` 使用独立的 `VolumeRegionTransaction` 对原始 8/16-bit 体数据做字节级快照，避免将 mask 事务错误用于原始体数据。
- PLY 实际构建路径的 `meshpaint/meshsimplify` `open_for_writing_ply()`、`ply_open_for_reading()` 成功/失败路径释放临时扩展名缓冲；此前的错误状态传播、close/flush 检查和 `exit(-1)` 移除保持不变。

本轮静态验证：`qmake tools/paint/paint.pro`、`qmake drishti/plugins/meshpaint/meshpaint.pro`、`qmake drishti/plugins/meshsimplify/meshsimplify.pro`、`git diff --check` 通过；对上述函数做了事务声明/提交一致性扫描，实际构建路径的两个 PLY 副本 `exit()` 扫描无结果。当前环境仍缺少可直接调用的完整 C++ 编译/链接依赖，因此不能声称全工程构建完成。

截至 14.19，仍明确后置：GraphCut/LiveWire 的完整失败语义和全部非活动旧入口逐项审计、TIFF 单次 codec 硬超时进程隔离、全产品统一崩溃恢复 journal、真实磁盘/短写/rename/delete、OpenGL allocation/FBO 故障注入、Intel/AMD 核显画面、正式 Import -> Paint -> Drishti GUI 闭环、净环境便携包和最终发布快照。源码完成到本节不等于构建通过或运行时全链路验收通过。

### 14.20 本轮 GraphCut/LiveWire/PLY 失败路径收口（2026-08-14，构建与验收后置）

本轮继续只完成可由源码确认的修改：

- GraphCut 的 `Graph::maxflow()` 增加可选取消回调；主增长循环、source/sink 邻接扫描、orphan adoption 和建图阶段均周期性检查共享原子取消标志。`ImageWidget::applyGraphCut()` 显示可取消进度对话框，取消或异常只保留原标签，不提交部分结果；GraphCut 算法准入同时持有进程级 `ProcessMemoryReservation`。
- LiveWire 增加图像/梯度/代价缓冲的尺寸、分配失败状态和错误文本；Dijkstra 路径计算、回溯和 Escape 取消增加事件泵与取消标志，非法/空图像不会继续访问空缓冲。其余曲线交互仍需正式 GUI 回归。
- 实际构建路径 `meshpaint/meshsimplify` 的 PLY C API 失败生命周期继续收口：解析器严格要求 `format`、`element`、`property`、`end_header`，失败时释放已分配的 header/element/property/comment/rule/other-element 结构；other 数据和 list/string 指针先零初始化，截断 ASCII/Binary element 不会沿未初始化指针释放。mesh loader 对顶点/面数组、单元素分配、读取错误和 `vcolor` 分配失败均回收候选对象并返回失败。

本轮验证：

- `qmake tools/paint/paint.pro`、`qmake drishti/plugins/meshpaint/meshpaint.pro`、`qmake drishti/plugins/meshsimplify/meshsimplify.pro` 通过；`git diff --check` 通过。
- MSVC 实际编译通过：`tools/paint/graphcut/graphcut.cpp`、`drishti/plugins/meshpaint/ply.c`、`drishti/plugins/meshsimplify/ply.c`。完整 Paint/LiveWire 编译在当前环境缺少 `QGLViewer/qglviewer.h` 处阻断，不能升级为工程构建通过。
- 未执行正式 GUI、OpenGL、真实故障注入、Intel/AMD 核显、便携包和最终闭环验收。

截至 14.20，仍明确后置：LiveWire 正式交互/曲线保存重开证据、TIFF 单次 codec 硬超时进程隔离、全产品统一崩溃恢复 journal、真实磁盘/短写/rename/delete、OpenGL allocation/FBO 故障注入、Intel/AMD 核显画面、正式 Import -> Paint -> Drishti GUI 闭环、净环境便携包和最终发布快照。源码收口完成不等于构建或运行时全链路通过。

### 14.21 本轮代码完成收口（2026-08-14，构建与验收继续后置）

本轮只完成源码修改和静态/qmake 检查，未启动正式构建、GUI、OpenGL、核显、故障注入或便携包验收：

- Paint 旧入口 `shrinkwrap()`、`poreCharacterization()`、`sortLabels()` 增加区域级 `MaskRegionTransaction`，所有临时区域识别、形态处理和逐 voxel 写回阶段响应取消；取消或异常由事务析构恢复原始 mask，完整成功才提交。
- LiveWire 调用方 `CurvesWidget` 现在检查图像/LOD/梯度重建及鼠标路径计算返回值；失败会停止当前 LiveWire 模式并向状态栏传播 `errorMessage()`，不会继续显示或保存不完整曲线。
- TIFF 实际像素解码移至新增 `tools/import/plugins/tiffdecodehelper/` 独立 helper。插件通过 `QProcess` 传递文件、IFD、行范围和行字节数，读取时执行取消检查、30 秒硬超时、退出码检查和短/长输出校验；超时会杀掉 helper、清零调用方缓冲并返回错误。helper 作为 `tools/import/plugins/plugins.pro` 子工程构建，TIFF 插件依赖 helper。
- helper 路径支持 `DRISHTI_TIFF_HELPER` 覆盖，默认查找应用目录及上级目录，便于便携包和故障注入环境显式指定。

本轮静态验证：`qmake -o NMakefile.code-source-final tools/paint/paint.pro`、`qmake -o NMakefile.tiff-helper-final tools/import/plugins/plugins.pro`、`qmake -o NMakefile.tiff-plugin-final tools/import/plugins/tiff/tiff.pro` 成功；相关文件 `git diff --check` 成功。当前环境未执行完整 C++ 构建/链接，不能把 qmake 通过写成工程构建通过。

代码层面本轮已完成到可交给构建/验收阶段的状态。仍必须由后续统一阶段验证：完整 MSVC/Qt 构建和部署 helper、正式 Import -> Paint -> Drishti GUI 闭环、LiveWire/曲线保存重开、真实磁盘满/短写/rename/delete、OpenGL allocation/FBO、Intel/AMD 核显、净环境便携包以及最终发布快照。这些是运行时证据，不在本轮源码完成度内。

### 14.22 本轮源码最终收口（2026-08-14，构建与验收后置）

本轮继续只完成可编码修改和静态核对，没有启动完整构建或正式运行时验收：

- 公共 `common/src/recoveryjournal.*` 新增 direct-output 事务模式，仍使用同一 JSON `STAGING/PREPARED/COMMITTED` 协议；它保护已有目标的 backup，允许 Import 继续写入原最终路径，提交标记写入后才清理旧代。恢复、路径/重复目标/同目录校验、journal 损坏阻断和失败回滚均由公共实现负责。
- Import `Raw2Pvl` 的 Save As、Time Series/Batch、MHD/RAW 和 Merge 的 `BatchPathJournal` 已完全改为调用公共 `RecoveryJournal`；旧的 Import 专用二进制 journal/恢复实现已移除，公共组件保留了一次性旧格式迁移恢复，避免升级后静默忽略历史残留。启动新操作时如果残留 journal 无法恢复，入口现在直接阻断，不会继续覆盖可能无法恢复的输出。
- LiveWire 的 seed propagation、seed 移动、shape/ellipse/polygon 更新现在检查所有路径计算返回值；失败会恢复本次交互前的 polygon/seed 状态，非法 seed、空 shape、零轴椭圆和不可达路径会进入错误状态，不再把部分曲线继续提交。
- 实际构建的 `meshpaint/meshsimplify` PLY C API 继续保持非致命失败语义；本轮同时完成 `LiveWire` 新返回值调用点和公共恢复 API 的接口静态一致性核对。未修改仅存在于仓库但不在当前插件构建路径中的旧 `drishti/ply.c` 副本。

本轮静态检查：相关文件 `git diff --check`、公共 journal/Import/LiveWire API 引用扫描、构建工程源文件接入扫描通过；没有执行 C++ 编译/链接。当前代码可交给下一阶段统一构建和验收，不能把源码收口写成全链路运行通过。

截至 14.22，剩余均属于后置构建/运行时证据：MSVC/Qt/第三方依赖完整构建与 helper 部署、Import -> Paint -> Drishti 正式 GUI 闭环、LiveWire/曲线重开、TIFF helper 运行验证、短写/磁盘满/rename/delete/异常退出注入、OpenGL allocation/FBO 故障注入、Intel i7-13700H 与 AMD 核显、净环境便携 ZIP 和最终发布快照。

### 14.23 本轮代码收口补充（2026-08-14，核验规划前）

本轮在进入构建/运行核验前又完成了一次针对新增实现的静态复核，发现并修正了两个会影响真实链路的源码边界：

- Import provenance 现在由 `VolumeData::sourceFiles()` 提供。TIFF 和 Image Stack 通过独立的可选 `SourceFilesProvider` 接口返回实际解析后的有序切片列表；目录导入不再只记录目录名，显式多文件导入也不再被 TIFF 插件二次按文件名排序，因此 `FilesListDialog` 的用户确认顺序会一路保留到 PVL `<sourceorder><file>...</file></sourceorder>`。该能力没有改变既有 `VolInterface` 虚表，未实现 provider 的旧插件仍回退到原始输入列表。
- Paint `VolumeFileManager` 的 dirty-chunk immutable snapshot 增加 baseline 失效边界：切换 memory-mapped 状态、创建新内存卷、加载压缩/普通卷后都会删除旧 baseline；同步创建/迁移入口不会复用其他卷的 snapshot。保存期间若 generation 发生变化，dirty 集仍保留到下一代刷新；取消、短写、worker 失败仍保留内存中的 dirty 数据。
- PVL manifest smoke 新增结构化 `sourceorder`（含空格路径）断言；TIFF orientation smoke 追加显式重排顺序和 provider provenance 断言；Import、TIFF、Image Stack、Paint 相关工程的 qmake 生成和受影响文件 `git diff --check` 均通过。

本节的“代码收口”只表示上述缺口已有源码实现和静态证据，不表示 C++ 编译、链接、helper 执行、GUI、OpenGL、目标核显或便携包通过。第 14.22 列出的正式构建、故障注入、硬件矩阵、净包和 Import -> Paint -> Drishti 闭环仍必须按下一阶段核验规划逐项取得证据；共享 `RecoveryJournal` 已统一底层协议，但各产品输出域仍需用正式崩溃/磁盘故障测试证明恢复语义。

### 14.24 本轮代码收口与已取得的局部构建证据（2026-08-14，正式核验仍后置）

本轮补齐了 TIFF helper 在 Windows 上的二进制输出边界：`tools/import/plugins/tiffdecodehelper/main.cpp` 将 stdout 切换为 `_O_BINARY`，避免像素字节 `0x0A` 被 CRT 文本模式转换成 `0x0D 0x0A`，从而造成 helper 输出长度漂移。helper 的短写/长写、退出码、取消和 30 秒硬超时检查仍由 TIFF 插件负责；当前 helper 实际构建产物位于 `C:\bin\tiffdecodehelper.exe`，正式便携包必须重新部署并记录其 hash。

在 VS2019 BuildTools 14.29 + Anaconda Qt 5.15.2 环境中，以下独立 MSVC 构建和运行证据已取得：

- `PVL manifest smoke passed`（包含结构化 `sourceorder`）；
- `TIFF top-left/bottom-left orientation smoke passed`（包含显式文件顺序和 `SourceFilesProvider` provenance）；
- `ImageStack transactional plugin smoke passed`；
- `Slab save transaction smoke passed`；
- `Graph Cut memory admission smoke passed`；
- `Algorithm memory admission smoke passed`；
- `Framebuffer budget smoke passed`；
- `Paint slice ordering smoke passed: 8/16-bit reversal, guards and overflow`；
- `Import memory admission smoke passed`；
- Sampling contract smoke 返回 0；Binary PLY writer smoke 构建并运行返回 0。

这些是独立组件的 MSVC 编译/运行证据，不等于三套主工程或全链路通过。完整 Import 工程实际编译在 `pybind11/embed.h`、`pybind11/pybind11.h` 和默认 qmake 路径缺少 `tiffio.h` 处阻断；TIFF 独立插件只有在显式传入 Anaconda `TIFF_INCLUDE_PATH`/`TIFF_LIBRARY_PATH` 后成功构建。Paint 的 VFM lifecycle smoke 在 `QGLViewer/qglviewer.h` 处阻断。上述依赖阻断记录为环境/构建前置，不再继续通过无关源码猜测规避。

截至本节，仍没有正式 Import -> Paint -> Drishti GUI 闭环、Paint 保存/关闭重开、Drishti 项目加载和最终渲染、真实磁盘满/短写/rename/delete/异常退出注入、OpenGL allocation/FBO 故障注入、Intel i7-13700H 与 AMD 核显实机证据、净环境便携 ZIP 依赖闭包和发布快照 hash。下一步进入核验阶段时，必须按“依赖闭包 -> 确定性 smoke -> Import GUI -> Paint 持久化 -> Drishti/OpenGL -> 故障注入 -> Intel/AMD 硬件 -> 净包”顺序记录证据，不能把 qmake、局部 smoke 或 helper 单独运行写成全链路通过。

### 14.25 本轮 VFM 运行边界修补（2026-08-14，正式核验仍后置）

上一轮定向编译发现 VFM lifecycle smoke 使用 `QCoreApplication`，但实际 VFM 保存快照路径会创建 `QProgressDialog`；Windows 上该测试宿主在 Qt5Core 内以 `0xC0000409` 退出，不能算作通过。本轮将 smoke 改为 offscreen `QApplication`，并将 `VolumeFileManager::createSaveSnapshot()` 的 immutable snapshot 物化从未诊断的 `QFile::copy()` 改为受检分块读写、flush、尾字节和最终尺寸检查；失败信息现在包含源/目标路径、尺寸和 QFile 错误。

修补后，VFM lifecycle smoke 在 VS2019 BuildTools 14.29 + Anaconda Qt 5.15.2、Anaconda blosc 头/库路径下实际编译并运行通过：`VFM lifecycle smoke passed`，返回码 0。该结果关闭了 snapshot baseline 的一个真实 Windows 运行阻断，但仍只是 Paint 生命周期组件证据。

当前源码交接边界保持不变：完整 Import 仍需补齐 OpenVDB、TIFF、pybind11 和其链接库后才能编译；主工程 GUI、OpenGL/FBO、真实磁盘/短写/rename/delete/异常退出、Intel/AMD 核显和净便携包尚未验收。代码阶段至此交给统一核验阶段，不把局部 smoke 结果扩大为全链路通过。

### 14.26 阶段 2 确定性 smoke 复核（2026-08-14）

在当前源码工作树和 VS2019/Anaconda Qt 环境下重新执行了确定性 smoke。以下程序均返回 0：

- `pvl_manifest_smoke.exe`：`PVL manifest smoke passed`；
- `tiff_plugin_orientation_smoke.exe release/importplugins/tiffplugin.dll`：`TIFF top-left/bottom-left orientation smoke passed`；
- `imagestack_plugin_smoke.exe release/importplugins/imagestackplugin.dll`：`ImageStack transactional plugin smoke passed`；损坏 PNG fixture 的 libpng 错误为预期负例输出，回滚断言通过；
- `slabsavetransaction_smoke.exe`、`vfm_lifecycle_smoke.exe`；
- `graphcut_memory_admission_smoke.exe`、`algorithm_memory_admission_smoke.exe`、`import_memory_admission_smoke.exe`、`framebuffer_budget_smoke.exe`；
- `slice_order_smoke.exe`：`Paint slice ordering smoke passed: 8/16-bit reversal, guards and overflow`；
- `sampling_contract_smoke.exe`、`binary_ply_writer_smoke.exe`。

TIFF smoke 显式设置 `DRISHTI_TIFF_HELPER=C:\bin\tiffdecodehelper.exe`；Image Stack/TIFF smoke 均使用当前 `release/importplugins` DLL。该阶段只证明可重复的组件契约和负例回滚，尚未证明 Import GUI、Paint GUI、Drishti 渲染、目标核显或便携包。

### 14.27 阶段 3 Import 正式构建阻断（2026-08-14）

使用 VS2019 `VsDevCmd.bat -arch=amd64`、Anaconda Qt 5.15.2，并显式提供 `DRISHTI_EXTRA_INCLUDEPATH=.lab-agent/deps/pybind11/include`、Anaconda Python root 和 `TIFF_INCLUDE_PATH`/`TIFF_LIBRARY_PATH` 重新生成 Import Release Makefile。qmake 生成通过；实际 MSVC 编译在业务源码进入链接前被以下外部头文件阻断：

- `common/src/vdb/vdbvolume.h` 找不到 `openvdb/openvdb.h`；
- 此前 `tools/import/tiffpagevalidation.h` 的 `tiffio.h` 接入缺口已在本轮补入 `tools/import/import.pro`：现在显式提供 `TIFF_INCLUDE_PATH`/`TIFF_LIBRARY_PATH` 后可进入 `tiffpagevalidation.cpp` 编译，独立 TIFF 插件也可构建；
- pybind11 路径显式接入后，未再成为首个阻断。

因此正式 `drishtiimport` GUI 尚未启动，TIFF/Image Stack -> PVL/RAW/MHD/Time Series 的正式菜单链路、取消/预览/保存/重开尚未取得证据。下一步需要先补齐并固定 OpenVDB/VDB/Gmsh/Imath 以及其余 Python/pybind11/TIFF 的同一 MSVC/Qt ABI 依赖闭包，再重做本阶段；不能把本节的 qmake 或插件 smoke 写成 Import 正式链路通过。

### 14.28 阶段 4 Paint 主工程构建阻断（2026-08-14）

使用 VS2019 x64、Anaconda Qt 5.15.2、`.lab-agent/deps/libQGLViewer-2.6.4` 头文件和 Anaconda blosc include/lib 重新生成 `tools/paint/paint.pro`，qmake 生成通过；Release 构建在进入业务 C++ 编译/链接前被工程声明的 `common/lib/vdb.lib` 缺失阻断。VFM lifecycle、Slab、内存准入和 GraphCut 独立 smoke 已通过，但 `drishtipaint` 正式 GUI、Paint 保存/关闭重开、LiveWire/曲线和完整算法矩阵尚未执行。

该阻断与 Import 的 OpenVDB 头/库闭包属于同一外部依赖层问题，不能通过继续改 Paint 业务代码规避。补齐同一 MSVC/Qt ABI 的 OpenVDB/VDB/Gmsh/Imath/Blosc/QGLViewer 后，必须从本节重新生成 Makefile 并取得主工程构建证据。

### 14.29 阶段 5 Drishti 主工程构建阻断（2026-08-14）

使用 VS2019 x64、Anaconda Qt 5.15.2、`.lab-agent/deps/glew-release/glew-2.2.0/include` 和 QGLViewer 头文件重新生成 `drishti/drishti.pro`，qmake 生成通过；Release 编译已越过 `GL/glew.h` 和多个基础渲染源文件，当前首个阻断为 `common/src/videoencoder/ffmpeg.h` 找不到 `libavcodec/avcodec.h`。因此 `drishti.exe` 尚未链接或启动，项目候选加载、shader/FBO、低/高分辨率和最终渲染均未执行。

补齐与 Qt/MSVC 匹配的 FFmpeg 开发头/库后，需要从同一源码快照重新构建，并按阶段 5 的项目资源负例和 OpenGL/FBO 矩阵取得运行证据；不能把已编译若干 `.obj` 或 qmake 结果写成 Drishti 通过。

### 14.30 统一依赖工作区与当前构建快照（2026-08-14）

本节更新并 supersede 14.27--14.29 中“缺少 OpenVDB/FFmpeg/QGLViewer 导致主工程无法构建”的历史阻断记录。当前构建统一使用：

- 依赖根目录：`.lab-agent/dependencies/`；
- Qt 5.15.2：`toolchain/Qt-open/5.15.2/msvc2019_64`；
- Python 3.13：`toolchain/Python313`；
- vcpkg x64：`install/vcpkg/installed/x64-windows`；
- OpenVDB 11.0.0：`install/openvdb-11.0.0`；
- QGLViewer 2.6.4：`install/qglviewer-2.6.4`；
- MSVC runtime DLLs：`install/msvc-runtime`；
- MSVC 14.29 / Windows x64 / Release CRT。

`drishti.pri` 的默认 Windows 路径和两个导入 smoke 工程已改为上述 canonical 前缀；不传旧 `D:/drishti-deps` 参数也能生成指向 `.lab-agent/dependencies` 的 Makefile。`tools/import/plugins/tiffdecodehelper/tiffdecodehelper.pro` 也已接入公共依赖配置，避免独立 helper 丢失 TIFF 头文件。

当前源码快照已实际编译生成：

- `.lab-agent/dependencies/build/main-current/bin/drishti.exe`；
- `.lab-agent/dependencies/build/main-current/bin/drishtiimport.exe`；
- `.lab-agent/dependencies/build/main-current/bin/drishtipaint.exe`；
- `.lab-agent/dependencies/build/main-current/bin/drishtimesh.exe`；
- `bin/importplugins/` 下 16 个 Release 导入插件 DLL；
- `bin/tiffdecodehelper.exe`。

统一前缀消费者验证：两个 canonical smoke 工程均成功编译链接；legacy plugin data-path smoke 返回 0；TIFF input routing smoke 返回 0；插件总工程和导入工具增量重编译均成功。TIFF 栈校验同时接受 top-left/bottom-left 两种已支持方向，混合方向回归已修复并重编 `tiffplugin.dll`、helper 和 `drishtiimport.exe`。

对 `drishti.exe`、`drishtiimport.exe`、`drishtipaint.exe`、`drishtimesh.exe`、`tiffdecodehelper.exe` 和 16 个导入插件执行 `dumpbin /dependents`：20 个二进制的非系统直接 DLL 均可在 canonical Qt/vcpkg/OpenVDB/QGLViewer/Python/MSVC runtime 前缀中定位。Windows API-set、系统 OpenGL 和系统基础 DLL 不计入第三方闭包。

上述内容只关闭依赖闭包和核心二进制构建阻断，不等于运行时全链路验收完成。仍待：便携包从当前快照生成、Qt/第三方 DLL 闭包与 dumpbin 审计、正式 Import GUI 的 TIFF -> ROI -> PVL、Paint 保存/重开、Drishti/OpenGL/FBO、真实磁盘故障注入，以及 Intel i7-13700H 和 AMD 核显设备验收。历史构建日志不得与本节快照混用。

### 14.31 统一依赖安装与 ITK/渲染插件增量构建（2026-08-14）

本轮继续只处理统一依赖工作区和构建前置，不宣称运行时全链路通过。依赖核对结果如下：

- `.lab-agent/dependencies/source`、`build`、`install`、`toolchain`、`downloads`、`stubs`、`licenses` 已作为唯一工作区；Qt、Python、vcpkg、OpenVDB、QGLViewer、GLEW、FFmpeg、NetCDF、TIFF 和 MSVC runtime 均由该前缀提供；旧 `D:\drishti-deps` 内容与 canonical 目录逐项核对一致，当前 qmake 不再消费旧路径。
- 旧路径中散落的 ITK/QGLViewer/OpenVDB/vcpkg/Python 归档已复制到 `.lab-agent/dependencies/downloads/`，依赖许可证归档到 `.lab-agent/dependencies/licenses/`；旧盘副本保留作回滚/来源，不属于当前构建输入。
- ITK 5.0.1 的 x64/MSVC 14.29 Release 头文件、静态库和 CMake package 已安装到 `.lab-agent/dependencies/install/ITK-5.0.1`。由于 ITK 自带源码/构建目录长度限制，canonical 配置保留在 `.lab-agent/dependencies/build/ITK-5.0.1-canonical`，实际已构建的同 ABI 对象由旧短路径构建树安装到 canonical 前缀；不改变源和安装内容归属。
- `drishti/plugins/common/common.pro` 已纳入 `pvlmanifest.*`、`memoryreservation.*`、`recoveryjournal.*`，公共 `common.lib` 已重新编译。
- 当前统一快照成功链接的渲染插件包括：`meshpaintplugin.dll`、`meshsimplifyplugin.dll`、`binarythinningplugin.dll`、`connectedcomponentplugin.dll`、`distancemapplugin.dll`、`smoothingplugin.dll`、`edgepreservingsmoothingplugin.dll` 和 `vedplugin.dll`，均位于 `.lab-agent/dependencies/build/main-current/bin/renderplugins/`（ITK 插件按 `ITK/` 子目录布局）。
- VED 已完成 ITK 5.0.1 源码适配：启用 `ITK_TEMPLATE_TXX=1` 以实例化模板实现，将 `ImageRegionIterator::Begin()` 改为 `GoToBegin()`，并将线程回调改为读取 `MultiThreaderBase::WorkUnitInfo` 与 `ITK_THREAD_RETURN_DEFAULT_VALUE`。本次只证明编译/链接，不代表 VED 已完成动态加载或 GUI 运行验收。

随后对上述八个渲染 DLL 执行 `dumpbin /dependents`，所有非系统直接 DLL 均能在 canonical Qt/vcpkg/QGLViewer/GLEW/MSVC runtime 前缀定位，缺失数为 0。该静态闭包结果仍不替代插件动态加载、OpenGL 上下文和真实 GUI 验收。

上述是依赖/编译证据，不等于 Import -> Paint -> Drishti GUI、OpenGL/FBO、真实磁盘故障、Intel/AMD 核显或净便携包验收。下一步按既定顺序进入正式运行时核验，并在打包阶段再次对最终 ZIP 中的全部 PE 做闭包审计。

### 14.32 本轮运行时验收（2026-08-14）

本轮开始执行正式运行时核验，使用统一快照
`.lab-agent/dependencies/build/main-current/bin`、canonical PATH 和 Qt
`qwindows` 平台插件。测试脚本、最小输入和日志保存在
`.lab-agent/acceptance-20260814/`；它们是验收证据，不是产品发布包。
本节的持久化进程/WER 证据更新并 supersede 前文“未落盘、不得升级为运行失败”的
历史 Harness 记录；两者输入、宿主进程和证据等级不同，不能混用。

#### 已取得证据

1. 四个主程序 `drishtiimport.exe`、`drishtipaint.exe`、`drishti.exe`、
   `drishtimesh.exe` 均能创建进程并在观察窗口内保持响应；`drishtimesh` 的
   主窗口标题可见。该结果只证明宿主进入 Qt/GL 事件循环，不证明数据加载或
   渲染功能通过。
2. Import 正式窗口的 UI Automation 已看到 `Files -> Load -> Files` 下的
   `Analyze 7.6`、`GRD Files`、`Standard Image Files`、`NIFTI Files`、
   `NRRD Files`、`RAW/RAW Slab/RAW Slice Files`、`Grayscale TIFF Image Files`、
   `TXM`、`VGI` 等入口。说明当前 bin 的插件注册和菜单生成至少完成到 UI
   层；原生文件对话框由 Qt 模态调用阻塞自动化，尚未把某个 TIFF 夹具送入
   正式 Save As，因此不能记为 Import GUI 通过。
3. 组件级 smoke 在当前环境重新执行，以下 13 项均返回 `0`：算法/ITK/Import/
   VolumeOperations 内存准入、GraphCut、Framebuffer、mask import、MeshTools
   I/O、slab save transaction、slice order、Undo、VFM lifecycle、video encoder
   和 Binary PLY writer。`meshtools_io_smoke` 的坏路径错误及 Qt 缺少 `lib/fonts`
   仅为预期负例/环境警告；字体目录仍须在最终便携包中补齐并复验。

#### 新发现的运行时阻断

1. **P1：VED 构建成功但 Qt 动态插件加载失败。**
   用原始统一 bin 启动 `drishti.exe` 时，插件注册弹出：
   `Cannot load .../renderplugins/ITK/Smoothing/vedplugin.dll`。
   `vedplugin.dll` 的 PE 直接依赖静态闭包没有缺失，根因已定位到
   `drishti/plugins/itk/ved/ved.h` 缺少其他 Render 插件都有的
   `Q_PLUGIN_METADATA(IID "drishti.render.Plugin.PluginInterface/1.0")`。
   这说明“VED 已构建”和“VED 可被 QPluginLoader 实例化”是两个独立门槛；
   在补 metadata 并重新构建前，不能声称全部渲染插件可用。
2. **P0/P1：正式 Drishti 体数据入口触发访问冲突。**
   为排除 VED 弹窗干扰，将当前 bin 复制到
   `.lab-agent/acceptance-20260814/runtime-no-ved-2`（只删除副本中的 VED DLL），
   用按当前 manifest 契约生成的最小 PVL 夹具 `fixture.pvl.nc`、`fixture8.pvl.nc` 以及对应
   `fixture.xml` 分别通过 `-drishti <path>` 打开。两次均在 12 秒观察期内退出，
   退出码 `0xC0000005`；Windows Application Error 1000/Windows Error
   Reporting 1001 同时记录 faulting module 为该 `drishti.exe`，偏移
   `0x00000000001fd8d7`。这不是正常退出，也不能归因于 VED；需在修复后用同一
   快照重跑 PVL 直接打开和 XML 项目打开，并取得不崩溃、项目提交和 shader/FBO
   证据。

#### 当前仍不能声称

- Import TIFF -> ROI/Z -> PVL/RAW/MHD/Time Series 的正式 GUI 保存、取消和回读；
- Paint mask/标签/curves/snapshot 的 GUI 保存、关闭重开和跨进程恢复；
- Drishti 项目候选提交、体数据加载、低/高分辨率纹理、shader/FBO 和渲染插件
  实例化；
- VED 动态加载和实际算法运行；
- 真实磁盘满/短写/rename/delete/异常退出注入、净便携包闭包、Intel i7-13700H
  首因笔记本和 AMD 核显硬件矩阵。

#### 下一轮最小修复/复验顺序

1. 给 VED 补 `Q_PLUGIN_METADATA`，重建并单独用 `QPluginLoader` 及 Drishti
   插件注册复验；
2. 调试并修复 `drishti.exe` 在最小 manifest 契约 PVL 直接打开时的 `0xC0000005`，先取得
   “不崩溃且体数据提交成功”的进程级证据，再扩展到真实 Import 输出；
3. 重新执行 Import 正式 TIFF 保存、Paint 保存/重开、Drishti 项目加载/渲染和
   VED 算法入口；
4. 最后才进行磁盘故障、OpenGL/FBO、Intel/AMD 核显和同快照净 ZIP 验收。

本节把构建、静态依赖、组件 smoke、GUI 菜单可见性和正式功能闭环明确分层；
本轮验收结论是：**局部运行证据通过，但当前快照不能发布，正式全链路未通过。**

### 14.33 运行时阻断修复复验（2026-08-14）

上一节列出的两个运行时阻断已完成源码修复并在同一当前源码快照重建：

- `drishti/plugins/itk/ved/ved.h` 增加
  `Q_PLUGIN_METADATA(IID "drishti.render.Plugin.PluginInterface/1.0")`。
  验收探针 `.lab-agent/acceptance-20260814/release/plugin_probe.exe` 实际调用
  `QPluginLoader::instance()` 和 `qobject_cast<RenderPluginInterface*>`，返回：
  `instance=true interface-cast=true registration=Vesselness Enhancement Diffusion`。
- `Viewer` 的 LUT 缓冲增加容量跟踪和按 `Global::lutSize()` 的重建保护，避免单体积
  加载把默认 4 层 LUT 切换到 8/16 层后继续访问旧容量。旧隔离副本打开
  `fixture.pvl.nc` / `fixture8.pvl.nc` 均以 `-1073741819`（`0xC0000005`）退出；
  当前 `main-current` 二进制打开两个 PVL 夹具均保持响应，未再出现该退出码。

当前主机的 XML 项目打开已验证为不崩溃，但弹出
`Project load failed / The rendering resources could not be created.`。该结果属于本机
OpenGL/纹理资源链尚未通过，不等于访问冲突仍在，也不等于 Drishti 项目渲染已经通过。
必须在真实 Intel iGPU（以及一台 AMD iGPU）上继续取得 OpenGL Vendor/Renderer、低/高分辨率
纹理、shader/FBO、项目提交和正常退出证据；在此之前不能把本轮标记为全链路发布通过。

### 14.34 全链路复核重跑（2026-08-14）

本轮对同一 `.lab-agent/dependencies/build/main-current/bin` 快照重新执行已有验收脚本，
结果如下。该节只记录复核证据，不把组件级结果升级为产品工作流通过。

#### 已复核通过

1. `run_component_smoke.ps1` 的 13 项组件 smoke 全部返回 `0`：算法/ITK/Import/
   VolumeOperations 内存准入、GraphCut、Framebuffer、mask import、MeshTools I/O、
   slab save transaction、slice order、Undo、VFM lifecycle、video encoder 和 Binary
   PLY writer。MeshTools 的坏路径输出属于预期负例；Qt 缺少 `lib/fonts` 仍是便携包告警。
2. 八个渲染插件逐个经 `QPluginLoader::instance()` 和
   `qobject_cast<RenderPluginInterface*>` 通过，注册名包含：Mesh Repaint、Mesh
   Simplify、Skeletonization、Connected Component Labeling、Signed Distance Map、
   Edge Preserving Smoothing Filters、Smoothing Filters、Vesselness Enhancement
   Diffusion。VED 的动态加载修复在当前构建中保持有效。
3. 新二进制直接打开 `fixture.pvl.nc` 和 `fixture8.pvl.nc` 均在观察期保持响应，未再出现
   旧副本的 `0xC0000005`；这只证明访问冲突回归未重现，不证明完整渲染成功。
4. 四个主程序均能进入 Qt 事件循环并在观察期保持响应；`drishtimesh` 可见主窗口。
   XML 项目打开不再崩溃，但会弹出 `Project load failed / The rendering resources could
   not be created.`。
5. Import 菜单的 `Files -> Load -> Files` 已显示 TIFF、RAW、NIFTI、NRRD、标准图像等
   入口。菜单注册证据通过，但 Qt 文件选择器的模态调用仍阻塞当前 UI Automation
   harness，尚未完成 TIFF 选择、ROI/Z 和 Save As PVL。
6. `pvl_manifest_smoke`、`sampling_contract_smoke`、`import_memory_admission_smoke`
   和 TIFF input routing smoke 返回 `0`。独立 ImageStack smoke 使用当前打包方式仍
   暴露一项待分级问题：测试在生成 PNG fixture 后收到 `libpng error: IDAT: incorrect
   data check` 并异常退出；需要在规范化 Qt/plugin 运行时和净包中复现，不能忽略或写成通过。
   TIFF orientation smoke 需要产品目录旁的 `platforms/qoffscreen.dll`，当前构建 bin
   尚未形成最终便携包，因此该测试尚未取得有效通过证据。

#### 本轮仍未完成

- Import 正式 TIFF 选择、ROI/Z 裁剪、Save As PVL/RAW/MHD/Time Series、取消/覆盖/失败
  回滚和输出回读；
- Paint 的真实 mask/tag/curve 保存、关闭重开和跨进程恢复；
- Drishti 项目候选提交、低/高分辨率纹理、shader/FBO、渲染资源成功创建和正常退出；
- VED 算法入口实际运行；
- 磁盘满、短写、只读、rename/delete、异常退出以及 OpenGL allocation/FBO incomplete
  故障注入；
- 最终净便携 ZIP 的 Qt platform/fonts、插件和第三方 DLL 闭包审计；
- 目标 Intel i7-13700H 核显和至少一台 AMD 核显硬件矩阵。当前审计机为 Intel UHD 770
  加两张 RTX 3090，不能替代目标设备。

因此截至本节，结论仍是：**局部组件、插件加载、主进程响应和 PVL 访问冲突修复已复核；
正式 Import -> Paint -> Drishti -> 便携包全链路尚未完成，不能交付“全链路验收通过”。**

### 14.35 剩余自动化测试重跑与 canonical ABI 复核（2026-08-14）

本节记录本轮把现有可自动化测试尽量跑完后的结果。所有新编译的 smoke 均使用
`.lab-agent/dependencies/toolchain/Qt-open/5.15.2/msvc2019_64/bin/qmake.exe`、
VS2019 x64 和统一 `.lab-agent/dependencies` 前缀；旧 NMakefile 不作为当前快照的
构建证据。

#### 通过

1. 13 项组件 smoke 在 canonical PATH 下全部返回 `0`；另行重跑的
   `imagestack_contract_smoke`、`plugin_error_bridge_smoke`、
   `volume_plugin_validation_smoke`、`pvl_manifest_smoke`、`sampling_contract_smoke`、
   legacy plugin data-path 和 TIFF input routing 也全部返回 `0`。
2. 以 canonical Qt 重建并运行的 Import/格式 smoke 全部通过：ImageStack transactional、
   Volume file transaction、RAW file safety、plugin lifecycle、plugin empty-input、
   native RAW、native RAW collections、NRRD、NIfTI、TXM、DICOM、DICOM histogram、
   MetaImage path、volume value mapping、TIFF-to-volume-file 和 Python Import script。
   TIFF-to-volume-file 夹具为 3 个 4x4 TIFF，保存 2 层，输出事务和 SHA-256 回读通过。
3. Windows 桌面会话的 `desktop_opengl_context_smoke` 通过，报告：
   `vendor=Intel`、`renderer=Intel(R) UHD Graphics 770`、OpenGL 4.5 compatibility
   profile、`max_3d_texture=2048`。这证明本机 Intel 驱动可以创建 Drishti 所需的桌面
   OpenGL 上下文；offscreen 模式下同一测试失败是没有桌面上下文的环境结果。
4. 四个主程序、PVL/PVL8 直接打开和渲染插件加载结果保持 14.34 的结论：主进程能响应，
   访问冲突未回归，8 个渲染插件可由 `QPluginLoader` 实例化；这仍不等于项目渲染闭环。

#### 发现的问题或阻断

1. **TIFF orientation smoke 当前有真实逻辑失败。** canonical Qt 重建后不再出现
   `Qt5Core.dll + 0x204e8 / 0xC0000409`，但 bottom-left 与 top-left 混合方向的栈被
   接受，测试返回 `1`（`A mixed top-left/bottom-left TIFF stack was accepted`）。
   这说明此前的 Qt5Core 崩溃是 smoke 与运行时 Qt ABI 混用造成的无效证据，而混合方向
   接受是当前 TIFF 实现仍需处理的真实缺陷。
2. 旧 `release/tiff_plugin_orientation_smoke.exe` 和
   `release/imagestack_plugin_smoke.exe` 仍会在 canonical Qt 运行时触发
   `Qt5Core.dll` 的 `0xC0000409`；其旧 Makefile 链接 `Qt5*_conda.lib`，因此不能再用作
   产品失败证据。canonical 重建的 ImageStack smoke 已返回 `0`。
3. 当前源码重建的 `volume_chain_harness` 返回 `1`，不是访问冲突。它报告 4 个失败计数：
   `missing_pvlnames_rejected` 和 `duplicate_pvl_paths_accepted` 的旧缺陷未重现（断言
   已过时）；`missing_slab_rejected` 仅因错误文案从 `cannot open` 变为统一的
   `missing, malformed, or unexpected size` 而计数；仍确认的行为缺口是带空格的显式
   PVL slab 名被拒绝，以及损坏 tag header 时 `saveTagNames()` 没有失败状态。该 harness
   还把大体积准入的旧阈值预期写死，当前预算模型变化导致其第四个计数；需更新断言后再
   作为正式 Paint 链路门槛使用。
4. `desktop_opengl_context_smoke` 只覆盖本机 Intel UHD 770；本机 CPU 是 i9-14900K，
   不是目标 i7-13700H，也没有 AMD 核显，因此不能替代目标硬件矩阵。
5. `package_windows_portable.ps1` 使用 canonical 主程序直接执行仍在第 244 行停止，
   因 `main-current/bin/python` 缺失。canonical PATH 下 29 个 EXE/DLL 的直接依赖静态
   审计为 `missing_count=0`，但这不等于净 ZIP 已生成；最终包仍缺 Python 嵌入目录、
   Qt platform/fonts、插件闭包和脚本/许可证闭包的打包证据。

#### 本轮仍未闭环

- Import 正式 GUI 的 TIFF 选择、ROI/Z、PVL/RAW/MHD/Time Series Save As、取消/覆盖/失败
  回滚和输出回读；文件选择器模态调用仍使 UI Automation 超时；
- Paint 正式 GUI 的 mask/tag/curve 保存、关闭重开和跨进程恢复；
- Drishti XML/PVL 项目成功提交、低/高分辨率纹理、shader/FBO、真实渲染资源创建；当前
  XML 夹具不崩溃但仍提示 `The rendering resources could not be created.`；
- VED 算法入口、真实磁盘满/短写/只读/rename/delete/异常退出和 OpenGL allocation/FBO
  故障注入；
- canonical 快照净便携 ZIP、目标 Intel i7-13700H 和 AMD 核显硬件验收。

因此本轮结论更新为：**可自动化的组件、格式和 Intel 桌面 OpenGL smoke 已尽量补跑；
TIFF 混合方向、Paint harness 断言/保存状态、正式 GUI、Drishti 渲染、故障注入、净包和
目标硬件仍未通过，不能交付全链路验收通过。**

### 14.36 便携包闭环修复与当前快照复核（2026-08-15）

本轮继续处理 14.35 中仍能由代码和当前构建环境关闭的真实缺口，并重新使用同一
`.lab-agent/dependencies/build/main-current/bin` 快照打包。以下结果 supersede 14.35
中关于便携包缺少 Python/Qt/插件闭包以及 TIFF 混合方向失败的旧记录。

#### 已修复

1. `package_windows_portable.ps1` 现在只从 `.lab-agent/dependencies` canonical 前缀
   解析 Qt、vcpkg、OpenVDB、QGLViewer、Python 和 MSVC runtime；复制 `tiffdecodehelper.exe`
   到包根目录，并把它纳入最终 PE 闭包审计。这样实际应用目录下的 TIFF 插件无需设置
   `DRISHTI_TIFF_HELPER` 即可找到隔离解码 helper。
2. 包脚本将 Windows `comctl32.dll`/`comdlg32.dll` 作为系统 inbox DLL 处理，修复
   `tk86t.dll` 静态闭包审计的误报；同时将空 MOP 插件集合保持为稳定数组，避免严格
   PowerShell 模式下 `.Count` 失败。
3. 两个 Python `_pth` 文件显式加入 `Lib\site-packages`，修复净包中 `python.exe` 能
   启动但 `import numpy` 失败的问题；Python 自动 `site` 导入仍保持关闭。
4. 包内保留 `lib/fonts/.keep` 路径，Qt 使用主机字体时不再产生缺失字体目录警告；没有
   把无法确认再分发许可的 Windows 系统字体复制进包。
5. `PORTABLE_README.md` 已同步当前清单：16 个 Import 插件、8 个 Render 插件（含
   VED）、无 MOP 插件，以及包根目录的 TIFF helper。

#### 当前快照证据

1. `.\package_windows_portable.ps1 -KeepStage` 成功生成：
   `.\lab-agent\package\drishti-cpu-igpu-release-2026-08-15-audited.zip`；
   SHA-256 为
   `756bb33058f86ce430d77c7507ee5d5c4625876140e8350ee8422db2a6aeb222`，包内 6184 个
   文件，216 个 x64 PE，16 个 Import 插件、8 个 Render 插件、0 个 MOP 插件、5 个
   帮助文档和 10 个脚本描述符。全部 PE 的 `dumpbin /dependents` 非系统依赖闭包审计
   通过，ZIP 中实际包含 `tiffdecodehelper.exe`、`lib/fonts/.keep` 和 VED DLL。
2. 13 项组件 smoke 在 canonical PATH 下全部返回 `0`。将 `meshtools_io_smoke` 放在
   当前 stage 中运行也返回 `0`，且不再有 Qt 缺失 `lib/fonts` 警告。
3. 当前 stage 中的 ImageStack transactional smoke 和 TIFF top-left/bottom-left
   orientation smoke 均返回 `0`。ImageStack smoke 输出的 `libpng error: IDAT...`
   来自其故意损坏 PNG 的回滚负例，最终结果为 `ImageStack transactional plugin smoke
   passed`，不能记录为产品失败。TIFF smoke 未设置 helper 环境变量时也返回 `0`，证明
   包根目录默认 helper 路径有效。
4. 净包 `python\python.exe --version` 返回 Python 3.13.2，`import numpy` 返回 NumPy
   2.4.1；使用该解释器执行 `python_import_scripts_smoke.py` 返回 `Python Import script
   smoke passed`。
5. 8 个 Render DLL 在当前 package stage 逐个通过 `QPluginLoader::instance()` 和
   `qobject_cast<RenderPluginInterface*>`，包括 `Vesselness Enhancement Diffusion`。
6. 当前 package stage 的 `drishtiimport.exe`、`drishtipaint.exe`、`drishti.exe`、
   `drishtimesh.exe` 均能进入 Qt 事件循环；`fixture.pvl.nc`、`fixture8.pvl.nc` 和
   `fixture.xml` 观察期内均未出现 `0xC0000005`。`drishti-runtime.log` 记录了低/高
   分辨率加载、pass-through/copy/reduce/extract/default/blur/backplane shader 步骤、
   纹理句柄 `1/1` 和 scene reset 完成，说明本机桌面 OpenGL 资源链已越过此前的失败点。

#### 14.35 旧结果的分级修正

- TIFF 混合 top-left/bottom-left 的真实逻辑缺陷已由当前实现和 canonical smoke 关闭；
  旧 `Qt5Core.dll` 崩溃属于 ABI 混用的旧测试产物，不再作为当前产品失败证据。
- `volume_chain_harness` 中“带空格显式 PVL slab 名必须拒绝”是过时断言：当前实现正确
  接受合法的空格路径。缺失 slab 的失败文案已统一为“missing, malformed, or unexpected
  size”。`Volume::saveTagNames()`/`VolumeMask::saveTagNames()` 在损坏 tag header 时
  返回 `false`、保留原文件且通过 `lastError()` 传播；旧 harness 只忽略返回值并把它标成
  “silent failure”，不代表当前 API 没有失败状态。

#### 当前仍需外部验收

以下不是本机可凭静态或组件 smoke 代替的编码缺口：正式 Import GUI 的 TIFF 选择、ROI/Z
裁剪和 Save As PVL/RAW/MHD/Time Series 回读；Paint GUI 的 mask/tag/curve 保存、关闭重开
和跨进程恢复；真实磁盘满/短写/只读/rename/delete/异常退出注入；目标 Intel i7-13700H
笔记本和至少一台 AMD 核显硬件矩阵。当前代码和同快照便携包已经达到可交付外部验收的
状态，但在这些设备/正式 GUI 操作完成前，不能把文档结论写成“所有硬件和人工工作流均已
验收通过”。

### 14.37 最终 canonical 组件证据修正（2026-08-15）

严格复核发现，14.35/14.36 中“13 项组件 smoke 均在 canonical PATH 下运行”的表述过宽：
其中 8 个旧 smoke 二进制的 Makefile 曾链接 `Qt5*_conda.lib`。这不影响正式产品二进制，
正式产品的 29 个 PE 没有 Conda 依赖；但旧 smoke 不能作为 canonical Qt 运行证据。

本轮已使用 Qt 5.15.2 MSVC2019 x64、VS2019 x64 和统一
`.lab-agent/dependencies` 前缀重新生成并编译以下 8 个 smoke：
`binary_ply_writer_smoke`、`graphcut_memory_admission_smoke`、
`itk_memory_admission_smoke`、`mask_import_smoke`、
`slabsavetransaction_smoke`、`slice_order_smoke`、`undo_smoke` 和
`vfm_lifecycle_smoke`。`dumpbin /dependents` 确认它们只引用正式的
`Qt5Core.dll`/`Qt5Widgets.dll`，没有任何 `Qt5*_conda.dll`。

随后把 `PATH` 限制为当前便携包根目录及 Windows 系统目录，设置包内 Qt 插件路径，
逐个运行上述 8 个新二进制，全部返回 `0`。VFM 测试最初因 QGLViewer 安装布局不是
`include/QGLViewer` 而是安装根目录下直接包含 `QGLViewer/` 编译失败；修正 include
父目录后成功构建并通过 lifecycle smoke。至此，旧 Conda smoke 证据缺口已经关闭。

打包溯源也同步修正为只统计已跟踪文件的脏状态，并排除 qmake 自动重写、含本机构建路径的
`drishti/drishti_resource.rc`；`.lab-agent`、自动生成 Makefile、对象文件和缓存等明确排除
的本地构建产物不再污染 `BUILD_INFO.txt`。最终 ZIP 必须在本节源码提交完成后重新生成，
并以同目录 SHA-256 校验值为转移依据。

### 14.38 i7-13700H TIFF 导入闪退回归修复（2026-08-17）

目标 i7-13700H 核显笔记本使用 14.36/14.37 对应旧包导入 TIFF 时，在
`Loading TIFF slice` 阶段再次出现 `drishtiimport.exe` 闪退。与最初问题不同，资源管理器
只在 Import 存活期间暂时无法操作，Import 退出后立即恢复；没有再次出现资源管理器持续
卡死。旧包
`.lab-agent/package/drishti-cpu-igpu-release-2026-08-15-audited.zip` 因包含本回归，已失去
最终验收资格，不得继续转移到目标机。

#### 根因和引入范围

该问题不是 OpenGL 或核显渲染崩溃。DrishtiImport 的该入口不使用 OpenGL。根因是旧的
`RemapHistogramWidget::paintEvent()` 路径会在直方图为空时同步发出 `getHistogram()`；
全链路修改又把 TIFF 预览改为 `QtConcurrent` worker、`QFutureWatcher` 和局部
`QEventLoop::exec()` 等待。两者组合形成以下可重入调用链：

`paintEvent -> getHistogram -> setHistogram -> newMapping -> getDepthSliceImage ->`
`TiffPlugin::getDepthSlice -> nested event loop -> recursive repaint`

因此，本次闪退是全链路修改与遗留绘制副作用发生交互后引入的回归，而不是最初 Explorer
持续卡死修复失效。目标机截图中的 Qt active-painter 警告尾部也与该调用链一致。回归首次
进入全链路提交 `66f86bd`，旧包源码提交 `2212919` 仍包含该问题。

#### 修复

1. `RemapHistogramWidget::drawHistogram()` 在空直方图时只返回；绘制函数不再发起同步体数据
   I/O。
2. `RemapWidget::setRawMinMax()` 在首次绘制前主动设置 raw range、histogram 和映射；使用
   `QSignalBlocker` 阻止初始化中的 `newMapping` 提前触发预览，随后直接把映射提交给
   `VolumeData`。`newMinMax()` 中重复设置 histogram 的路径一并删除。
3. TIFF depth/width/height preview 增加共享原子不可重入保护。外层预览读取期间的嵌套读取
   会被拒绝并清零嵌套输出缓冲，外层结果和最终 `lastError()` 不受嵌套失败污染。
4. 三种 TIFF preview 进度对话框设为 application-modal；这不是主要修复，只用于限制用户
   在局部等待期间触发新的应用级交互。

#### 当前自动化证据

- `remap_histogram_widget_smoke` 使用 canonical Qt/VS2019 重建并通过：空直方图绘制发出的
  数据读取请求数为 `0`；常量、全零、单 bin 和窄控件回归保持通过。
- `tiff_plugin_orientation_smoke` 使用新 TIFF DLL 和包内 helper 通过。新增定时嵌套预览
  用例确认内层请求被安全拒绝、内层缓冲清零、外层切片逐像素正确且外层成功状态未被污染。
- `tiffplugin.dll` 和 `drishtiimport.exe` 已在 canonical
  `.lab-agent/dependencies/build/main-current` 中重新编译链接成功。
- 与“每张图片分散导出、Paint 无法打开”直接相关的回归保持通过：
  `pvl_manifest_smoke`、`volume_file_transaction_smoke` 和 canonical
  `vfm_lifecycle_smoke` 均返回 `0`。这证明普通体数据输出仍采用一个 PVL manifest 加
  `.001/.002/...` 连续分片，事务提交后无临时尾文件，并可由 Paint 共用 parser/VFM
  保存和重开。旧 `volume_chain_harness` 的 4 个计数仍来自 14.36 已登记的过时断言，
  不得作为当前产品失败证据。

#### 验收边界

代码和自动化层面已关闭此次可重入闪退，并确认普通 Save As/PVL/Paint 契约未被破坏；但
目标 i7-13700H 上同一组真实 TIFF 的文件选择、首张预览、ROI/Z、普通 Save As 和 Paint
打开仍必须使用 2026-08-17 新包实际走一遍。只有该实机流程不再闪退，且导出目录表现为
一个 `.pvl.nc` 清单和其连续分片，才能把目标机人工验收标为通过。

#### 2026-08-17 新包

四个主程序、16 个 Import 插件、TIFF helper 和 8 个 Render 插件均在本节源码之后强制
重建。打包脚本的产品新鲜度、x64 PE、非系统 DLL 闭包、插件数量、Python/NumPy、帮助和
脚本清单审计通过。独立解压目录中重新运行上述五项关键 smoke，全部返回 `0`；包内
`drishtiimport.exe` 在 offscreen 事件循环保持 5 秒，`drishtipaint.exe` 在真实桌面 OpenGL
会话保持 7 秒，均由测试主动终止。Paint 的 offscreen 启动不作为有效产品证据，因为该
程序要求桌面 OpenGL 上下文。

最终转移文件为：

`C:\saveproject\LBJ-workspace\_external\drishti\.lab-agent\package\drishti-cpu-igpu-release-2026-08-17-ui-fix.zip`

最终 SHA-256 必须以完成本节文档提交后重新打包并复算的值为准；不得再使用 2026-08-15
旧包或其哈希。

### 14.39 i7-13700H 实机否决与 TIFF helper 窗口风暴修复（2026-08-17）

目标 i7-13700H 核显笔记本实测 14.38 的新包时，DrishtiImport 仍在导入阶段闪退，
并连续弹出大量带乱码的控制台窗口，直至必须重启机器。因此以下包及哈希已明确作废，
不得继续测试或交付：

`C:\saveproject\LBJ-workspace\_external\drishti\.lab-agent\package\drishti-cpu-igpu-release-2026-08-17-ui-fix.zip`

`fb0ee911f892a32c5da99f9de51b892df0205e3f57293bd5be5f35c03628ac2e`

#### 新确认的根因与漏测原因

`tiffdecodehelper.exe` 为保留 stdout 二进制解码协议而使用 console subsystem。TIFF 插件
对每个输入文件至少启动一次 metadata helper，初始统计又对每张切片启动一次 decode
helper，随后首张预览还会再次启动；N 张单页 TIFF 因而约有 `2N+1` 次 helper 启动。
Windows GUI 父进程此前没有设置 `CREATE_NO_WINDOW`，目标机为每次启动创建可见控制台，
raw scanline 输出在窗口中表现为乱码。大量窗口创建、焦点切换和进程启动共同拖累桌面
Shell；DrishtiImport 退出后 Explorer 恢复，与本次实机现象一致。该 helper 隔离机制最早
由全链路提交 `66f86bd` 引入，用于给 TIFF codec 提供 30 秒硬超时边界。

14.38 的 orientation 与 offscreen smoke 使用 console 测试父进程和极小 TIFF 栈，子进程
可以继承已有控制台，既没有覆盖 GUI 父进程创建新窗口的行为，也没有形成足够的启动压力。
因此此前“新包可交付实机验收”的结论证据不足。14.38 的绘制副作用和 preview 重入修复
仍是必要修复，但已被实机证明并不充分。

#### 修复

1. Windows 下 TIFF 插件对 metadata/decode 两类 `QProcess` 统一设置
   `CREATE_NO_WINDOW`，同时保留 stdout/stderr pipe 和 console subsystem，不破坏现有二进制
   协议及硬超时隔离。
2. 插件传递 `--no-console-required`，helper 使用 `GetConsoleWindow()` 自检。若未来重构
   丢失无窗口创建标志，第一项 helper 操作会明确失败，不能再次演变成窗口风暴。
3. 插件增加全局 helper 原子 guard，同一进程中任意时刻最多运行一个 TIFF helper；
   `setFile()`/`replaceFile()` 增加整段加载原子 guard，事件循环中的嵌套加载会被拒绝，
   外层成功状态和已提交数据不受污染。
4. `tiff_plugin_orientation_smoke` 新增嵌套加载断言及 128 张重复切片压力段；新增
   `remap_tiff_integration_smoke`，通过正式 `RemapWidget -> VolumeData -> TIFF plugin`
   组合加载 64 张灰度 TIFF，并强制执行首次绘制和 Z/Y/X 三方向预览。它补上了此前把
   UI 与插件分开测试的覆盖缺口。

#### 当前自动化证据

- 128 张压力栈完整通过；5 ms 桌面进程采样记录 helper 并发峰值 `1`、有效采样 `559`
  次、非零 `MainWindowHandle` 样本 `0`、结束残留 `0`。helper 内部无控制台断言也全程
  通过。
- 64 个不同灰度 TIFF 的 `TIFF -> VolumeFileManager` 测试通过，保存 50 层、2 个连续
  slab，回读 SHA-256 为
  `a5e877bb0292789c924b2e95589e512a67a618fdd116b5f694a89d8e0042b809`。
- `RemapWidget -> VolumeData -> TIFF plugin` 64 张组合链连续运行 5 次均返回 `0`；首次
  histogram 绘制及 Z/Y/X preview 全部通过，测试期间 Windows Application Error 新增
  `0` 条，helper 残留 `0`。
- `remap_histogram_widget_smoke`、`volume_file_transaction_smoke`、`pvl_manifest_smoke`
  和 canonical `vfm_lifecycle_smoke` 继续通过。因此“每张图片分散导出、Paint 无法打开”
  对应的 PVL manifest、连续 slab、事务提交及 Paint 共用 VFM 契约没有回归。
- 组合测试开发时曾出现一次 `Qt5Widgets.dll + 0x188a1f / 0xC0000005`；故障偏移对应
  `QStatusBar::clearMessage()`，原因是初版测试夹具未像正式 DrishtiImport 一样注册
  `Global::statusBar()`。补齐夹具初始化后连续 5 次通过。该事件不得误记为产品崩溃证据。

#### 当前边界

本机自动化已覆盖本次新增的窗口风暴、加载重入、UI 首绘/三方向 preview、TIFF 到连续
PVL slab 以及 Paint 共用读取契约。目标 i7-13700H 的原始 TIFF 数据和驱动环境仍无法由
本机完全复制，因此只有从本节源码重新全量构建、打包的新 ZIP 才能进入下一次最终实机
验收；通过前仍不得写成目标硬件已验收。若新包在目标机仍发生闪退，必须取得该次
Application Error/WER 的 faulting module、exception offset 和 dump，不能再把所有
`0xC0000005` 统一归因于同一问题。
