TEMPLATE = lib

QT += opengl xml network

CONFIG += release plugin

TARGET = edgepreservingsmoothingplugin

include(../itk.pri)

FORMS += ../../../propertyeditor.ui

win32 {
DESTDIR = ../../../../bin/renderplugins/ITK/Smoothing

INCLUDEPATH +=  . \
 	../../../ \
	c:\Qt\include \
	c:\drishtilib \
	c:\drishtilib\glew-1.5.4\include \
	$$InsightToolkit\Modules\Video\Filtering\include \
	$$InsightToolkit\Modules\Video\IO\include \
	$$InsightToolkit\Modules\Video\Core\include \
	$$InsightToolkit\Modules\Nonunit\Review\include \
	$$InsightToolkit\Modules\Registration\RegistrationMethodsv4\include \
	$$InsightToolkit\Modules\Registration\Metricsv4\include \
	$$InsightToolkit\Modules\Numerics\Optimizersv4\include \
	$$InsightToolkit\Modules\Segmentation\LevelSetsv4\include \
	$$InsightToolkit\Modules\Filtering\ImageFusion\include \
	$$InsightToolkit\Modules\IO\TransformMatlab\include \
	$$InsightToolkit\Modules\IO\TransformInsightLegacy\include \
	$$InsightToolkit\Modules\IO\TransformHDF5\include \
	$$InsightToolkit\Modules\IO\TransformBase\include \
	$$InsightToolkit\Modules\IO\HDF5\include \
	$$InsightToolkit\Modules\IO\CSV\include \
	$$InsightToolkit\Modules\ThirdParty\HDF5\src \
	$$InsightToolkit\Modules\IO\Mesh\include \
	$$InsightToolkit\Modules\ThirdParty\GIFTI\src\gifticlib \
	$$InsightToolkit\Modules\Segmentation\Watersheds\include \
	$$InsightToolkit\Modules\Segmentation\Voronoi\include \
	$$InsightToolkit\Modules\Bridge\VTK\include \
	$$InsightToolkit\Modules\Filtering\SpatialFunction\include \
	$$InsightToolkit\Modules\Segmentation\RegionGrowing\include \
	$$InsightToolkit\Modules\Filtering\QuadEdgeMeshFiltering\include \
	$$InsightToolkit\Modules\Numerics\NeuralNetworks\include \
	$$InsightToolkit\Modules\Segmentation\MarkovRandomFieldsClassifiers\include \
	$$InsightToolkit\Modules\Segmentation\LabelVoting\include \
	$$InsightToolkit\Modules\Segmentation\KLMRegionGrowing\include \
o	$$InsightToolkit\Modules\IO\Siemens\include \
	$$InsightToolkit\Modules\IO\RAW\include \
	$$InsightToolkit\Modules\IO\GE\include \
	$$InsightToolkit\Modules\IO\IPL\include \
	$$InsightToolkit\Modules\Registration\FEM\include \
	$$InsightToolkit\Modules\Registration\PDEDeformable\include \
	$$InsightToolkit\Modules\Numerics\FEM\include \
	$$InsightToolkit\Modules\Registration\Common\include \
	$$InsightToolkit\Modules\IO\SpatialObjects\include \
	$$InsightToolkit\Modules\IO\XML\include \
	$$InsightToolkit\Modules\Numerics\Eigen\include \
	$$InsightToolkit\Modules\Filtering\DisplacementField\include \
	$$InsightToolkit\Modules\Filtering\DiffusionTensorImage\include \
	$$InsightToolkit\Modules\Segmentation\DeformableMesh\include \
	$$InsightToolkit\Modules\Filtering\Deconvolution\include \
	$$InsightToolkit\Modules\ThirdParty\DICOMParser\src\DICOMParser \
	$$InsightToolkit\Modules\Filtering\Convolution\include \
	$$InsightToolkit\Modules\Filtering\FFT\include \
	$$InsightToolkit\Modules\Filtering\Colormap\include \
	$$InsightToolkit\Modules\Segmentation\Classifiers\include \
	$$InsightToolkit\Modules\Segmentation\BioCell\include \
	$$InsightToolkit\Modules\Filtering\BiasCorrection\include \
	$$InsightToolkit\Modules\Numerics\Polynomials\include \
	$$InsightToolkit\Modules\Filtering\AntiAlias\include \
	$$InsightToolkit\Modules\Segmentation\LevelSets\include \
	$$InsightToolkit\Modules\Segmentation\SignedDistanceFunction\include \
	$$InsightToolkit\Modules\Numerics\Optimizers\include \
	$$InsightToolkit\Modules\Filtering\ImageFeature\include \
	$$InsightToolkit\Modules\Filtering\ImageSources\include \
	$$InsightToolkit\Modules\Filtering\ImageGradient\include \
	$$InsightToolkit\Modules\Filtering\Smoothing\include \
	$$InsightToolkit\Modules\Filtering\ImageCompare\include \
	$$InsightToolkit\Modules\Filtering\FastMarching\include \
	$$InsightToolkit\Modules\Core\QuadEdgeMesh\include \
	$$InsightToolkit\Modules\Filtering\DistanceMap\include \
	$$InsightToolkit\Modules\Numerics\NarrowBand\include \
	$$InsightToolkit\Modules\Filtering\BinaryMathematicalMorphology\include \
	$$InsightToolkit\Modules\Filtering\LabelMap\include \
	$$InsightToolkit\Modules\Filtering\MathematicalMorphology\include \
	$$InsightToolkit\Modules\Segmentation\ConnectedComponents\include \
	$$InsightToolkit\Modules\Filtering\Thresholding\include \
	$$InsightToolkit\Modules\Filtering\ImageLabel\include \
	$$InsightToolkit\Modules\Filtering\ImageIntensity\include \
	$$InsightToolkit\Modules\Filtering\Path\include \
	$$InsightToolkit\Modules\Filtering\ImageStatistics\include \
	$$InsightToolkit\Modules\Core\SpatialObjects\include \
	$$InsightToolkit\Modules\Core\Mesh\include \
	$$InsightToolkit\Modules\Filtering\ImageCompose\include \
	$$InsightToolkit\Modules\Core\TestKernel\include \
	$$InsightToolkit\Modules\IO\VTK\include \
	$$InsightToolkit\Modules\IO\Stimulate\include \
	$$InsightToolkit\Modules\IO\PNG\include \
	$$InsightToolkit\Modules\ThirdParty\PNG\src \
	$$InsightToolkit\Modules\IO\NRRD\include \
	$$InsightToolkit\Modules\ThirdParty\NrrdIO\src\NrrdIO \
	$$InsightToolkit\Modules\IO\NIFTI\include \
	$$InsightToolkit\Modules\ThirdParty\NIFTI\src\nifti\znzlib \
	$$InsightToolkit\Modules\ThirdParty\NIFTI\src\nifti\niftilib \
	$$InsightToolkit\Modules\IO\Meta\include \
	$$InsightToolkit\Modules\ThirdParty\MetaIO\src\MetaIO \
	$$InsightToolkit\Modules\IO\LSM\include \
	$$InsightToolkit\Modules\IO\TIFF\include \
	$$InsightToolkit\Modules\ThirdParty\TIFF\src \
	$$InsightToolkit\Modules\IO\GIPL\include \
	$$InsightToolkit\Modules\IO\GDCM\include \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Utilities\C99 \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\DataStructureAndEncodingDefinition \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\MessageExchangeDefinition \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\InformationObjectDefinition \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\Common \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\DataDictionary \
	$$InsightToolkit\Modules\ThirdParty\GDCM\src\gdcm\Source\MediaStorageAndFileFormat \
	$$InsightToolkit\Modules\IO\JPEG\include \
	$$InsightToolkit\Modules\ThirdParty\JPEG\src \
	$$InsightToolkit\Modules\ThirdParty\ZLIB\src \
	$$InsightToolkit\Modules\ThirdParty\OpenJPEG\src\openjpeg \
	$$InsightToolkit\Modules\ThirdParty\Expat\src\expat \
	$$InsightToolkit\Modules\IO\BioRad\include \
	$$InsightToolkit\Modules\IO\BMP\include \
	$$InsightToolkit\Modules\IO\ImageBase\include \
	$$InsightToolkit\Modules\Filtering\AnisotropicSmoothing\include \
	$$InsightToolkit\Modules\Filtering\ImageGrid\include \
	$$InsightToolkit\Modules\Core\ImageFunction\include \
	$$InsightToolkit\Modules\Core\Transform\include \
	$$InsightToolkit\Modules\Numerics\Statistics\include \
	$$InsightToolkit\Modules\Core\ImageAdaptors\include \
	$$InsightToolkit\Modules\Filtering\CurvatureFlow\include \
	$$InsightToolkit\Modules\Filtering\ImageFilterBase\include \
	$$InsightToolkit\Modules\Core\FiniteDifference\include \
	$$InsightToolkit\Modules\Core\Common\include \
	$$InsightToolkit\Modules\ThirdParty\VNLInstantiation\include \
	$$InsightToolkit\Modules\ThirdParty\VNL\src\vxl\core \
	$$InsightToolkit\Modules\ThirdParty\VNL\src\vxl\vcl \
	$$InsightToolkit\Modules\ThirdParty\VNL\src\vxl\v3p\netlib \
	$$ITK\Examples\ITKIOFactoryRegistration \
	$$ITK\Modules\ThirdParty\TIFF\src\itktiff \
	$$ITK\Modules\ThirdParty\TIFF\src \
	$$ITK\Modules\ThirdParty\HDF5\src \
	$$ITK\Modules\ThirdParty\DICOMParser\src\DICOMParser \
	$$ITK\Modules\ThirdParty\PNG\src \
	$$ITK\Modules\ThirdParty\NrrdIO\src\NrrdIO \
	$$ITK\Modules\ThirdParty\MetaIO\src\MetaIO \
	$$ITK\Modules\ThirdParty\JPEG\src \
	$$ITK\Modules\ThirdParty\GDCM\src\gdcm\Source\Common \
	$$ITK\Modules\ThirdParty\GDCM \
	$$ITK\Modules\ThirdParty\ZLIB\src \
	$$ITK\Modules\ThirdParty\OpenJPEG\src\openjpeg \
	$$ITK\Modules\ThirdParty\Expat\src\expat \
	$$ITK\Modules\IO\ImageBase \
	$$ITK\Modules\ThirdParty\Netlib \
	$$ITK\Modules\ThirdParty\KWSys\src \
	$$ITK\Modules\ThirdParty\VNL\src\vxl\core \
	$$ITK\Modules\Core\Common \
	$$ITK\Modules\ThirdParty\VNL\src\vxl\vcl \
	$$ITK\Modules\ThirdParty\VNL\src\vxl\v3p\netlib

QMAKE_LIBDIR += ..\..\common \
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

DESTDIR = ../../../../bin/renderplugins/ITK/Smoothing

INCLUDEPATH += ../../../ \
	/usr/local/include \
	$$InsightToolkit/Modules/Core/Common/include \
	$$InsightToolkit/Modules/Core/FiniteDifference/include \
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

  INCLUDEPATH += ../../../ \
        ../../../../../Library/Frameworks/QGLViewer.framework/Headers \
	/usr/local/include \
	$$InsightToolkit/Modules/Core/Common/include \
	$$InsightToolkit/Modules/Core/FiniteDifference/include \
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
	$$ITK/Modules/Core/Common \
	$$ITK/Modules/IO/ImageBase \
	$$ITK/Modules/ThirdParty/GDCM \
	$$ITK/Modules/ThirdParty/GDCM/src/gdcm/Source/Common \
	$$ITK/Modules/ThirdParty/KWSys/src \
	$$ITK/Modules/ThirdParty/VNL/src/vxl/core \
	$$ITK/Modules/ThirdParty/VNL/src/vxl/vcl \
	$$ITK/Modules/ThirdParty/MetaIO/src/MetaIO \
	$$ITK/Modules/ThirdParty/ZLIB/src


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


HEADERS = smoothing.h \
	filter.h	

SOURCES = smoothing.cpp \
	filter.cpp
