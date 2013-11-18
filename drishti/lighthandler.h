#ifndef LIGHTHANDLER_H
#define LIGHTHANDLER_H

#include <GL/glew.h>
#include <QtGui>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QGLWidget>
#include <QGLFramebufferObject>

#include "gilights.h"
#include "gilightinfo.h"
#include "geometryobjects.h"

class LightHandler
{
 public :
    static void init();
    static void reset();
    static void clean();

    static bool basicLight() { return m_basicLight; }

    static bool openPropertyEditor();

    static void updateOpacityTexture(GLuint,
				     int, int,
				     Vec,
				     Vec, Vec, Vec,
				     uchar*);
    static void updateAndLoadLightTexture(GLuint,
					  int, int,
					  Vec,
					  Vec, Vec, Vec,
					  uchar*);

    static void setLut(uchar*);

    static void createOpacityShader(bool);
    static void createLightShaders();

    static GLuint texture();

    static void lightBufferInfo(int&, int&, int&,
				int&, int&, int&);

    static GiLightInfo giLightInfo();
    static void setGiLightInfo(GiLightInfo);

    static void setPointLightParameters(float, float, float);

    static GiLights* giLights();

    static bool grabsMouse();
    static void removeFromMouseGrabberPool();
    static void addInMouseGrabberPool();    
    static bool keyPressEvent(QKeyEvent*);
    static void mouseReleaseEvent(QMouseEvent*, Camera*);

    static bool updateOnlyLightBuffers() { return (m_onlyLightBuffers); }
    static bool willUpdateLightBuffers() { return (m_doAll || m_onlyLightBuffers); }
    static bool lightsChanged();
    static void updateLightBuffers();
    
    static bool inPool;
    static bool showLights;

    static void show();
    static void hide();

    static void setClips(QList<Vec>, QList<Vec>);
    static bool checkClips(QList<Vec>, QList<Vec>);
    static bool checkCrops();

 private :
    static bool m_initialized;
    static GLuint m_dataTex;

    static bool m_doAll, m_onlyLightBuffers;
    static bool m_lutChanged;
    static uchar *m_lut;

    static bool m_showLights;
    static GiLights* m_giLights;

    static QGLFramebufferObject *m_opacityBuffer;
    static QGLFramebufferObject *m_finalLightBuffer;
    static QGLFramebufferObject *m_pruneBuffer;

    static bool m_basicLight;
    static bool m_onlyAOLight;
    static int m_lightLod;
    static int m_lightDiffuse;

    static Vec m_aoLightColor;
    static int m_aoRad, m_aoTimes;
    static float m_aoFrac, m_aoDensity1, m_aoDensity2;

    static float m_emisDecay;
    static float m_emisBoost;
    static int m_emisTimes;

    static bool m_applyClip, m_applyCrop;

    static int m_dtexX, m_dtexY;
    static Vec m_dragInfo, m_subVolSize;
    static Vec m_dataMin, m_dataMax;
    static int m_opacityTF, m_emisTF;

    static GLuint m_lutTex;

    static GLhandleARB m_opacityShader;
    static GLhandleARB m_mergeOpPruneShader;

    static GLhandleARB m_initdLightShader;
    static GLhandleARB m_initpLightShader;
    static GLhandleARB m_initEmisShader;

    static GLhandleARB m_aoLightShader;
    static GLhandleARB m_dLightShader;
    static GLhandleARB m_pLightShader;
    static GLhandleARB m_fLightShader;
    static GLhandleARB m_efLightShader;
    static GLhandleARB m_diffuseLightShader;
    static GLhandleARB m_emisShader;

    static GLhandleARB m_expandLightShader;

    static GLint m_opacityParm[20];
    static GLint m_mergeOpPruneParm[20];

    static GLint m_emisParm[20];
    static GLint m_initEmisParm[20];

    static GLint m_initdLightParm[20];
    static GLint m_initpLightParm[20];

    static GLint m_aoLightParm[20];
    static GLint m_dLightParm[20];
    static GLint m_pLightParm[20];
    static GLint m_fLightParm[20];
    static GLint m_efLightParm[20];
    static GLint m_diffuseLightParm[20];

    static GLint m_expandLightParm[20];

    static GLuint m_lightBuffer;
    static GLuint m_lightTex[2];

    static int m_dilatedEmisTex;
    static int m_origEmisTex;
    static GLuint m_emisBuffer;
    static GLuint m_emisTex[2];

    static int m_nrows, m_ncols, m_gridx, m_gridy, m_gridz;

    static QGLFramebufferObject* newFBO(int, int);

    static QList<Vec> m_clipPos;
    static QList<Vec> m_clipNorm;
    static GLhandleARB m_clipShader;
    static GLint m_clipParm[20];

    static QList<CropObject> m_crops;
    static GLhandleARB m_cropShader;
    static GLint m_cropParm[20];

    static void generateOpacityTexture();

    static void generateEmissiveTexture();
    static void dilateEmissiveTexture();

    static bool standardChecks();
    static void genBuffers();

    static void createAmbientOcclusionLightShader();
    static void createDirectionalLightShader();
    static void createPointLightShader();
    static void createFinalLightShader();
    static void createDiffuseLightShader();
    static void createEmissiveShader();
    static void createExpandShader();
    static void createMergeOpPruneShader();
    static void createClipShader();
    static void createCropShader();

    static void updatePruneBuffer();

    static void updateAmbientOcclusionLightBuffer(int, float,
						  float, float,
						  int, Vec);
    static void updatePointLightBuffer(QList<Vec>, float,
				       float, float,
				       Vec, int, int,
				       bool);
    static void updateDirectionalLightBuffer(Vec, float, Vec,
					     int, int);

    static void updateEmissiveBuffer(float);

    static void updateFinalLightBuffer(int, Vec);
    static void updateFinalLightBuffer(int);

    static void diffuseLightBuffer(int);

    static int lightBufferCalculations(int, int lct = -1, int lX=0, int lY=0);


    static void mergeOpPruneBuffers(int);

    static int applyClipping(int);
    static int applyCropping(int);
};

#endif
