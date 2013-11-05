TEMPLATE = lib

QT += opengl xml network

CONFIG += release plugin

TARGET = vedplugin


FORMS += ../../../propertyeditor.ui

win32 {
DESTDIR = ../../../../bin/renderplugins/ITK/Smoothing

ITKVer = 4.3
InsightToolkit = InsightToolkit-$${ITKVer}.1

message(ITK version $$ITKVer)
message(ITK $$InsightToolkit)

INCLUDEPATH +=  . \
 	../../../ \
	c:\Qt\include \
	c:\drishtilib \
	c:\drishtilib\glew-1.5.4\include \
	D:\ITK\Examples\ITKIOFactoryRegistration \
	D:\\$$InsightToolkit\Modules\Video\Filtering\include \
	D:\\$$InsightToolkit\Modules\Video\IO\include \
	D:\\$$InsightToolkit\Modules\Video\Core\include \
	D:\\$$InsightToolkit\Modules\Nonunit\Review\include \
	D:\\$$InsightToolkit\Modules\Registration\RegistrationMethodsv4\include \
	D:\\$$InsightToolkit\Modules\Registration\Metricsv4\include \
	D:\\$$InsightToolkit\Modules\Numerics\Optimizersv4\include \
	D:\\$$InsightToolkit\Modules\Segmentation\LevelSetsv4\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageFusion\include \
	D:\\$$InsightToolkit\Modules\IO\TransformMatlab\include \
	D:\\$$InsightToolkit\Modules\IO\TransformInsightLegacy\include \
	D:\\$$InsightToolkit\Modules\IO\TransformHDF5\include \
	D:\\$$InsightToolkit\Modules\IO\TransformBase\include \
	D:\\$$InsightToolkit\Modules\IO\HDF5\include \
	D:\\$$InsightToolkit\Modules\IO\CSV\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\HDF5\src \
	D:\\$$InsightToolkit\Modules\IO\Mesh\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\GIFTI\src\gifticlib \
	D:\\$$InsightToolkit\Modules\Segmentation\Watersheds\include \
	D:\\$$InsightToolkit\Modules\Segmentation\Voronoi\include \
	D:\\$$InsightToolkit\Modules\Bridge\VTK\include \
	D:\\$$InsightToolkit\Modules\Filtering\SpatialFunction\include \
	D:\\$$InsightToolkit\Modules\Segmentation\RegionGrowing\include \
	D:\\$$InsightToolkit\Modules\Filtering\QuadEdgeMeshFiltering\include \
	D:\\$$InsightToolkit\Modules\Numerics\NeuralNetworks\include \
	D:\\$$InsightToolkit\Modules\Segmentation\MarkovRandomFieldsClassifiers\include \
	D:\\$$InsightToolkit\Modules\Segmentation\LabelVoting\include \
	D:\\$$InsightToolkit\Modules\Segmentation\KLMRegionGrowing\include \
o	D:\\$$InsightToolkit\Modules\IO\Siemens\include \
	D:\\$$InsightToolkit\Modules\IO\RAW\include \
	D:\\$$InsightToolkit\Modules\IO\GE\include \
	D:\\$$InsightToolkit\Modules\IO\IPL\include \
	D:\\$$InsightToolkit\Modules\Registration\FEM\include \
	D:\\$$InsightToolkit\Modules\Registration\PDEDeformable\include \
	D:\\$$InsightToolkit\Modules\Numerics\FEM\include \
	D:\\$$InsightToolkit\Modules\Registration\Common\include \
	D:\\$$InsightToolkit\Modules\IO\SpatialObjects\include \
	D:\\$$InsightToolkit\Modules\IO\XML\include \
	D:\\$$InsightToolkit\Modules\Numerics\Eigen\include \
	D:\\$$InsightToolkit\Modules\Filtering\DisplacementField\include \
	D:\\$$InsightToolkit\Modules\Filtering\DiffusionTensorImage\include \
	D:\\$$InsightToolkit\Modules\Segmentation\DeformableMesh\include \
	D:\\$$InsightToolkit\Modules\Filtering\Deconvolution\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\DICOMParser\src\DICOMParser \
	D:\\$$InsightToolkit\Modules\Filtering\Convolution\include \
	D:\\$$InsightToolkit\Modules\Filtering\FFT\include \
	D:\\$$InsightToolkit\Modules\Filtering\Colormap\include \
	D:\\$$InsightToolkit\Modules\Segmentation\Classifiers\include \
	D:\\$$InsightToolkit\Modules\Segmentation\BioCell\include \
	D:\\$$InsightToolkit\Modules\Filtering\BiasCorrection\include \
	D:\\$$InsightToolkit\Modules\Numerics\Polynomials\include \
	D:\\$$InsightToolkit\Modules\Filtering\AntiAlias\include \
	D:\\$$InsightToolkit\Modules\Segmentation\LevelSets\include \
	D:\\$$InsightToolkit\Modules\Segmentation\SignedDistanceFunction\include \
	D:\\$$InsightToolkit\Modules\Numerics\Optimizers\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageFeature\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageSources\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageGradient\include \
	D:\\$$InsightToolkit\Modules\Filtering\Smoothing\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageCompare\include \
	D:\\$$InsightToolkit\Modules\Filtering\FastMarching\include \
	D:\\$$InsightToolkit\Modules\Core\QuadEdgeMesh\include \
	D:\\$$InsightToolkit\Modules\Filtering\DistanceMap\include \
	D:\\$$InsightToolkit\Modules\Numerics\NarrowBand\include \
	D:\\$$InsightToolkit\Modules\Filtering\BinaryMathematicalMorphology\include \
	D:\\$$InsightToolkit\Modules\Filtering\LabelMap\include \
	D:\\$$InsightToolkit\Modules\Filtering\MathematicalMorphology\include \
	D:\\$$InsightToolkit\Modules\Segmentation\ConnectedComponents\include \
	D:\\$$InsightToolkit\Modules\Filtering\Thresholding\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageLabel\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageIntensity\include \
	D:\\$$InsightToolkit\Modules\Filtering\Path\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageStatistics\include \
	D:\\$$InsightToolkit\Modules\Core\SpatialObjects\include \
	D:\\$$InsightToolkit\Modules\Core\Mesh\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageCompose\include \
	D:\\$$InsightToolkit\Modules\Core\TestKernel\include \
	D:\\$$InsightToolkit\Modules\IO\VTK\include \
	D:\\$$InsightToolkit\Modules\IO\Stimulate\include \
	D:\\$$InsightToolkit\Modules\IO\PNG\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\PNG\src \
	D:\\$$InsightToolkit\Modules\IO\NRRD\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\NrrdIO\src\NrrdIO \
	D:\\$$InsightToolkit\Modules\IO\NIFTI\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\NIFTI\src\nifti\znzlib \
	D:\\$$InsightToolkit\Modules\ThirdParty\NIFTI\src\nifti\niftilib \
	D:\\$$InsightToolkit\Modules\IO\Meta\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\MetaIO\src\MetaIO \
	D:\\$$InsightToolkit\Modules\IO\LSM\include \
	D:\\$$InsightToolkit\Modules\IO\TIFF\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\TIFF\src \
	D:\\$$InsightToolkit\Modules\IO\GIPL\include \
	D:\\$$InsightToolkit\Modules\IO\GDCM\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Utilities\C99 \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\DataStructureAndEncodingDefinition \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\MessageExchangeDefinition \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\InformationObjectDefinition \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\Common \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\DataDictionary \
	D:\\$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\MediaStorageAndFileFormat \
	D:\\$$InsightToolkit\Modules\IO\JPEG\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\JPEG\src \
	D:\\$$InsightToolkit\Modules\ThirdParty\ZLIB\src \
	D:\\$$InsightToolkit\Modules\ThirdParty\OpenJPEG\src\openjpeg \
	D:\\$$InsightToolkit\Modules\ThirdParty\Expat\src\expat \
	D:\\$$InsightToolkit\Modules\IO\BioRad\include \
	D:\\$$InsightToolkit\Modules\IO\BMP\include \
	D:\\$$InsightToolkit\Modules\IO\ImageBase\include \
	D:\\$$InsightToolkit\Modules\Filtering\AnisotropicSmoothing\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageGrid\include \
	D:\\$$InsightToolkit\Modules\Core\ImageFunction\include \
	D:\\$$InsightToolkit\Modules\Core\Transform\include \
	D:\\$$InsightToolkit\Modules\Numerics\Statistics\include \
	D:\\$$InsightToolkit\Modules\Core\ImageAdaptors\include \
	D:\\$$InsightToolkit\Modules\Filtering\CurvatureFlow\include \
	D:\\$$InsightToolkit\Modules\Filtering\ImageFilterBase\include \
	D:\\$$InsightToolkit\Modules\Core\FiniteDifference\include \
	D:\\$$InsightToolkit\Modules\Core\Common\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\VNLInstantiation\include \
	D:\\$$InsightToolkit\Modules\ThirdParty\VNL\src\vxl\core \
	D:\\$$InsightToolkit\Modules\ThirdParty\VNL\src\vxl\vcl \
	D:\\$$InsightToolkit\Modules\ThirdParty\VNL\src\vxl\v3p\netlib \
	D:\ITK\Modules\ThirdParty\TIFF\src\itktiff \
	D:\ITK\Modules\ThirdParty\TIFF\src \
	D:\ITK\Modules\ThirdParty\HDF5\src \
	D:\ITK\Modules\ThirdParty\DICOMParser\src\DICOMParser \
	D:\ITK\Modules\ThirdParty\PNG\src \
	D:\ITK\Modules\ThirdParty\NrrdIO\src\NrrdIO \
	D:\ITK\Modules\ThirdParty\MetaIO\src\MetaIO \
	D:\ITK\Modules\ThirdParty\JPEG\src \
	D:\ITK\Modules\ThirdParty\GDCM\src\gdcm\Source\Common \
	D:\ITK\Modules\ThirdParty\GDCM \
	D:\ITK\Modules\ThirdParty\ZLIB\src \
	D:\ITK\Modules\ThirdParty\OpenJPEG\src\openjpeg \
	D:\ITK\Modules\ThirdParty\Expat\src\expat \
	D:\ITK\Modules\IO\ImageBase \
	D:\ITK\Modules\ThirdParty\Netlib \
	D:\ITK\Modules\ThirdParty\KWSys\src \
	D:\ITK\Modules\ThirdParty\VNL\src\vxl\core \
	D:\ITK\Modules\Core\Common \
	D:\ITK\Modules\ThirdParty\VNL\src\vxl\vcl \
	D:\ITK\Modules\ThirdParty\VNL\src\vxl\v3p\netlib

QMAKE_LIBDIR += ../../common \
	 d:\ITK\lib\Release \
	c:\Qt\lib \
	c:\drishtilib\GL \ 
	c:\drishtilib\glew-1.5.4\lib

LIBS += common.lib \
	QGLViewer2.lib \
	glew32.lib \
 	Advapi32.lib \
	User32.lib \
	Gdi32.lib \
	Ws2_32.lib \
	Rpcrt4.lib \
	itksys-$${ITKVer}.lib \
	itkvnl_algo-$${ITKVer}.lib \
	itkvnl-$${ITKVer}.lib \
	itkv3p_netlib-$${ITKVer}.lib \
	ITKCommon-$${ITKVer}.lib \
	itkNetlibSlatec-$${ITKVer}.lib \
	ITKStatistics-$${ITKVer}.lib \
	ITKIOImageBase-$${ITKVer}.lib \
	ITKIOBMP-$${ITKVer}.lib \
	ITKIOBioRad-$${ITKVer}.lib \
	ITKEXPAT-$${ITKVer}.lib \
	itkopenjpeg-$${ITKVer}.lib \
	itkzlib-$${ITKVer}.lib \
	itkgdcmDICT-$${ITKVer}.lib \
	itkgdcmMSFF-$${ITKVer}.lib \
	ITKIOGDCM-$${ITKVer}.lib \
	ITKIOGIPL-$${ITKVer}.lib \
	itkjpeg-$${ITKVer}.lib \
	ITKIOJPEG-$${ITKVer}.lib \
	itktiff-$${ITKVer}.lib \
	ITKIOTIFF-$${ITKVer}.lib \
	ITKIOLSM-$${ITKVer}.lib \
	ITKMetaIO-$${ITKVer}.lib \
	ITKIOMeta-$${ITKVer}.lib \
	ITKznz-$${ITKVer}.lib \
	ITKniftiio-$${ITKVer}.lib \
	ITKIONIFTI-$${ITKVer}.lib \
	ITKNrrdIO-$${ITKVer}.lib \
	ITKIONRRD-$${ITKVer}.lib \
	itkpng-$${ITKVer}.lib \
	ITKIOPNG-$${ITKVer}.lib \
	ITKIOStimulate-$${ITKVer}.lib \
	ITKIOVTK-$${ITKVer}.lib \
	ITKMesh-$${ITKVer}.lib \
	ITKSpatialObjects-$${ITKVer}.lib \
	ITKPath-$${ITKVer}.lib \
	ITKLabelMap-$${ITKVer}.lib \
	ITKQuadEdgeMesh-$${ITKVer}.lib \
	ITKOptimizers-$${ITKVer}.lib \
	ITKPolynomials-$${ITKVer}.lib \
	ITKBiasCorrection-$${ITKVer}.lib \
	ITKBioCell-$${ITKVer}.lib \
	ITKDICOMParser-$${ITKVer}.lib \
	ITKIOXML-$${ITKVer}.lib \
	ITKIOSpatialObjects-$${ITKVer}.lib \
	ITKFEM-$${ITKVer}.lib \
	ITKIOIPL-$${ITKVer}.lib \
	ITKIOGE-$${ITKVer}.lib \
	ITKIOSiemens-$${ITKVer}.lib \
	ITKKLMRegionGrowing-$${ITKVer}.lib \
	ITKVTK-$${ITKVer}.lib \
	ITKWatersheds-$${ITKVer}.lib \
	ITKgiftiio-$${ITKVer}.lib \
	ITKIOMesh-$${ITKVer}.lib \
	itkhdf5_cpp-$${ITKVer}.lib \
	itkhdf5-$${ITKVer}.lib \
	ITKIOCSV-$${ITKVer}.lib \
	ITKIOHDF5-$${ITKVer}.lib \
	ITKIOTransformBase-$${ITKVer}.lib \
	ITKIOTransformHDF5-$${ITKVer}.lib \
	ITKIOTransformInsightLegacy-$${ITKVer}.lib \
	ITKIOTransformMatlab-$${ITKVer}.lib \
	ITKOptimizersv4-$${ITKVer}.lib \
	ITKReview-$${ITKVer}.lib \
	ITKVideoCore-$${ITKVer}.lib \
	ITKVideoIO-$${ITKVer}.lib \
	itkgdcmIOD-$${ITKVer}.lib \
	itkgdcmDSED-$${ITKVer}.lib \
	itkgdcmCommon-$${ITKVer}.lib \
	itkgdcmjpeg8-$${ITKVer}.lib \
	itkgdcmjpeg12-$${ITKVer}.lib \
	itkgdcmjpeg16-$${ITKVer}.lib \
	ITKVNLInstantiation-$${ITKVer}.lib \
	itkv3p_lsqr-$${ITKVer}.lib \
	itkvcl-$${ITKVer}.lib

}

unix {
  !macx {

DESTDIR = ../../../../bin/renderplugins/ITK

ITKVer = 4.3
InsightToolkit = /home/acl900/InsightToolkit-$${ITKVer}.0
ITK = /home/acl900/ITK

message(ITK version $$ITKVer)
message(ITK $$InsightToolkit)

INCLUDEPATH += ../../../ \
	/usr/local/include \
	$$InsightToolkit/Modules/Core/Common/include \
	$$InsightToolkit/Modules/Core/FiniteDifference/include \
	$$InsightToolkit/Modules/Core/ImageAdaptors/include \
	$$InsightToolkit/Modules/IO/GDCM/include \
	$$InsightToolkit/Modules/IO/ImageBase/include \
	$$InsightToolkit/Modules/IO/Meta/include \
	$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/Common \
	$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/DataDictionary \
	$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/DataStructureAndEncodingDefinition \
	$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/MediaStorageAndFileFormat \
	$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/InformationObjectDefinition \
	$$InsightToolkit/Modules/ThirdParty/MetaIO/src/MetaIO \
	$$InsightToolkit/Modules/ThirdParty/VNL/src/vxl/core \
	$$InsightToolkit/Modules/ThirdParty/VNL/src/vxl/vcl \
	$$InsightToolkit/Modules/ThirdParty/ZLIB/src \
	$$InsightToolkit/Modules/Segmentation/RegionGrowing/include \
	$$InsightToolkit/Modules/Segmentation/ConnectedComponents/include \
	$$InsightToolkit/Modules/Core/ImageFunction/include \
	$$InsightToolkit/Modules/Filtering/Thresholding/include \
	$$InsightToolkit/Modules/Filtering/ImageIntensity/include \
	$$InsightToolkit/Modules/Filtering/ImageFeature/include \
	$$InsightToolkit/Modules/Filtering/ImageSources/include \
	$$InsightToolkit/Modules/Filtering/ImageStatistics/include \
	$$InsightToolkit/Modules/Filtering/ImageFilterBase/include \
	$$InsightToolkit/Modules/Filtering/AnisotropicSmoothing/include \
	$$InsightToolkit/Modules/Filtering/CurvatureFlow/include \
	$$InsightToolkit/Modules/Filtering/Smoothing/include \
	$$ITK/Modules/Core/Common \
	$$ITK/Modules/IO/ImageBase \
	$$ITK/Modules/ThirdParty/GDCM \
	$$ITK/Modules/ThirdParty/GDCM/src/gdcm/Source/Common \
	$$ITK/Modules/ThirdParty/KWSys/src \
	$$ITK/Modules/ThirdParty/VNL/src/vxl/core \
	$$ITK/Modules/ThirdParty/VNL/src/vxl/vcl \
	$$ITK/Modules/ThirdParty/MetaIO/src/MetaIO \
	$$ITK/Modules/ThirdParty/ZLIB/src

  QMAKE_LIBDIR += ../../common /usr/lib /usr/lib/x86_64-linux-gnu
  QMAKE_LIBDIR += /home/acl900/ITK/lib

  LIBS += -lcommon \
	 -lQGLViewer \
        -lGLEW \
 	-lglut \
	-lGLU

  LIBS += -lm -lstdc++ \
        -litksys-$$ITKVer \        
	-litkzlib-$$ITKVer \
	-litkvnl_algo-$$ITKVer \
	-litkvnl-$$ITKVer \
	-litkvcl-$$ITKVer \
	-litkv3p_netlib-$$ITKVer \
	-litkv3p_netlib-$$ITKVer \
	-litkv3p_lsqr-$$ITKVer \
	-litktiff-$$ITKVer \
	-litksys-$$ITKVer \
	-litkpng-$$ITKVer \
	-litkopenjpeg-$$ITKVer \
	-litkjpeg-$$ITKVer \
	-litkhdf5_cpp-$$ITKVer \
	-litkhdf5-$$ITKVer \
	-litkgdcmuuid-$$ITKVer \
	-litkgdcmjpeg8-$$ITKVer \
	-litkgdcmjpeg16-$$ITKVer \
	-litkgdcmjpeg12-$$ITKVer \
	-litkgdcmMSFF-$$ITKVer \
	-litkgdcmIOD-$$ITKVer \
	-litkgdcmDSED-$$ITKVer \
	-litkgdcmDICT-$$ITKVer \
	-litkgdcmCommon-$$ITKVer \
	-litkNetlibSlatec-$$ITKVer \
	-lITKznz-$$ITKVer \
	-lITKniftiio-$$ITKVer \
	-lITKgiftiio-$$ITKVer \
	-lITKWatersheds-$$ITKVer \
	-lITKVideoIO-$$ITKVer \
	-lITKVideoCore-$$ITKVer \
	-lITKVTK-$$ITKVer \
	-lITKVNLInstantiation-$$ITKVer \
	-lITKStatistics-$$ITKVer \
	-lITKSpatialObjects-$$ITKVer \
	-lITKReview-$$ITKVer \
	-lITKQuadEdgeMesh-$$ITKVer \
	-lITKPolynomials-$$ITKVer \
	-lITKPath-$$ITKVer \
	-lITKOptimizersv4-$$ITKVer \
	-lITKOptimizers-$$ITKVer \
	-lITKNrrdIO-$$ITKVer \
	-lITKMetaIO-$$ITKVer \
	-lITKMesh-$$ITKVer \
	-lITKLabelMap-$$ITKVer \
	-lITKKLMRegionGrowing-$$ITKVer \
	-lITKIOXML-$$ITKVer \
	-lITKIOVTK-$$ITKVer \
	-lITKIOTransformMatlab-$$ITKVer \
	-lITKIOTransformInsightLegacy-$$ITKVer \
	-lITKIOTransformHDF5-$$ITKVer \
	-lITKIOTransformBase-$$ITKVer \
	-lITKIOTIFF-$$ITKVer \
	-lITKIOStimulate-$$ITKVer \
	-lITKIOSpatialObjects-$$ITKVer \
	-lITKIOSiemens-$$ITKVer \
	-lITKIOPNG-$$ITKVer \
	-lITKIONRRD-$$ITKVer \
	-lITKIONIFTI-$$ITKVer \
	-lITKIOMeta-$$ITKVer \
	-lITKIOMesh-$$ITKVer \
	-lITKIOLSM-$$ITKVer \
	-lITKIOJPEG-$$ITKVer \
	-lITKIOImageBase-$$ITKVer \
	-lITKIOIPL-$$ITKVer \
	-lITKIOHDF5-$$ITKVer \
	-lITKIOGIPL-$$ITKVer \
	-lITKIOGE-$$ITKVer \
	-lITKIOGDCM-$$ITKVer \
	-lITKIOCSV-$$ITKVer \
	-lITKIOBioRad-$$ITKVer \
	-lITKIOBMP-$$ITKVer \
	-lITKFEM-$$ITKVer \
	-lITKEXPAT-$$ITKVer \
	-lITKDICOMParser-$$ITKVer \
	-lITKCommon-$$ITKVer \
	-lITKBioCell-$$ITKVer \
	-lITKBiasCorrection-$$ITKVer

 }
}

macx {
  QMAKE_CFLAGS_X86_64 += -mmacosx-version-min=10.7
  QMAKE_CXXFLAGS_X86_64 = $$QMAKE_CFLAGS_X86_64

  DESTDIR = ../../../../bin/drishti.app/renderplugins/ITK/Smoothing

  ITKVer = 4.3
  InsightToolkit = /Users/acl900/InsightToolkit-$${ITKVer}.1
  ITK = /Users/acl900/ITK

  message(ITK version $$ITKVer)
  message(ITK $$InsightToolkit)

  INCLUDEPATH += ../../../ \
        ../../../../../Library/Frameworks/QGLViewer.framework/Headers \
	/usr/local/include \
	/Users/acl900/$$InsightToolkit/Modules/Core/Common/include \
	/Users/acl900/$$InsightToolkit/Modules/Core/FiniteDifference/include \
	/Users/acl900/$$InsightToolkit/Modules/Core/ImageAdaptors/include \
	/Users/acl900/$$InsightToolkit/Modules/IO/GDCM/include \
	/Users/acl900/$$InsightToolkit/Modules/IO/ImageBase/include \
	/Users/acl900/$$InsightToolkit/Modules/IO/Meta/include \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/Common \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/DataDictionary \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/DataStructureAndEncodingDefinition \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/MediaStorageAndFileFormat \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/GDCM/src/gdcm/Source/InformationObjectDefinition \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/MetaIO/src/MetaIO \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/VNL/src/vxl/core \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/VNL/src/vxl/vcl \
	/Users/acl900/$$InsightToolkit/Modules/ThirdParty/ZLIB/src \
	/Users/acl900/$$InsightToolkit/Modules/Segmentation/RegionGrowing/include \
	/Users/acl900/$$InsightToolkit/Modules/Segmentation/ConnectedComponents/include \
	/Users/acl900/$$InsightToolkit/Modules/Core/ImageFunction/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/Thresholding/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/ImageIntensity/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/ImageFeature/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/ImageSource/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/ImageStatistics/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/ImageFilterBase/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/AnisotropicSmoothing/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/CurvatureFlow/include \
	/Users/acl900/$$InsightToolkit/Modules/Filtering/Smoothing/include \
	/Users/acl900/ITK/Modules/Core/Common \
	/Users/acl900/ITK/Modules/IO/ImageBase \
	/Users/acl900/ITK/Modules/ThirdParty/GDCM \
	/Users/acl900/ITK/Modules/ThirdParty/GDCM/src/gdcm/Source/Common \
	/Users/acl900/ITK/Modules/ThirdParty/KWSys/src \
	/Users/acl900/ITK/Modules/ThirdParty/VNL/src/vxl/core \
	/Users/acl900/ITK/Modules/ThirdParty/VNL/src/vxl/vcl \
	/Users/acl900/ITK/Modules/ThirdParty/MetaIO/src/MetaIO \
	/Users/acl900/ITK/Modules/ThirdParty/ZLIB/src

  QMAKE_LIBDIR += /Users/acl900/ITK/lib

  LIBS += -framework ApplicationServices
  LIBS += -L../../common -L/usr/local/lib -F../../../../../Library/Frameworks -L../../../../../Library/Frameworks
  LIBS += -lcommon -lGLEW -framework QGLViewer -framework GLUT

  LIBS += -lm -lstdc++ \
        -litksys-$$ITKVer \        
	-litkzlib-$$ITKVer \
	-litkvnl_algo-$$ITKVer \
	-litkvnl-$$ITKVer \
	-litkvcl-$$ITKVer \
	-litkv3p_netlib-$$ITKVer \
	-litkv3p_netlib-$$ITKVer \
	-litkv3p_lsqr-$$ITKVer \
	-litktiff-$$ITKVer \
	-litksys-$$ITKVer \
	-litkpng-$$ITKVer \
	-litkopenjpeg-$$ITKVer \
	-litkjpeg-$$ITKVer \
	-litkhdf5_cpp-$$ITKVer \
	-litkhdf5-$$ITKVer \
	-litkgdcmuuid-$$ITKVer \
	-litkgdcmjpeg8-$$ITKVer \
	-litkgdcmjpeg16-$$ITKVer \
	-litkgdcmjpeg12-$$ITKVer \
	-litkgdcmMSFF-$$ITKVer \
	-litkgdcmIOD-$$ITKVer \
	-litkgdcmDSED-$$ITKVer \
	-litkgdcmDICT-$$ITKVer \
	-litkgdcmCommon-$$ITKVer \
	-litkNetlibSlatec-$$ITKVer \
	-lITKznz-$$ITKVer \
	-lITKniftiio-$$ITKVer \
	-lITKgiftiio-$$ITKVer \
	-lITKWatersheds-$$ITKVer \
	-lITKVideoIO-$$ITKVer \
	-lITKVideoCore-$$ITKVer \
	-lITKVTK-$$ITKVer \
	-lITKVNLInstantiation-$$ITKVer \
	-lITKStatistics-$$ITKVer \
	-lITKSpatialObjects-$$ITKVer \
	-lITKReview-$$ITKVer \
	-lITKQuadEdgeMesh-$$ITKVer \
	-lITKPolynomials-$$ITKVer \
	-lITKPath-$$ITKVer \
	-lITKOptimizersv4-$$ITKVer \
	-lITKOptimizers-$$ITKVer \
	-lITKNrrdIO-$$ITKVer \
	-lITKMetaIO-$$ITKVer \
	-lITKMesh-$$ITKVer \
	-lITKLabelMap-$$ITKVer \
	-lITKKLMRegionGrowing-$$ITKVer \
	-lITKIOXML-$$ITKVer \
	-lITKIOVTK-$$ITKVer \
	-lITKIOTransformMatlab-$$ITKVer \
	-lITKIOTransformInsightLegacy-$$ITKVer \
	-lITKIOTransformHDF5-$$ITKVer \
	-lITKIOTransformBase-$$ITKVer \
	-lITKIOTIFF-$$ITKVer \
	-lITKIOStimulate-$$ITKVer \
	-lITKIOSpatialObjects-$$ITKVer \
	-lITKIOSiemens-$$ITKVer \
	-lITKIOPNG-$$ITKVer \
	-lITKIONRRD-$$ITKVer \
	-lITKIONIFTI-$$ITKVer \
	-lITKIOMeta-$$ITKVer \
	-lITKIOMesh-$$ITKVer \
	-lITKIOLSM-$$ITKVer \
	-lITKIOJPEG-$$ITKVer \
	-lITKIOImageBase-$$ITKVer \
	-lITKIOIPL-$$ITKVer \
	-lITKIOHDF5-$$ITKVer \
	-lITKIOGIPL-$$ITKVer \
	-lITKIOGE-$$ITKVer \
	-lITKIOGDCM-$$ITKVer \
	-lITKIOCSV-$$ITKVer \
	-lITKIOBioRad-$$ITKVer \
	-lITKIOBMP-$$ITKVer \
	-lITKFEM-$$ITKVer \
	-lITKEXPAT-$$ITKVer \
	-lITKDICOMParser-$$ITKVer \
	-lITKCommon-$$ITKVer \
	-lITKBioCell-$$ITKVer \
	-lITKBiasCorrection-$$ITKVer
}


HEADERS = ved.h \
	filter.h	

SOURCES = ved.cpp \
	filter.cpp
