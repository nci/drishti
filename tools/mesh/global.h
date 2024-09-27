#ifndef GLOBAL_H
#define GLOBAL_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "commonqtclasses.h"
#include <QStatusBar>
#include <QProgressBar>
#include <QProgressDialog>
#include <QMessageBox>

class Global
{
 public :

  enum VisualizationModeType
    {
     ImportMode = 0,
     Surfaces,
     Volumes
    };
  
  enum SubvolumeFilter
    {
      _NoFilter = 0,
      _ConservativeFilter,
      _MeanFilter,
      _MedianFilter
    };

  enum ImageQuality
    {
      _NormalQuality,
      _LowQuality,
      _VeryLowQuality
    };

  enum ViewerFilterType
    {
      _GaussianFilter,
      _SharpnessFilter
    };

    enum VoxelUnit {
    Nounit = 0,
    Micron,
    Millimeter,
    Centimeter,
    Meter
  };


  static QString DrishtiVersion();

  static void setPrayogMode(bool);
  static bool prayogMode();
  
  static void setHelpLanguage(QString);
  static QString helpLanguage();
  
  static void setBatchMode(bool);
  static bool batchMode();

  static void checkGLError(QString, bool clearErrors=false);

  static int viewerFilter();
  static void setViewerFilter(int);

  static bool updateViewer();
  static void enableViewerUpdate();
  static void disableViewerUpdate();

  static bool emptySpaceSkip();
  static void setEmptySpaceSkip(bool);

  static int imageQuality();
  static void setImageQuality(int);

  static bool playFrames();
  static void setPlayFrames(bool);

  static bool bottomText();
  static void setBottomText(bool);

  static bool depthcue();
  static void setDepthcue(bool);


  static bool flipImage();
  static bool flipImageX();
  static bool flipImageY();
  static bool flipImageZ();
  static void setFlipImageX(bool);
  static void setFlipImageY(bool);
  static void setFlipImageZ(bool);

  static void setTexSizeReduceFraction(float); 
  static float texSizeReduceFraction();
  static GLint max2dTextureSize();
  static int textureSize();
  static void setTextureSize(int);

  static int textureMemorySize();
  static void setTextureMemorySize(int);

  static void calculate3dTextureSize();

  static int filteredData();
  static void setFilteredData(int);


  static Vec voxelScaling(); // -- m_voxelScaling * m_relativeVoxelScaling  
  static Vec relativeVoxelScaling();
  static void setRelativeVoxelScaling(Vec);

  static QString previousDirectory();
  static void setPreviousDirectory(QString);

  static bool useFBO();
  static void setUseFBO(bool);

  static void setBounds(Vec, Vec);
  static void bounds(Vec&, Vec&);

  static void setVolumeNumber(int, int vol=0);
  static int volumeNumber(int vol=0);

  static void setActualVolumeNumber(int, int vol=0);
  static int actualVolumeNumber(int vol=0);

  static void setFrameNumber(int);
  static int frameNumber();

  static void setVolumeType(int);
  static int volumeType();
  enum VolumeType {
    SingleVolume,
    DoubleVolume,
    TripleVolume,
    QuadVolume,
    RGBVolume,
    RGBAVolume,
    DummyVolume
  };

  static void setDrawBox(bool);
  static bool drawBox();

  static void setDrawAxis(bool);
  static bool drawAxis();

  static void setBackgroundColor(Vec);
  static Vec backgroundColor();

  static void resetBackgroundImageFile();
  static void setBackgroundImageFile(QString, QString);
  static QString backgroundImageFile();
  static QImage backgroundImage();
  static GLuint backgroundTexture();
  static void removeBackgroundTexture();

  static void setStepsizeStill(float);
  static float stepsizeStill();

  static void setStepsizeDrag(float);
  static float stepsizeDrag();

  static void setSaveImageType(int);
  static int saveImageType();
  enum SaveImageType {
    NoImage,
    MonoImage,
    LeftImageAnaglyph,
    RightImageAnaglyph,
    LeftImage,
    RightImage,
    CubicFrontImage,
    CubicRightImage,
    CubicBackImage,
    CubicLeftImage,
    CubicTopImage,
    CubicBottomImage
  };

  enum InterpolationType {
    TextureInterpolation = 0,
    CameraPositionInterpolation
  };

  static void setReplaceTF(bool);
  static bool replaceTF();

  static void setMorphTF(bool);
  static bool morphTF();

  static void setUseMask(bool);
  static bool useMask();

  static void setTagColors(uchar*);
  static uchar* tagColors();

  static void setTextureSizeLimit(int);
  static int textureSizeLimit();

  static void setInterpolationType(int);
  static int interpolationType();

  static void setInterpolationType(int, bool);
  static bool interpolationType(int);

  static int maxRecentFiles();
  static QStringList recentFiles();
  static void setRecentFiles(QStringList);
  static void addRecentFile(QString);
  static QString recentFile(int);

  static void setCurrentProjectFile(QString);
  static void resetCurrentProjectFile();
  static QString currentProjectFile();

  static void setDoodleMask(uchar*, int);
  static uchar* doodleMask();

  static GLuint spriteTexture();
  static void removeSpriteTexture();

  static GLuint lightTexture();
  static void removeLightTexture();

  static GLuint hollowSpriteTexture();
  static void removeHollowSpriteTexture();

  static GLuint infoSpriteTexture();
  static void removeInfoSpriteTexture();

  static GLuint boxTexture();
  static void removeBoxTexture();

  static GLuint sphereTexture();
  static void removeSphereTexture();

  static GLuint cylinderTexture();
  static void removeCylinderTexture();



  static void setStatusBar(QStatusBar*, QAction*);
  static QStatusBar* statusBar();

  static QProgressBar* progressBar(bool forceShow=true);

  static QString tempDir();
  static void setTempDir(QString);

  static int floatPrecision();
  static void setFloatPrecision(int);

  static void setGeoRenderSteps(int);
  static int geoRenderSteps();

  static void setUpdatePruneTexture(bool);
  static bool updatePruneTexture();


  static void setCopyShader(GLhandleARB); 
  static GLhandleARB copyShader(); 
  static void setCopyParm(GLint*, int);
  static GLint copyParm(int);
  static GLint* copyParm();

  static void setReduceShader(GLhandleARB); 
  static GLhandleARB reduceShader(); 
  static void setReduceParm(GLint*, int);
  static GLint reduceParm(int);
  static GLint* reduceParm();

  static void setExtractSliceShader(GLhandleARB); 
  static GLhandleARB extractSliceShader(); 
  static void setExtractSliceParm(GLint*, int);
  static GLint extractSliceParm(int);
  static GLint* extractSliceParm();

  static int dpi();

  static void setVoxelUnit(int);
  static int voxelUnit();
  static QString voxelUnitString();
  static QString voxelUnitStringShort();  

  static void setVoxelSize(Vec);
  static Vec voxelSize();
  
  static void setRelDataPos(Vec);
  static Vec relDataPos();

  static void setGamma(float);
  static float gamma();

  static void setHideBlack(bool);
  static bool hideBlack();

  static void setShadowBox(bool);
  static bool shadowBox();

  static int visualizationMode();
  static void setVisualizationMode(int);


  static void setBytesPerVoxel(int);
  static int bytesPerVoxel();


private :
  static int m_visualizationMode;

  static bool m_prayogMode;
  
  static QString m_helpLanguage;
  
  static float m_gamma;
  static QString m_documentationPath;
  static bool m_useFBO;
  static bool m_drawBox;
  static bool m_drawAxis;
  static float m_texSizeReduceFraction;
  static int m_textureSize;
  static int m_textureMemorySize;
  static int m_textureSizeLimit;
  static int m_filteredData;
  static Vec m_relativeVoxelScaling;
  static QString m_previousDirectory;
  static Vec m_dataMin, m_dataMax;
  static Vec m_bgColor;
  static QString m_bgImageFile;
  static QImage m_bgImage;
  static GLuint m_bgTexture;
  static int m_saveImageType;

  static int m_volumeType;

  static int m_frameNumber;

  static bool m_use1D;

  static uchar *m_tagColors;

  static bool m_interpolationType[10];

  static int m_maxRecentFiles;
  static QStringList m_recentFiles;

  static QString m_currentProjectFile;

  static bool m_opacityNormal;

  static uchar *m_doodleMask;

  static GLuint m_spriteTexture;
  static GLuint m_lightTexture;
  static GLuint m_hollowSpriteTexture;
  static GLuint m_infoSpriteTexture;
  static GLuint m_boxTexture;
  static GLuint m_sphereTexture;
  static GLuint m_cylinderTexture;

  static bool m_flipImageX;
  static bool m_flipImageY;
  static bool m_flipImageZ;

  static bool m_bottomText;
  static bool m_depthcue;

  static bool m_playFrames;
  static int m_imageQuality;

  static bool m_updateViewer;
  static int m_viewerFilter;

  static bool m_showGeo;

  static QProgressBar *m_progressBar;
  static QStatusBar *m_statusBar;
  static QAction *m_actionStatusBar;

  static bool m_batchMode;

  static QString m_tempDir;

  static int m_floatPrecision;
  static int m_geoRenderSteps;


  static GLhandleARB m_copyShader;
  static GLint m_copyParm[5];

  static GLhandleARB m_reduceShader;
  static GLint m_reduceParm[5];

  static GLhandleARB m_extractSliceShader;
  static GLint m_extractSliceParm[5];

  static int m_dpi;

  static Vec m_relDataPos;

  static bool m_hideBlack;

  static bool m_shadowBox;

  static int m_bytesPerVoxel;

  static int m_voxelUnit;
  static Vec m_voxelSize;

  static QStringList m_voxelUnitStrings;
  static QStringList m_voxelUnitStringsShort;
};

#endif
