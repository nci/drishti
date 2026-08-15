[CmdletBinding()]
param(
  [string]$RepositoryRoot = $PSScriptRoot,
  [string]$BuildBin = (Join-Path $PSScriptRoot '.lab-agent\dependencies\build\main-current\bin'),
  [string]$OutputDirectory = (Join-Path $PSScriptRoot '.lab-agent\package'),
  [string]$PackageName = 'drishti-cpu-igpu-release-2026-08-15-audited',
  [string]$VcpkgInstalled = (Join-Path $PSScriptRoot '.lab-agent\dependencies\install\vcpkg\installed\x64-windows'),
  [string]$DependencyRoot = (Join-Path $PSScriptRoot '.lab-agent\dependencies'),
  [string]$VcRuntimeRoot = (Join-Path $PSScriptRoot '.lab-agent\dependencies\install\msvc-runtime'),
  [string]$QtRoot = (Join-Path $PSScriptRoot '.lab-agent\dependencies\toolchain\Qt-open\5.15.2\msvc2019_64'),
  [string]$PythonRuntimeRoot = (Join-Path $PSScriptRoot '.lab-agent\dependencies\toolchain\Python313'),
  [string]$DumpbinPath = '',
  [switch]$KeepStage
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-NormalizedPath([string]$Path)
{
  return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Assert-PathWithin([string]$Child, [string]$Parent)
{
  $normalizedChild = Get-NormalizedPath $Child
  $normalizedParent = Get-NormalizedPath $Parent
  $prefix = $normalizedParent + '\'
  if (!$normalizedChild.StartsWith(
        $prefix, [System.StringComparison]::OrdinalIgnoreCase))
    {
      throw "Refusing to modify a path outside $normalizedParent`: $normalizedChild"
    }
}

function Reset-Directory([string]$Path, [string]$AllowedParent)
{
  Assert-PathWithin $Path $AllowedParent
  if (Test-Path -LiteralPath $Path)
    {
      Remove-Item -LiteralPath $Path -Recurse -Force
    }
  New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Copy-Directory([string]$Source, [string]$Destination)
{
  if (!(Test-Path -LiteralPath $Source -PathType Container))
    {
      throw "Required directory is missing: $Source"
    }
  New-Item -ItemType Directory -Path $Destination -Force | Out-Null
  Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
  }
}

function Copy-RequiredFile([string]$Source, [string]$Destination)
{
  if (!(Test-Path -LiteralPath $Source -PathType Leaf))
    {
      throw "Required file is missing: $Source"
    }
  $parent = Split-Path -Parent $Destination
  New-Item -ItemType Directory -Path $parent -Force | Out-Null
  Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Resolve-Dumpbin([string]$RequestedPath)
{
  if (![string]::IsNullOrWhiteSpace($RequestedPath))
    {
      if (!(Test-Path -LiteralPath $RequestedPath -PathType Leaf))
        {
          throw "dumpbin.exe was not found at: $RequestedPath"
        }
      return Get-NormalizedPath $RequestedPath
    }

  $command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
  if ($command)
    {
      return Get-NormalizedPath $command.Source
    }

  $toolsRoot =
    'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC'
  if (Test-Path -LiteralPath $toolsRoot -PathType Container)
    {
      $candidate = Get-ChildItem -LiteralPath $toolsRoot -Directory |
        Sort-Object Name -Descending | ForEach-Object {
          Join-Path $_.FullName 'bin\Hostx64\x64\dumpbin.exe'
        } | Where-Object {
          Test-Path -LiteralPath $_ -PathType Leaf
        } | Select-Object -First 1
      if ($candidate)
        {
          return Get-NormalizedPath $candidate
        }
    }

  throw 'dumpbin.exe is required for the portable dependency audit.'
}

function Get-PeDependencies([string]$Dumpbin, [string]$Binary)
{
  $output = @(& $Dumpbin /nologo /dependents $Binary 2>&1)
  if ($LASTEXITCODE -ne 0)
    {
      throw "dumpbin failed for $Binary`: $($output -join [Environment]::NewLine)"
    }

  return @($output | ForEach-Object {
    $line = $_.ToString().Trim()
    if ($line -match '^[A-Za-z0-9_.+\-]+\.dll$')
      {
        $line.ToLowerInvariant()
      }
  } | Sort-Object -Unique)
}

function Assert-X64Pe([string]$Dumpbin, [string]$Binary)
{
  $output = @(& $Dumpbin /nologo /headers $Binary 2>&1)
  if ($LASTEXITCODE -ne 0)
    {
      throw "dumpbin failed for $Binary`: $($output -join [Environment]::NewLine)"
    }

  $machine = @($output | Where-Object {
    $_.ToString() -match '^\s*8664 machine \(x64\)\s*$'
  })
  $pe32Plus = @($output | Where-Object {
    $_.ToString() -match '^\s*20B magic # \(PE32\+\)\s*$'
  })
  if ($machine.Count -ne 1 -or $pe32Plus.Count -ne 1)
    {
      throw "Portable package contains a non-x64 PE file: $Binary"
    }
}

function Disable-PythonSiteImport([string]$Path)
{
  if (!(Test-Path -LiteralPath $Path -PathType Leaf))
    {
      throw "Python path configuration is missing: $Path"
    }

  $lines = [System.IO.File]::ReadAllLines($Path)
  $filtered = @($lines | Where-Object { $_ -notmatch '^\s*import\s+site\s*$' })
  [System.IO.File]::WriteAllLines(
    $Path, $filtered, [System.Text.UTF8Encoding]::new($false))

  if ([System.IO.File]::ReadAllText($Path) -match '(?m)^\s*import\s+site\s*$')
    {
      throw "Python user-site isolation failed for: $Path"
    }
}

function Get-RelativeFiles([string]$Root, [string]$Filter)
{
  return @(Get-ChildItem -LiteralPath $Root -Recurse -File -Filter $Filter |
    ForEach-Object { $_.FullName.Substring($Root.Length + 1) } |
    Sort-Object)
}

function Assert-ExactManifest([string]$Label,
                              [string[]]$Expected,
                              [string[]]$Actual)
{
  $difference = Compare-Object @($Expected | Sort-Object) @($Actual | Sort-Object)
  if ($difference)
    {
      throw "$Label manifest does not match:`n$($difference | Out-String)"
    }
}

function Test-PackagedDependency([string]$StageRoot,
                                 [System.IO.FileInfo]$PeFile,
                                 [string]$Dependency)
{
  $searchDirectories = New-Object System.Collections.Generic.List[string]
  $searchDirectories.Add($PeFile.DirectoryName)

  $pythonRoot = Join-Path $StageRoot 'python'
  $normalizedPe = Get-NormalizedPath $PeFile.FullName
  $pythonPrefix = (Get-NormalizedPath $pythonRoot) + '\'
  if ($normalizedPe.StartsWith(
        $pythonPrefix, [System.StringComparison]::OrdinalIgnoreCase))
    {
      # Validate the standalone interpreter independently of the application
      # directory and the caller's current working directory.
      $searchDirectories.Add($pythonRoot)

      $pythonRelative = $normalizedPe.Substring($pythonPrefix.Length)
      if ($pythonRelative.StartsWith(
            'Lib\site-packages\numpy\',
            [System.StringComparison]::OrdinalIgnoreCase) -or
          $pythonRelative.StartsWith(
            'Lib\site-packages\numpy.libs\',
            [System.StringComparison]::OrdinalIgnoreCase))
        {
          # NumPy's delvewheel bootstrap explicitly calls AddDllDirectory for
          # this directory before importing its extension modules.
          $searchDirectories.Add(
            (Join-Path $pythonRoot 'Lib\site-packages\numpy.libs'))
        }
    }
  else
    {
      # Applications and their plugins resolve shared dependencies from the
      # directory containing the four executables.
      $searchDirectories.Add($StageRoot)
    }

  foreach ($directory in @($searchDirectories | Sort-Object -Unique))
    {
      if (Test-Path -LiteralPath (Join-Path $directory $Dependency) -PathType Leaf)
        {
          return $true
        }
    }

  return $false
}

$RepositoryRoot = Get-NormalizedPath $RepositoryRoot
$BuildBin = Get-NormalizedPath $BuildBin
$OutputDirectory = Get-NormalizedPath $OutputDirectory
$VcpkgInstalled = Get-NormalizedPath $VcpkgInstalled
$DependencyRoot = Get-NormalizedPath $DependencyRoot
$VcRuntimeRoot = Get-NormalizedPath $VcRuntimeRoot
$QtRoot = Get-NormalizedPath $QtRoot
$PythonRuntimeRoot = Get-NormalizedPath $PythonRuntimeRoot
$DumpbinPath = Resolve-Dumpbin $DumpbinPath

$runtimeSearchRoots = @(
  $BuildBin,
  (Join-Path $QtRoot 'bin'),
  (Join-Path $VcpkgInstalled 'bin'),
  (Join-Path $DependencyRoot 'install\openvdb-11.0.0\bin'),
  (Join-Path $DependencyRoot 'install\qglviewer-2.6.4\bin'),
  $VcRuntimeRoot,
  $PythonRuntimeRoot
)
function Resolve-RequiredRuntime([string]$Name)
{
  foreach ($root in $runtimeSearchRoots)
    {
      $candidate = Join-Path $root $Name
      if (Test-Path -LiteralPath $candidate -PathType Leaf)
        {
          return $candidate
        }
    }
  throw "Required runtime DLL is missing from the canonical dependency roots: $Name"
}

foreach ($requiredDirectory in @(
    $RepositoryRoot,
    $BuildBin,
    (Join-Path $RepositoryRoot 'bin\assets'),
    (Join-Path $RepositoryRoot 'bin\docs'),
    $PythonRuntimeRoot,
    (Join-Path $VcpkgInstalled 'share'),
    $VcRuntimeRoot))
  {
    if (!(Test-Path -LiteralPath $requiredDirectory -PathType Container))
      {
        throw "Required directory is missing: $requiredDirectory"
      }
  }

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$stageContainer = Join-Path $OutputDirectory '_stage'
New-Item -ItemType Directory -Path $stageContainer -Force | Out-Null
$stageRoot = Join-Path $stageContainer $PackageName
Reset-Directory $stageRoot $stageContainer

$mainPrograms = @(
  'drishti.exe',
  'drishtiimport.exe',
  'drishtipaint.exe',
  'drishtimesh.exe'
)
foreach ($program in $mainPrograms)
  {
    Copy-RequiredFile (Join-Path $BuildBin $program) (Join-Path $stageRoot $program)
  }

# TIFF imports use an isolated helper process for metadata and scanline
# decoding. Keep it beside the applications so the plugin can resolve it
# without an environment variable in a clean portable directory.
Copy-RequiredFile (Join-Path $BuildBin 'tiffdecodehelper.exe') `
                  (Join-Path $stageRoot 'tiffdecodehelper.exe')

$expectedRootRuntimeDlls = @(
  'aec.dll',
  'assimp-vc142-mt.dll',
  'avcodec-63.dll',
  'avdevice-63.dll',
  'avfilter-12.dll',
  'avformat-63.dll',
  'avutil-61.dll',
  'blosc.dll',
  'boost_atomic-vc142-mt-x64-1_91.dll',
  'boost_chrono-vc142-mt-x64-1_91.dll',
  'boost_container-vc142-mt-x64-1_91.dll',
  'boost_date_time-vc142-mt-x64-1_91.dll',
  'boost_iostreams-vc142-mt-x64-1_91.dll',
  'boost_random-vc142-mt-x64-1_91.dll',
  'boost_thread-vc142-mt-x64-1_91.dll',
  'bz2.dll',
  'concrt140.dll',
  'D3Dcompiler_47.dll',
  'deflate.dll',
  'ffi-8.dll',
  'freeglut.dll',
  'glew32.dll',
  'gmsh.dll',
  'hdf5_hl.dll',
  'hdf5.dll',
  'hwloc-15.dll',
  'Iex-3_4.dll',
  'IlmThread-3_4.dll',
  'Imath-3_2.dll',
  'jpeg62.dll',
  'kubazip.dll',
  'legacy.dll',
  'libcrypto-3-x64.dll',
  'libcurl.dll',
  'libEGL.dll',
  'libexpat.dll',
  'libGLESv2.dll',
  'liblzma.dll',
  'libssl-3-x64.dll',
  'lz4.dll',
  'minizip.dll',
  'msvcp140_1.dll',
  'msvcp140_2.dll',
  'msvcp140_atomic_wait.dll',
  'msvcp140_codecvt_ids.dll',
  'msvcp140.dll',
  'netcdf.dll',
  'openblas.dll',
  'OpenEXR-3_4.dll',
  'OpenEXRCore-3_4.dll',
  'OpenEXRUtil-3_4.dll',
  'opengl32sw.dll',
  'openjp2.dll',
  'openjph.0.31.dll',
  'openvdb.dll',
  'pkgconf-8.dll',
  'poly2tri.dll',
  'pugixml.dll',
  'python313.dll',
  'QGLViewer2.dll',
  'Qt5Concurrent.dll',
  'Qt5Core.dll',
  'Qt5Gui.dll',
  'Qt5Multimedia.dll',
  'Qt5MultimediaWidgets.dll',
  'Qt5Network.dll',
  'Qt5OpenGL.dll',
  'Qt5Svg.dll',
  'Qt5Widgets.dll',
  'Qt5Xml.dll',
  'snappy.dll',
  'sqlite3.dll',
  'swresample-7.dll',
  'swscale-10.dll',
  'szip.dll',
  'tbb12.dll',
  'tbbbind_2_5.dll',
  'tbbmalloc_proxy.dll',
  'tbbmalloc.dll',
  'tiff.dll',
  'tinyxml2.dll',
  'turbojpeg.dll',
  'vccorlib140.dll',
  'vcruntime140_1.dll',
  'vcruntime140.dll',
  'z.dll',
  'zstd.dll'
)
foreach ($runtimeDll in $expectedRootRuntimeDlls)
  {
    Copy-RequiredFile (Resolve-RequiredRuntime $runtimeDll) `
                      (Join-Path $stageRoot $runtimeDll)
  }

# The build directory contains an obsolete Python 3.12 stable-ABI DLL. Use the
# matching 3.13 copy from the embedded distribution instead.
Copy-RequiredFile (Join-Path $PythonRuntimeRoot 'python3.dll') `
                  (Join-Path $stageRoot 'python3.dll')
$rootPythonPath = Join-Path $stageRoot 'python313._pth'
[System.IO.File]::WriteAllLines(
  $rootPythonPath, @('.', 'Lib', 'Lib\site-packages', 'import site'),
  [System.Text.UTF8Encoding]::new($false))

$qtPluginDirectories = @(
  'audio',
  'bearer',
  'iconengines',
  'imageformats',
  'mediaservice',
  'platforms',
  'playlistformats',
  'styles'
)
foreach ($directory in $qtPluginDirectories)
  {
    $sourcePluginDirectory = Join-Path $QtRoot "plugins\$directory"
    $stagePluginDirectory = Join-Path $stageRoot $directory
    if (!(Test-Path -LiteralPath $sourcePluginDirectory -PathType Container))
      {
        throw "Required Qt plugin directory is missing: $sourcePluginDirectory"
      }
    New-Item -ItemType Directory -Path $stagePluginDirectory -Force | Out-Null
    Get-ChildItem -LiteralPath $sourcePluginDirectory -File | Where-Object {
      $_.Name -notmatch 'd\.dll$'
    } | ForEach-Object {
      Copy-Item -LiteralPath $_.FullName -Destination $stagePluginDirectory -Force
    }
  }

foreach ($directory in @('importplugins', 'renderplugins'))
  {
    Copy-Directory (Join-Path $BuildBin $directory) `
                   (Join-Path $stageRoot $directory)
  }

Copy-Directory $PythonRuntimeRoot (Join-Path $stageRoot 'python')
# Packaging tools carry helper executables for multiple architectures and are
# not part of Drishti's embedded runtime.  Exclude them from the release
# closure so every shipped PE remains x64 and the runtime stays deterministic.
foreach ($pythonToolPath in @(
    'python\Scripts',
    'python\Lib\ensurepip',
    'python\Lib\site-packages\pip',
    'python\Lib\site-packages\pip-24.3.1.dist-info'))
  {
    $absoluteToolPath = Join-Path $stageRoot $pythonToolPath
    if (Test-Path -LiteralPath $absoluteToolPath)
      {
        Remove-Item -LiteralPath $absoluteToolPath -Recurse -Force
      }
  }
$embeddedPythonPath = Join-Path $stageRoot 'python\python313._pth'
[System.IO.File]::WriteAllLines(
  $embeddedPythonPath, @('.', 'Lib', 'Lib\site-packages', 'import site'),
  [System.Text.UTF8Encoding]::new($false))

Disable-PythonSiteImport (Join-Path $stageRoot 'python313._pth')
Disable-PythonSiteImport (Join-Path $stageRoot 'python\python313._pth')

$vcRuntimeDlls = @(
  'concrt140.dll',
  'msvcp140.dll',
  'msvcp140_1.dll',
  'msvcp140_2.dll',
  'msvcp140_atomic_wait.dll',
  'msvcp140_codecvt_ids.dll',
  'vccorlib140.dll',
  'vcruntime140.dll',
  'vcruntime140_1.dll'
)
$pythonVcVersion =
  (Get-Item -LiteralPath (Resolve-RequiredRuntime 'vcruntime140.dll')).
    VersionInfo.FileVersionRaw
foreach ($runtimeDll in $vcRuntimeDlls)
  {
    $source = Get-Item -LiteralPath (Join-Path $VcRuntimeRoot $runtimeDll) `
                       -ErrorAction Stop
    if (!$source.VersionInfo.FileVersionRaw -or
        $source.VersionInfo.FileVersionRaw -lt $pythonVcVersion)
      {
        throw "VC Runtime $($source.FullName) is older than embedded Python " +
              "$pythonVcVersion."
      }
    Copy-RequiredFile $source.FullName (Join-Path $stageRoot $runtimeDll)
    Copy-RequiredFile $source.FullName (Join-Path $stageRoot "python\$runtimeDll")
  }

$actualRootRuntimeDlls = @(
  Get-ChildItem -LiteralPath $stageRoot -File -Filter '*.dll' |
    Select-Object -ExpandProperty Name
)
Assert-ExactManifest 'Root runtime DLL' `
  @($expectedRootRuntimeDlls + 'python3.dll') $actualRootRuntimeDlls

# Assets and help are copied from source so stale build-directory files cannot
# reappear in a release.
Copy-Directory (Join-Path $RepositoryRoot 'bin\assets') `
               (Join-Path $stageRoot 'assets')
Copy-Directory (Join-Path $RepositoryRoot 'bin\docs') `
               (Join-Path $stageRoot 'docs')

# Qt probes this directory even when it ultimately uses Windows system fonts.
# Keep the path present without bundling an unlicensed system font.
$fontDirectory = Join-Path $stageRoot 'lib\fonts'
New-Item -ItemType Directory -Path $fontDirectory -Force | Out-Null
[System.IO.File]::WriteAllText(
  (Join-Path $fontDirectory '.keep'),
  'Fonts are provided by the host operating system.' + [Environment]::NewLine,
  [System.Text.UTF8Encoding]::new($false))

Copy-RequiredFile (Join-Path $RepositoryRoot 'PORTABLE_README.md') `
                  (Join-Path $stageRoot 'README.md')
Copy-RequiredFile (Join-Path $RepositoryRoot 'THIRD_PARTY_NOTICES.md') `
                  (Join-Path $stageRoot 'THIRD_PARTY_NOTICES.md')
Copy-RequiredFile (Join-Path $RepositoryRoot 'qt.conf') `
                  (Join-Path $stageRoot 'qt.conf')
foreach ($licenseName in @('LICENSE', 'license.md'))
  {
    Copy-RequiredFile (Join-Path $RepositoryRoot $licenseName) `
                      (Join-Path $stageRoot $licenseName)
  }

$licenseRoot = Join-Path $stageRoot 'licenses'
$vcpkgLicenseRoot = Join-Path $licenseRoot 'vcpkg'
New-Item -ItemType Directory -Path $vcpkgLicenseRoot -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $VcpkgInstalled 'share') `
              -Recurse -File -Filter 'copyright' | ForEach-Object {
  $package = Split-Path -Leaf (Split-Path -Parent $_.FullName)
  Copy-Item -LiteralPath $_.FullName `
            -Destination (Join-Path $vcpkgLicenseRoot ($package + '.txt')) `
            -Force
}

$directLicenses = @(
  @('source\ITK-5.0.1\LICENSE', 'ITK-5.0.1\LICENSE'),
  @('source\libQGLViewer-2.6.4\LICENCE', 'libQGLViewer-2.6.4\LICENCE'),
  @('source\libQGLViewer-2.6.4\GPL_EXCEPTION', 'libQGLViewer-2.6.4\GPL_EXCEPTION'),
  @('source\openvdb-11.0.0\LICENSE', 'OpenVDB-11.0.0\LICENSE'),
  @('licenses\qt-5.15.2\LICENSE.GPL3', 'Qt-5.15.2\LICENSE.GPL3'),
  @('licenses\qt-5.15.2\LICENSE.GPL3-EXCEPT', 'Qt-5.15.2\LICENSE.GPL3-EXCEPT'),
  @('licenses\qt-5.15.2\LICENSE.LGPL3', 'Qt-5.15.2\LICENSE.LGPL3'),
  @('licenses\qt-5.15.2\LICENSE.LGPLv3', 'Qt-5.15.2\LICENSE.LGPLv3')
)
foreach ($license in $directLicenses)
  {
    Copy-RequiredFile (Join-Path $DependencyRoot $license[0]) `
                      (Join-Path $licenseRoot $license[1])
  }

$vcpkgStatus = Join-Path (Split-Path -Parent (Split-Path -Parent $VcpkgInstalled)) 'status'
if (Test-Path -LiteralPath $vcpkgStatus -PathType Leaf)
  {
    Copy-RequiredFile $vcpkgStatus (Join-Path $stageRoot 'VCPKG_INSTALLED.txt')
  }
else
  {
    $installedPackages = Get-ChildItem -LiteralPath (Join-Path $VcpkgInstalled 'share') `
      -Directory | Sort-Object Name | ForEach-Object { $_.Name }
    [System.IO.File]::WriteAllLines(
      (Join-Path $stageRoot 'VCPKG_INSTALLED.txt'),
      @('Canonical vcpkg status is unavailable; installed package shares:') +
      $installedPackages,
      [System.Text.UTF8Encoding]::new($false))
  }

# Remove compiler outputs and bytecode caches from every copied subtree.
Get-ChildItem -LiteralPath $stageRoot -Recurse -File | Where-Object {
  $_.Extension -in @('.lib', '.exp', '.pdb', '.ilk', '.obj', '.pyc') -or
  $_.Name -ieq 'python312.dll'
} | Remove-Item -Force
Get-ChildItem -LiteralPath $stageRoot -Recurse -Directory |
  Where-Object { $_.Name -eq '__pycache__' } |
  Sort-Object { $_.FullName.Length } -Descending |
  Remove-Item -Recurse -Force

$expectedImportPlugins = @(
  'analyzeplugin.dll',
  'dicomplugin.dll',
  'grdplugin.dll',
  'imagestackplugin.dll',
  'jp2plugin.dll',
  'metaimageplugin.dll',
  'nc4plugin.dll',
  'niftiplugin.dll',
  'nrrdplugin.dll',
  'rawplugin.dll',
  'rawslabsplugin.dll',
  'rawslicesplugin.dll',
  'tiffplugin.dll',
  'tomplugin.dll',
  'txmplugin.dll',
  'vgiplugin.dll'
)
$actualImportPlugins = @(
  Get-ChildItem -LiteralPath (Join-Path $stageRoot 'importplugins') `
                -File -Filter '*.dll' | Sort-Object Name |
    Select-Object -ExpandProperty Name
)
Assert-ExactManifest 'Import plugin' $expectedImportPlugins $actualImportPlugins

$requiredPaths = @(
  'platforms\qwindows.dll',
  'assets\scripts\import\raw\raw.json',
  'assets\scripts\import\raw\raw.py',
  'assets\scripts\import\numpy_array\numpy_array.json',
  'assets\scripts\import\numpy_array\numpy_array.py',
  'python313.dll',
  'python3.dll',
  'python313._pth',
  'tiffdecodehelper.exe',
  'python\python.exe',
  'python\python313.dll',
  'python\Lib\site-packages\numpy\__init__.py',
  'VCPKG_INSTALLED.txt'
)
foreach ($requiredPath in $requiredPaths)
  {
    if (!(Test-Path -LiteralPath (Join-Path $stageRoot $requiredPath) -PathType Leaf))
      {
        throw "Portable package is missing: $requiredPath"
      }
  }

$forbidden = @(
  Get-ChildItem -LiteralPath $stageRoot -Recurse -Force | Where-Object {
    (!$_.PSIsContainer -and
      ($_.Extension -in @('.lib', '.exp', '.pdb', '.ilk', '.obj', '.pyc') -or
       $_.Name -ieq 'python312.dll')) -or
    ($_.PSIsContainer -and $_.Name -eq '__pycache__')
  }
)
if ($forbidden.Count -ne 0)
  {
    throw "Forbidden development artifacts remain in the stage: $($forbidden.FullName)"
  }

$expectedRenderPlugins = @(
  'meshpaintplugin.dll',
  'meshsimplifyplugin.dll',
  'ITK\binarythinningplugin.dll',
  'ITK\connectedcomponentplugin.dll',
  'ITK\distancemapplugin.dll',
  'ITK\Smoothing\edgepreservingsmoothingplugin.dll',
  'ITK\Smoothing\smoothingplugin.dll',
  'ITK\Smoothing\vedplugin.dll'
)
$expectedMopPlugins = @()
$expectedHelpDocuments = @(
  'DrishtiPaint.pdf',
  'FileFormats.pdf',
  'MainComandDialog.pdf',
  'Mops.pdf',
  'WidgetsHelp.pdf'
)
$renderPlugins = Get-RelativeFiles (Join-Path $stageRoot 'renderplugins') '*.dll'
$mopPluginRoot = Join-Path $stageRoot 'mopplugins'
$mopPlugins = @(if (Test-Path -LiteralPath $mopPluginRoot -PathType Container) {
  Get-RelativeFiles $mopPluginRoot '*.dll'
} else { @() }
)
$helpDocuments = Get-RelativeFiles (Join-Path $stageRoot 'docs') '*.pdf'
Assert-ExactManifest 'Render plugin' $expectedRenderPlugins $renderPlugins
Assert-ExactManifest 'MOP plugin' $expectedMopPlugins $mopPlugins
Assert-ExactManifest 'Help document' $expectedHelpDocuments $helpDocuments

$scriptRoot = Join-Path $stageRoot 'assets\scripts'
$scriptDescriptors = @(Get-ChildItem -LiteralPath $scriptRoot `
  -Recurse -File -Filter '*.json')
foreach ($descriptor in $scriptDescriptors)
  {
    try
      {
        $definition = Get-Content -LiteralPath $descriptor.FullName -Raw |
          ConvertFrom-Json
      }
    catch
      {
        throw "Invalid script JSON $($descriptor.FullName): $($_.Exception.Message)"
      }

    if (!$definition.script -or
        [string]::IsNullOrWhiteSpace($definition.script.ToString()))
      {
        throw "Script descriptor has no script entry: $($descriptor.FullName)"
      }

    $scriptFile = Get-NormalizedPath (
      Join-Path $descriptor.DirectoryName $definition.script.ToString())
    Assert-PathWithin $scriptFile $scriptRoot
    if (!(Test-Path -LiteralPath $scriptFile -PathType Leaf))
      {
        throw "Script descriptor references a missing file: $scriptFile"
      }
  }
if ($scriptDescriptors.Count -ne 10)
  {
    throw "Expected 10 script descriptors, found $($scriptDescriptors.Count)"
  }

$sourceExtensions = @('.c', '.cc', '.cpp', '.h', '.hpp', '.pri', '.pro',
                      '.qrc', '.ui')
$productSourceFiles = @(
  Get-ChildItem -LiteralPath $RepositoryRoot -File | Where-Object {
    $_.Extension -in $sourceExtensions
  }
  foreach ($sourceDirectory in @('common', 'drishti', 'tools'))
    {
      Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot $sourceDirectory) `
                    -Recurse -File | Where-Object {
        $_.Extension -in $sourceExtensions
      }
    }
)
if ($productSourceFiles.Count -eq 0)
  {
    throw 'No product source files were found for the freshness audit.'
  }
$newestSource = $productSourceFiles |
  Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
$builtProductPaths = @($mainPrograms | ForEach-Object { $_ })
$builtProductPaths += @($expectedImportPlugins | ForEach-Object {
  Join-Path 'importplugins' $_
})
$builtProductPaths += @($expectedRenderPlugins | ForEach-Object {
  Join-Path 'renderplugins' $_
})
$builtProductPaths += @($expectedMopPlugins | ForEach-Object {
  Join-Path 'mopplugins' $_
})
foreach ($relativeProduct in $builtProductPaths)
  {
    $product = Get-Item -LiteralPath (Join-Path $stageRoot $relativeProduct)
    if ($product.LastWriteTimeUtc -lt $newestSource.LastWriteTimeUtc)
      {
        throw "Stale build product $relativeProduct ($($product.LastWriteTimeUtc.ToString('o'))) " +
              "predates source $($newestSource.FullName) " +
              "($($newestSource.LastWriteTimeUtc.ToString('o')))."
      }
  }

$windowsInboxDependencies = @(
  'advapi32.dll',
  'avicap32.dll',
  'bcrypt.dll',
  'comctl32.dll',
  'comdlg32.dll',
  'crypt32.dll',
  'd3d11.dll',
  'd3d9.dll',
  'dnsapi.dll',
  'dwmapi.dll',
  'dxgi.dll',
  'dxva2.dll',
  'evr.dll',
  'gdi32.dll',
  'glu32.dll',
  'imagehlp.dll',
  'imm32.dll',
  'iphlpapi.dll',
  'kernel32.dll',
  'mf.dll',
  'mfplat.dll',
  'mfreadwrite.dll',
  'mmdevapi.dll',
  'mpr.dll',
  'msvcrt.dll',
  'ncrypt.dll',
  'netapi32.dll',
  'ole32.dll',
  'oleaut32.dll',
  'opengl32.dll',
  'propsys.dll',
  'rpcrt4.dll',
  'secur32.dll',
  'shell32.dll',
  'shlwapi.dll',
  'user32.dll',
  'userenv.dll',
  'uxtheme.dll',
  'version.dll',
  'winmm.dll',
  'ws2_32.dll',
  'wsock32.dll',
  'wtsapi32.dll'
)
$peFiles = @(Get-ChildItem -LiteralPath $stageRoot -Recurse -File |
  Where-Object { $_.Extension -in @('.exe', '.dll', '.pyd') })
$missingDependencies = New-Object System.Collections.Generic.List[string]
foreach ($peFile in $peFiles)
  {
    Assert-X64Pe $DumpbinPath $peFile.FullName
    $relativePe = $peFile.FullName.Substring($stageRoot.Length + 1)
    foreach ($dependency in Get-PeDependencies $DumpbinPath $peFile.FullName)
      {
        if ((Test-PackagedDependency $stageRoot $peFile $dependency) -or
            $dependency.StartsWith('api-ms-win-') -or
            $dependency.StartsWith('ext-ms-win-') -or
            $dependency -in $windowsInboxDependencies)
          {
            continue
          }
        $missingDependencies.Add("$relativePe -> $dependency")
      }
  }
if ($missingDependencies.Count -ne 0)
  {
    throw "Portable dependency closure is incomplete:`n$($missingDependencies -join "`n")"
  }

$gitCommit = (& git -C $RepositoryRoot rev-parse HEAD).Trim()
$dirtyEntryCount = @(& git -C $RepositoryRoot status --porcelain --untracked-files=no).Count
$buildInfo = @(
  "Package: $PackageName",
  "Created: $([DateTimeOffset]::Now.ToString('o'))",
  "Source commit: $gitCommit",
  "Source working-tree entries: $dirtyEntryCount",
  "Newest audited product source: $($newestSource.FullName)",
  "Newest audited source timestamp: $($newestSource.LastWriteTimeUtc.ToString('o'))",
  "Architecture: x64",
  "Toolchain: MSVC 2019 / Qt 5.15.2",
  "VC Runtime: $((Get-Item -LiteralPath (Join-Path $stageRoot 'vcruntime140.dll')).VersionInfo.FileVersionRaw)",
  'Python automatic site import: disabled',
  'Runtime contract: CPU compute plus Intel/AMD Desktop OpenGL display'
)
[System.IO.File]::WriteAllLines(
  (Join-Path $stageRoot 'BUILD_INFO.txt'), $buildInfo,
  [System.Text.UTF8Encoding]::new($false))

$hashLines = Get-ChildItem -LiteralPath $stageRoot -Recurse -File |
  Where-Object { $_.Name -ne 'SHA256SUMS.txt' } |
  Sort-Object FullName | ForEach-Object {
    $relative = $_.FullName.Substring($stageRoot.Length + 1).Replace('\', '/')
    $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $relative"
  }
[System.IO.File]::WriteAllLines(
  (Join-Path $stageRoot 'SHA256SUMS.txt'), $hashLines,
  [System.Text.UTF8Encoding]::new($false))

$zipPath = Join-Path $OutputDirectory ($PackageName + '.zip')
Assert-PathWithin $zipPath $OutputDirectory
if (Test-Path -LiteralPath $zipPath)
  {
    Remove-Item -LiteralPath $zipPath -Force
  }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory(
  $stageRoot, $zipPath,
  [System.IO.Compression.CompressionLevel]::Optimal,
  $true)

$zipHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
$fileCount = @(Get-ChildItem -LiteralPath $stageRoot -Recurse -File).Count
$stageBytes = (Get-ChildItem -LiteralPath $stageRoot -Recurse -File |
  Measure-Object Length -Sum).Sum

if (!$KeepStage)
  {
    Assert-PathWithin $stageRoot $stageContainer
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
  }

[PSCustomObject]@{
  Zip = $zipPath
  Sha256 = $zipHash
  Files = $fileCount
  UncompressedBytes = $stageBytes
  ImportPlugins = $actualImportPlugins.Count
  RenderPlugins = $renderPlugins.Count
  MopPlugins = $mopPlugins.Count
  HelpDocuments = $helpDocuments.Count
  ScriptDescriptors = $scriptDescriptors.Count
  PeFilesAudited = $peFiles.Count
}
