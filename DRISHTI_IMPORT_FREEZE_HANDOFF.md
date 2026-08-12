# DrishtiImport 图片导入卡死修复交接

> 历史说明：本文记录的是修复实施前的根因审计和验收要求，其中“当前工作树”及实验性 `BATCH_SIZE = 32` 描述属于当时状态。当前实现、构建结果、CPU + 核显路线和目标设备执行顺序以 `CPU_IGPU_IMPLEMENTATION_HANDOFF.md` 为准。

## 0. 给目标设备智能体的执行指令

请在出现问题的 Windows 11 笔记本上完成本文件中的现场确认、代码修复和验证。

目标设备已知条件：

- CPU：Intel Core i7-13700H
- GPU：近五年的 Intel 核显；同类 AMD 核显也属于目标平台
- 症状：使用 Drishti 导入约 10 张图片时，导入阶段卡死；严重时 Windows 资源管理器也失去响应，只能重启
- 用户不是上游仓库作者，不要求向 GitHub 推送；交付本地可构建的修改、补丁和验证结果即可

执行时必须遵守：

1. 先确认目标设备运行的 `drishtiimport.exe` 对应哪个源码版本，不要假定它包含当前工作树中的实验性 `QtConcurrent` 修改。
2. 先做只读 TIFF 元数据检查，再运行单张、小批量测试；不要直接用完整问题数据触发整机卡死。
3. 不要只把 `BATCH_SIZE` 从 32 改小后就宣布修复。必须同时处理输入校验、边界检查、整数溢出、空句柄、UI 阻塞和整切片泄漏。
4. 不要修改三维渲染代码。本问题发生在 `drishtiimport`，该程序的图片导入链不依赖 OpenGL。
5. 保留工作树中与本任务无关的用户修改。开始前先执行 `git status --short` 和 `git diff`。

## 1. 审计基线和结论

审计仓库：`nci/drishti`

审计提交：

```text
b53bd9790829dbeb38cbe0f160e79a95349905d2
2026-06-02
```

审计日期：`2026-08-09`

当前审计机器不是发生卡死的 i7-13700H 笔记本，没有现场原图、崩溃转储或性能日志。因此：

- 可以确认源码中存在多项能够直接造成 UI 假死、内存/页文件风暴和堆越界的缺陷。
- 可以确认图片导入阶段不是核显/OpenGL 性能问题。
- 不能在没有现场文件和日志的情况下诚实地断言只有一个根因。
- 目标设备智能体必须按第 5 节的决策树把现场归入对应分支。

最可能的总体机制是：

```text
GUI 线程同步进入 TIFF 插件
  -> 解码全部切片并扫描直方图
  -> 实验版本同时预分配并解码最多 32 张完整切片
  -> CPU、磁盘和内存提交同时达到峰值
  -> Windows 开始大量换页，磁盘持续 100%
  -> DrishtiImport 和 Explorer 都表现为“未响应”
```

若输入不是严格一致的单通道 TIFF，还可能在上述流程中发生缓冲区写越界，造成堆破坏、异常 OOM 或进程崩溃。

## 2. 导入调用链

常规文件导入的同步调用链如下：

```text
tools/import/drishtiimport.cpp
  DrishtiImport::loadFiles()                         约 266-347 行
    -> tools/import/remapwidget.cpp
       RemapWidget::setFile()                        约 100-223 行
      -> tools/import/volumedata.cpp
         VolumeData::setFile()                       约 252-332 行
        -> VolInterface::setFile()
          -> tools/import/plugins/tiff/tiffplugin.cpp
             TiffPlugin::setFile()                   约 241-268 行
            -> TiffPlugin::setImageFiles()            约 105-238 行
              -> findMinMaxandGenerateHistogram()     约 341-532 行
```

关键事实：`RemapWidget::setFile()` 在 GUI 线程直接调用 `VolumeData::setFile()`，插件又在返回前完成 TIFF 解码和直方图扫描。当前没有真正的后台导入任务。

`tools/import/import.pro` 只启用 Qt Widgets/Core/Gui/XML/Concurrent，没有 Qt OpenGL。导入卡死不应归因于 Intel/AMD 核显的着色器能力。

## 3. 当前工作树状态

审计时工作树已有用户/实验修改：

```text
 M tools/import/plugins/tiff/tiff.pro
 M tools/import/plugins/tiff/tiffplugin.cpp
 M tools/import/plugins/tiff/tiffplugin.h
?? .lab-agent/
?? CPU_ADAPTATION_DESIGN.md
```

其中 TIFF 修改做了以下事情：

- 增加 `QT += concurrent` 和 `#include <QtConcurrent>`。
- 把 `loadTiffImage()` 从 `void` 改为 `bool`。
- strip 读取失败时关闭 TIFF 句柄并返回，避免原实现错误路径泄漏句柄。
- 把原来复用单张缓冲的串行直方图扫描改为最多 32 张一批的 `QtConcurrent::blockingMap()`。

必须保留“错误路径关闭句柄并返回错误”这个正向修复，但不能保留固定 32 张完整切片预分配的设计。

## 4. 已确认缺陷

### F-01：实验版本按图片张数批量预分配完整切片

严重度：Critical

位置：`tools/import/plugins/tiff/tiffplugin.cpp`

- `int nbytes = m_width*m_height*m_bytesPerVoxel;`，约 379 行
- `const int BATCH_SIZE = 32;`，约 387 行
- `batch[i].buf.resize(nbytes);`，约 413 行
- `QtConcurrent::blockingMap(...)`，约 419 行

实验版本先在 GUI 线程给 `min(depth, 32)` 张图片各分配一个完整解码缓冲，再开始处理。对于 10 张图片，10 张缓冲会全部同时存在。

仅计算 16 位像素缓冲，不含 libtiff、Qt、Python、直方图和其他进程：

| 单张尺寸 | 单张解码后 | 10 张 | 32 张 |
|---|---:|---:|---:|
| 4096 x 4096 | 32 MiB | 320 MiB | 1 GiB |
| 8192 x 8192 | 128 MiB | 1.25 GiB | 4 GiB |
| 16384 x 16384 | 512 MiB | 5 GiB | 16 GiB |

压缩后的 TIFF 文件大小不能代表解码后内存。核显还会与 CPU 共享系统内存。

预期现场特征：

- `drishtiimport.exe` Private Bytes/Commit Size 快速上升。
- 系统 Commit 接近上限，Hard Faults/sec 很高。
- 页面文件所在磁盘 100% 活跃。
- Explorer 与其他程序同时变得无响应。

### F-02：`blockingMap()` 仍然阻塞 GUI 线程

严重度：High

`QtConcurrent::blockingMap()` 不是异步 UI 方案。它在整个批次完成前不会返回。当前代码只在批次结束后调用 `qApp->processEvents()`。深度小于等于 32 时，全部图片构成单一批次。

i7-13700H 有很多逻辑处理器，Qt 全局线程池通常会同时运行这 10 个解码任务。结果是多路解压和多路磁盘读取同时发生，GUI 仍然显示“未响应”。

原始 `HEAD` 虽然只复用一张切片缓冲，内存峰值较低，但也在 GUI 线程逐张解码；单张大 TIFF 解码期间同样会失去响应。

### F-03：没有验证 TIFF 栈的一致性和实际解码字节数

严重度：Critical

位置：`TiffPlugin::setImageFiles()` 与 `TiffPlugin::loadTiffImage()`。

当前实现只从第一张 TIFF 取得：

- `TIFFTAG_IMAGEWIDTH`
- `TIFFTAG_IMAGELENGTH`
- `TIFFTAG_BITSPERSAMPLE`

然后按第一张的 `width * height * bytesPerVoxel` 为所有图片分配缓冲。它没有验证：

- 后续文件的宽高是否一致
- `SAMPLESPERPIXEL` 是否为 1
- `SAMPLEFORMAT` 是否与用户选择的有符号/无符号/浮点类型一致
- `PLANARCONFIG`
- strip 与 tiled TIFF
- 每页/每个文件的位深是否一致
- 多页 TIFF 每页的尺寸与格式是否一致
- `TIFFReadEncodedStrip()` 返回的累计字节数是否仍在目标缓冲范围内

如果 RGB TIFF 被错误地交给“Grayscale TIFF”插件，或后续文件更大，strip 数据可写出目标缓冲区。该缺陷能够造成堆破坏，表现不一定是立即崩溃，也可能是假死或异常内存消耗。

多页 TIFF 还有额外问题：代码先用 `TIFFReadDirectory()` 遍历到最后一页计数，随后直接读取宽高和位深，没有显式切回第 0 页。基准元数据可能来自最后一页。

### F-04：`TIFFOpen()` 和 `TIFFSetDirectory()` 结果未完整检查

严重度：High

位置：

- `TiffPlugin::setImageFiles()`，约 132-155 行
- `TiffPlugin::loadTiffImage()`，约 273-304 行

首张 `TIFFOpen()` 失败后，代码仍可能调用 `TIFFGetField()` 和 `TIFFClose()`。后续图片打开失败后，也会直接调用 `TIFFStripSize()`。

触发条件包括：

- 文件损坏或在导入期间被移动
- 权限问题
- Windows Unicode 路径处理不兼容
- TIFF 编码/目录结构不被当前 libtiff 接受
- 多页目录索引无效

Windows 路径应优先使用所链接 libtiff 提供的宽字符打开 API，并对其版本做编译期保护；不能仅把 `QString::toUtf8()` 交给窄字符 `TIFFOpen()` 后假定所有中文路径都可靠。

### F-05：尺寸和偏移计算使用 32 位 `int`

严重度：Critical

存在多处：

```cpp
int nbytes = m_width*m_height*m_bytesPerVoxel;
```

16 位方形图片在约 32768 x 32768 时已达到/超过 `INT_MAX`。溢出后可能出现负长度、错误的小分配或后续大写入。

所有图片尺寸、像素数、字节数、strip/tile 偏移和预计峰值内存都必须使用 `uint64_t`/`quint64` 或 libtiff 对应的 64 位类型，并使用显式 checked multiplication。

### F-06：查看 TIFF 像素值时泄漏整张切片

严重度：High

位置：

- `tools/import/plugins/tiff/tiffplugin.cpp`，`TiffPlugin::rawValue()`，约 827-862 行
- `tools/import/remapimage.cpp`，`RemapImage::mouseMoveEvent()`，约 974-1013 行

`rawValue()` 每次调用都会：

```cpp
new uchar[10];
new uchar[m_width*m_height*m_bytesPerVoxel];
```

两个分配均没有释放。右键拖动时，每个鼠标移动事件都会调用一次。

8192 x 8192、16 位图片每个事件约泄漏 128 MiB；10 个事件约泄漏 1.25 GiB。即使导入成功，用户在预览中查看像素也可能很快拖垮系统。

### F-07：普通 Image Stack 插件也缺少输入验证

严重度：High

位置：`tools/import/plugins/imagestack/imagestackplugin.cpp`

- `setImageFiles()` 只用第一张 `QImage` 设置尺寸，约 99-142 行。
- `getDepthSlice()` 加载后续图片但不检查 `isNull()` 或尺寸一致性，约 192-217 行。
- `rawValue()` 在每次像素查询时重新加载并解码整张图片，约 278-313 行。

TIFF 修复是首要任务，但如果现场选择的是“Image Stack”而不是“Grayscale TIFF”，必须同时修复这一插件。

### F-08：有符号类型边界与直方图计数类型

严重度：Medium

原始实现将 `Char` 最小值按 `-127`、`Short` 最小值按 `-32767` 处理，但实际最小值是 `-128` 和 `-32768`。最小值可能产生负直方图下标。实验版本通过 clamp 掩盖了越界，但语义仍需修正。

直方图计数目前使用 32 位 `uint`。超大图栈中单个 bin 超过 `UINT_MAX` 时会回绕，应在计算阶段使用 64 位计数，最终传给旧 UI 时再做明确的缩放或饱和转换。

## 5. 在目标笔记本上锁定现场分支

### 5.1 先记录版本和入口

记录以下信息：

```text
drishtiimport.exe 文件版本/构建时间：
对应 git commit（若可知）：
是否包含 QtConcurrent::blockingMap TIFF 修改：是/否
导入菜单项：Grayscale TIFF Image Files / Grayscale TIFF Image Directory / Image Stack / 其他
卡死前最后一条界面文字：
系统内存：
页面文件：系统管理/固定/关闭，当前上限：
数据位于：本地 SSD / 外置盘 / 网络盘 / OneDrive 等同步目录
```

如果卡死前是 `Generating Histogram`，优先检查 F-01 到 F-05。

如果已经出现预览，随后右键拖动才开始内存增长，优先检查 F-06。

如果卡死发生在 `Saving...`、PVL 转换或主程序 `Loading Volume`，则不是本交接文件当前锁定的同一阶段，应另行追踪 `Raw2Pvl` 或主渲染器加载链。

### 5.2 只读检查一张原始文件

不要先解码全部 10 张。使用 libtiff 的 `tiffinfo`/`tiffdump`，或编写只读取目录和标签的最小探针，输出每个文件/页面的：

```text
path
directory/page count
width, height
bits per sample
samples per pixel
sample format
planar config
compression
rows per strip
strip count
is tiled, tile width, tile length
orientation
scanline bytes
estimated decoded bytes
```

对全部 10 张只做元数据扫描，确认它们完全一致。元数据探针必须使用 checked 64 位运算，不得分配完整像素缓冲。

### 5.3 区分资源风暴、UI 阻塞与堆破坏

从 1 张开始，依次测试 1、2、4、10 张；每次记录：

- `drishtiimport.exe` Working Set
- Private Bytes/Commit Size
- 系统 Committed/Commit Limit
- CPU 总占用和 `drishtiimport.exe` CPU
- 数据盘和页面文件盘的 Active Time
- 进程句柄数
- UI 最长无响应时间

判断：

| 现场表现 | 首要分支 |
|---|---|
| 图片数量翻倍时 Private Bytes 近似按完整切片大小线性增长 | F-01 批量预分配 |
| 内存稳定，但全核 CPU/磁盘高且窗口未响应 | F-02 同步解码 |
| 输入是 RGB、混合尺寸/位深、tiled 或异常多页 TIFF | F-03 越界风险 |
| 立即崩溃或 Windows Error Reporting 指向 libtiff/heap | F-03/F-04/F-05 |
| 预览后右键拖动，内存每个事件阶梯增长 | F-06 |
| 选择的是 Image Stack | F-07 |

正常调用链中没有发现可证实的互斥锁死。如果现场 CPU 接近 0、内存稳定、磁盘空闲而程序仍永久不返回，再抓线程栈确认真正死锁。

可选诊断：使用 Sysinternals ProcDump 对“窗口无响应”抓完整转储。不要在系统已经严重换页时重复抓取大型 dump。

```powershell
procdump64.exe -accepteula -ma -h drishtiimport.exe C:\DrishtiDumps
```

## 6. 必须实现的修复

### 6.1 TIFF 元数据预检

在任何完整像素分配前完成整个栈的预检：

1. RAII 打开每个文件/页面，打开失败立即返回结构化错误。
2. 多页 TIFF 显式将目录设为 0，再读取基准元数据。
3. 使用 `TIFFGetFieldDefaulted()` 获取有默认值的标签，并检查返回状态。
4. 当前“Grayscale TIFF”插件只接受 `SAMPLESPERPIXEL == 1`；其他输入明确拒绝，不得静默按灰度缓冲读取。
5. 只接受代码确实支持的位深与 sample format 组合。
6. 校验所有文件/页面宽高、位深、sample format、samples per pixel、planar config 一致。
7. 对 tiled TIFF：要么实现安全 tile 解码，要么在预检阶段清晰拒绝；不得走 strip 路径碰运气。
8. 用 checked 64 位乘法计算像素数、行字节数和切片字节数。
9. 输出预计单张解码大小和预计工作集，超出配置上限时在开始前报错。

建议将预检结果保存为不可变 metadata 结构，后续解码不要重复猜测格式。

### 6.2 安全解码

对于当前仅支持的灰度格式，优先采用容易验证边界的行读取方案：

- 使用 libtiff 的 64 位 size API（链接版本支持时）。
- 取得 scanline size，验证其与预期有效行字节数的关系。
- 每次只解码到已知大小的行/strip 临时缓冲，再拷贝到目标切片的已验证范围。
- 每次写入前检查 `destinationOffset + bytesToCopy <= sliceBytes`。
- 对所有 libtiff 返回值检查错误；错误中止当前任务并释放资源。

如果继续使用 `TIFFReadEncodedStrip()`：

- 不要直接把未知返回长度写入 `tmp + imageOffset`。
- strip 临时缓冲按 libtiff 报告的安全大小分配。
- 根据 rows-per-strip、最后一个 strip 的实际行数和有效行字节数拷贝。
- 使用 `tmsize_t`/64 位偏移，并检查每次累计边界。

保持现有 `getDepthSlice()` 对外的轴顺序/转置语义，除非用回归测试证明需要改变。不要在修复内存问题时顺便改变数据方向。

### 6.3 有界内存和并发

删除固定 `BATCH_SIZE = 32` 的完整切片批处理模型。

推荐模型：

```text
一个后台导入 worker
  -> 顺序读取文件/页面（默认并发 1）
  -> 复用 1 个或最多 2 个完整切片缓冲
  -> 对已解码切片按像素区块并行扫描直方图
  -> 合并小型线程局部直方图
  -> 发出逐张进度信号
```

图片来自同一磁盘时，多文件并发通常只会增加 I/O 竞争。若确实提供解码并发，必须按以下两者共同限制：

```text
workerCount <= configuredCpuCap
workerCount * worstCasePerWorkerBytes <= memoryBudget
```

Windows 核显设备默认建议 `workerCount = 1` 或 `2`，不能默认使用所有逻辑处理器。

峰值目标：完整切片缓冲的数量与图栈深度无关。

### 6.4 真正的异步 UI

- 不要在 GUI 线程调用 `blockingMap()`。
- 不要把 `qApp->processEvents()` 当成并发架构。
- 使用项目可接受的 `QThread`/worker object 或等价异步任务。
- worker 仅发送进度、成功、取消和错误信号；所有 QMessageBox/控件更新都在 GUI 线程执行。
- 进度至少包含 `Validating metadata`、`Decoding i/N`、`Building histogram`。
- 提供可取消操作；取消后等待 worker 安全结束并释放句柄/缓冲。
- 导入期间禁用重复启动同一导入任务，避免重入。
- 窗口关闭时必须取消并 join worker，不能留下全局线程池任务访问已销毁插件。

### 6.5 直方图

- 使用 64 位计数累积。
- 正确处理 `int8` 的 `-128` 和 `int16` 的 `-32768`。
- 每线程只分配一个小型局部直方图，不为每张图片永久保留直方图。
- 合并时检查溢出。
- 旧 UI 若只能接收 `QList<uint>`，在边界处明确做饱和或比例缩放，并记录这一兼容行为。

### 6.6 修复像素查询泄漏

`TiffPlugin::rawValue()` 必须移除无用的 `new uchar[10]`，并用 RAII 管理任何临时缓冲。

更好的实现是：

- 复用当前预览切片缓存；或
- 只读取目标 scanline/tile，而不是为一个像素解码整张图片。

右键连续移动 1000 次后，进程 Private Bytes 不应持续阶梯增长。

### 6.7 普通 Image Stack 插件

现场若使用 `Image Stack`，至少同步完成：

- 检查每个 `QImage` 是否加载成功。
- 验证所有图片尺寸一致。
- 明确 RGB/RGBA 的内部字节布局，避免 3 字节声明与 4 字节写入不一致。
- 像素查询复用缓存或按需读取，避免每个鼠标事件完整解码。
- 所有尺寸计算改为 checked 64 位运算。

## 7. 建议的修改范围

首要文件：

```text
tools/import/plugins/tiff/tiffplugin.cpp
tools/import/plugins/tiff/tiffplugin.h
tools/import/plugins/tiff/tiff.pro
tools/import/remapwidget.cpp
tools/import/remapwidget.h
tools/import/volumedata.cpp
tools/import/volumedata.h
tools/import/remapimage.cpp
```

如果现场使用 Image Stack：

```text
tools/import/plugins/imagestack/imagestackplugin.cpp
tools/import/plugins/imagestack/imagestackplugin.h
```

允许增加小型、职责明确的 metadata/worker/checked-size 辅助类；不要借此重构渲染器或整个插件系统。

建议拆分为可独立验证的修改组：

1. TIFF RAII、元数据预检、64 位边界和安全解码。
2. 后台 worker、进度和取消。
3. 直方图内存模型和像素查询泄漏。
4. Image Stack 输入验证（若现场相关）。
5. 自动化测试和 Windows 现场验证脚本。

## 8. 测试矩阵

应使用程序生成或无隐私的测试图片。不要把用户原始科研数据提交到仓库。

### 8.1 正常输入

| 用例 | 预期 |
|---|---|
| 10 x 4096²，8 位灰度，uncompressed | 成功，UI 可响应，内存与深度无关 |
| 10 x 4096²，16 位灰度，LZW/Deflate | 成功，像素值和直方图正确 |
| 10 x 8192²，16 位灰度 | 成功或在预算不足前明确拒绝，不得拖垮系统 |
| 单个一致的多页 TIFF | 成功，页数和方向正确 |
| 中文目录及中文文件名 | 成功，或给出明确、非崩溃的路径错误 |

### 8.2 必须安全拒绝的输入

| 用例 | 预期 |
|---|---|
| RGB/RGBA TIFF 交给灰度插件 | 在解码前报格式不支持 |
| 后续图片尺寸不同 | 在预检阶段报哪一张不一致 |
| 位深/sample format 混合 | 在预检阶段报错 |
| 损坏/截断 TIFF | 释放资源并报错，不崩溃 |
| 文件不存在或导入中被移走 | 释放资源并报错 |
| tiled TIFF（若未实现） | 在预检阶段明确拒绝 |
| 多页 TIFF 页面尺寸不同 | 在预检阶段明确拒绝 |
| 预计字节数发生 64 位溢出或超过上限 | 在分配前拒绝 |

### 8.3 交互和资源测试

- 导入期间拖动、最小化、恢复窗口，界面持续刷新。
- 点击取消后在合理时间内停止，句柄数回到基线。
- 连续导入成功、取消、失败各 20 次，Private Bytes 不持续增长。
- 右键移动查询像素 1000 次，内存稳定。
- 导入同一组图片时，1、2、4、10 张的峰值内存不按完整切片大小线性增长。
- 关闭窗口时没有 worker 使用已销毁对象。

### 8.4 内存安全工具

在可用的 Windows MSVC 构建上运行 AddressSanitizer，至少覆盖：

- RGB TIFF 误入灰度插件
- 混合尺寸堆栈
- 截断 strip
- 超大尺寸标签
- 右键像素查询

同时运行静态分析，检查所有乘法、窄化转换和 libtiff 返回值。

## 9. 验收标准

修复只有同时满足以下条件才算完成：

1. `drishtiimport` 在没有独显、只使用 Intel/AMD 核显的 Windows 设备上可完成目标图片导入；导入链不创建或依赖 OpenGL 上下文。
2. 10 张图片不会因为固定批量预分配而同时常驻完整解码缓冲。
3. UI 在元数据检查、解码和直方图计算期间保持可交互，并能取消。
4. 峰值内存由固定 worker 数和单张解码大小决定，不由图片总张数直接决定。
5. RGB、混合尺寸、混合位深、损坏、tiled/异常多页输入均不会写越界或解引用空句柄。
6. 所有尺寸和偏移使用 checked 64 位计算。
7. 右键像素查询无整切片泄漏。
8. 正常 8/16 位灰度 TIFF 的方向、像素值、min/max 和直方图与修复前有效结果一致。
9. 自动化测试通过，并提供目标 i7-13700H 笔记本的峰值内存、CPU、磁盘和响应性记录。
10. `git diff --check` 通过；构建日志和测试命令写入交付说明。

建议的定量内存验收：

```text
peak private bytes <= baseline + workerCount * worstCaseSliceBytes + boundedDecoderOverhead
```

实际阈值应在目标设备测量后记录，但不允许出现 `depth * sliceBytes` 的批量缓冲行为。

## 10. 不接受的“修复”

以下做法不足以关闭问题：

- 只把 `BATCH_SIZE` 从 32 改成 4 或 1。
- 继续在 GUI 线程使用 `blockingMap()`。
- 仅增加 `qApp->processEvents()`。
- 仅限制 CPU 线程数，不修复 TIFF 边界和格式校验。
- 用压缩文件大小估算解码内存。
- 捕获 `std::bad_alloc` 后继续使用部分初始化状态。
- 把问题归因于 Intel 核显并改三维渲染设置。
- 从 worker 线程直接弹 `QMessageBox` 或更新 Qt 控件。
- 为了方便测试而删除/覆盖用户现有未提交修改。

## 11. 目标设备回传模板

修复智能体完成后，请在本文件末尾或单独报告中填写：

```text
目标设备：
RAM / 页面文件：
GPU / 驱动（仅记录，不作为导入依赖）：
运行二进制对应 commit：
使用的导入菜单项：
问题 TIFF 元数据摘要：
现场根因分支（F-01 ... F-08）：

修改文件：
关键设计：
构建命令：
测试命令：

修复前：
  1/2/4/10 张峰值 Private Bytes：
  CPU：
  磁盘 Active Time：
  UI 最长无响应：

修复后：
  1/2/4/10 张峰值 Private Bytes：
  CPU：
  磁盘 Active Time：
  UI 最长无响应：

ASan/静态分析结果：
剩余限制：
```

## 12. 最终判断

对于 i7-13700H 和近五年的 Intel/AMD 核显，本问题的第一修复目标应是“有界内存、安全 TIFF 解码、后台导入”，而不是 CPU 三维渲染。

当前源码已经提供了与“约 10 张图片导致 DrishtiImport 和 Explorer 一起卡死”高度吻合的资源风暴路径；同时还存在足以造成堆破坏的输入边界缺陷。目标设备上的样本元数据和资源曲线用于确定它们中哪一个是现场首因，但上述 Critical/High 缺陷都应修复，不能只处理最先观察到的一项。
