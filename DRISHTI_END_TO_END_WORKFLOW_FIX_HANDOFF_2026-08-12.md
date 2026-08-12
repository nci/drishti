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
| Import 保存 | 单个 VFM 管理的同一 base 全组 slabs 具有进程内 staging、backup/rollback、exact I/O 和取消传播；header 使用 `QSaveFile`；多个调用点检查错误 | VFM 事务无 journal/崩溃恢复，零填充候选会在真实写入前进入最终路径；header + PVL VFM + 可选 RAW VFM 不是一个总事务；空间预检、旧尾 slabs 和正式 Save As 见 P0-I2/P0-I3/P1-I12 |
| Drishti 底层 I/O | VFM 受检容量/I/O、`VolumeBase` 阶段错误传播、候选对象内部回滚、加载失败可退到 `DummyVolume` | 应用层 `preLoadVolume()` 仍先清旧场景，不能保留用户原工作集；时间序列和共享 manifest 见 P0-P4/P0-P7/P0-P15 |
| Paint mask/VFM | 受检 VFM、压缩 mask 单 worker、350 ms debounce、generation 合并、不可变 snapshot、dirty 保留、多 slab journal、Undo、Checkpoint | GUI 线程仍同步写整卷原始 snapshot；临时 snapshot 清扫、tag names/curves/Save Work 总事务和正式重开未闭合 |
| Paint 算法内存 | 实时物理/Commit 模型、GraphCut、六个 ITK profile 和八个原生三维入口的部分准入/回滚 | 固定 `LargeVolume` 门槛仍误拒绝；GraphCut/多项算法仍同步阻塞；LiveWire、其余旧算法、正式 ITK 构建/运行和目标机峰值未闭合 |
| OpenGL/iGPU | Desktop OpenGL Compatibility profile、renderer 状态、shader/FBO 错误传播、延迟资源、纹理/FBO 预算、固定 slab、两套 `ScopedTrisetGlState`、CPU mesh paint | 只能以当前源码复核两套状态守卫，历史“尚未实现”已过时；仍需目标 Intel/AMD 驱动上的全部延迟 shader/FBO、低预算、失败注入和画面验证 |
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
