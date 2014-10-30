#ifndef PRUNEHANDLER_H
#define PRUNEHANDLER_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QGLWidget>
#include <QGLFramebufferObject>

class PruneHandler
{
  public :
    enum MorphologicalOperation
      {
	NoMorphOp = 0,
	Dilate,
	Erode,
	Open,
	Close,
	ShrinkWrap,
	Invert,
	Thicken,
	CityBlock,
	CopyChannel	
      };

    enum PruneShaderType
      {
	NoPruneShader = 0,
	SetValueShader,
	RemovePatchShader,
	CopyShader,
	InvertShader,
	DilateShader,
	RestrictedDilateShader,
	ErodeShader,
	ShrinkShader,
	ThickenShader,
	EdgeShader,
	CarveShader,
	PaintShader,
	DilateEdgeShader,
	LocalMaxShader,
	LocalThicknessShader,
	CopyChannelShader,
	TriangleShader,
	SmoothChannelShader,
	AverageShader,
	ClipShader,
	CropShader,
	PatternShader
      };

    static void clean();

    static void updateAndLoadPruneTexture(GLuint,
					  int, int,
					  Vec, Vec,
					  uchar*);

    static void createPruneShader(bool);
    static void createMopShaders();

    static GLuint texture();

    static void setUseSavedBuffer(bool);
    static bool useSavedBuffer();

    static void swapBuffer();

    static void saveBuffer();

    static void copyChannel(int, int, bool doPrint=true);
    static void copyToFromSavedChannel(bool,
				       int ch1=-1, int ch2=-1,
				       bool showmesg=true);

    static void smoothChannel(int);

    static void setChannel(int);
    static int channel();

    static void getRaw(uchar*, int, Vec, Vec, bool maskUsingRed = false);
    static void setRaw(uchar*, int, Vec, Vec);

    static void invert(int chan=-1);
    static void setValue(int, int, int, int);
    static void dilate(int, int);
    static void erode(int, int);
    static void shrink(int, int);
    static void open(int, int);
    static void close(int, int);
    static void thicken(int, bool cityBlock=true);
    static void shrinkwrap(int, int sz2 = 0);
    static void distanceTransform(int, bool cityBlock=true);
    static void minmax(bool, int ch1=-1, int ch2=-1);
    static void xorTexture(int ch1=-1, int ch2=-1);
    static void edge(int, int);
    static void localMaximum();
    static void dilateEdge(int, int);
    static void restrictedDilate(int, int);
    static void average(int, int, int);

    static QByteArray getPruneBuffer();
    static void setPruneBuffer(QByteArray, bool compressed=true);


    static void setBlend(bool);
    static bool blend();

    static void setPaint(bool);
    static bool paint();
    static void setTag(int);
    static int tag();

    static void setCarve(bool);
    static bool carve();
    static void sculpt(int, Vec,
		       QList<Vec>,
		       float rad = -1.0f, float decay = -1.0f,
		       int tag=-1);
    static void setCarveRad(float, float decay = -10.0f);
    static void carveRad(float&, float&);
    static void setPlanarCarve(Vec cp=Vec(0,0,0),
			       Vec cn=Vec(0,0,0),
			       Vec cx=Vec(0,0,0),
			       Vec cy=Vec(0,0,0),
			       float ct=1.0f,
			       Vec dmin=Vec(0,0,0));

    static void clip(Vec, Vec, Vec);
    static void crop(QString, Vec);

    static void fillPathPatch(Vec, QList<Vec>, int, int, bool);
    static void removePatch(bool);
    
    static QByteArray interpolate(QByteArray,QByteArray,float);

    static QList<int> getMaxValue();
    static QList<int> getHistogram(int chan=2);

    static void localThickness(int);

    static void pattern(bool, int, int, int, int, int, int);

  private :
    static bool m_mopActive;

    static GLuint m_dataTex;
    static uchar *m_lut;

    static bool m_blendActive;
    static bool m_paintActive;
    static int m_tag;

    static bool m_carveActive;
    static float m_carveRadius;
    static float m_carveDecay;
    static Vec m_carveP, m_carveN, m_carveX, m_carveY;
    static float m_carveT;

    static GLuint m_pruneTex;
    static QGLFramebufferObject *m_pruneBuffer;
    static QGLFramebufferObject *m_savedPruneBuffer;

    static int m_channel;
    static int m_dtexX, m_dtexY;
    static Vec m_dragInfo, m_subVolSize;

    static bool m_useSavedBuffer;

    static GLuint m_lutTex;

    static GLhandleARB m_pruneShader;
    static GLhandleARB m_dilateEdgeShader;
    static GLhandleARB m_dilateShader;
    static GLhandleARB m_erodeShader;
    static GLhandleARB m_shrinkShader;
    static GLhandleARB m_invertShader;
    static GLhandleARB m_thickenShader;
    static GLhandleARB m_edgeShader;
    static GLhandleARB m_copyChannelShader;
    static GLhandleARB m_setValueShader;
    static GLhandleARB m_minShader;
    static GLhandleARB m_maxShader;
    static GLhandleARB m_xorShader;
    static GLhandleARB m_localmaxShader;
    static GLhandleARB m_rDilateShader;
    static GLhandleARB m_carveShader;
    static GLhandleARB m_paintShader;
    static GLhandleARB m_triShader;
    static GLhandleARB m_removePatchShader;
    static GLhandleARB m_clipShader;
    static GLhandleARB m_maxValueShader;
    static GLhandleARB m_histogramShader;
    static GLhandleARB m_localThicknessShader;
    static GLhandleARB m_smoothChannelShader;
    static GLhandleARB m_averageShader;
    static GLhandleARB m_patternShader;
    static GLhandleARB m_cropShader;

    static GLint m_pruneParm[20];
    static GLint m_dilateEdgeParm[20];
    static GLint m_dilateParm[20];
    static GLint m_erodeParm[20];
    static GLint m_shrinkParm[20];
    static GLint m_invertParm[20];
    static GLint m_thickenParm[20];
    static GLint m_edgeParm[20];
    static GLint m_copyChannelParm[5];
    static GLint m_setValueParm[5];
    static GLint m_minParm[5];
    static GLint m_maxParm[5];
    static GLint m_xorParm[5];
    static GLint m_localmaxParm[20];
    static GLint m_rDilateParm[20];
    static GLint m_carveParm[20];
    static GLint m_paintParm[20];
    static GLint m_triParm[20];
    static GLint m_removePatchParm[5];
    static GLint m_clipParm[20];
    static GLint m_maxValueParm[5];
    static GLint m_histogramParm[20];
    static GLint m_localThicknessParm[20];
    static GLint m_smoothChannelParm[20];
    static GLint m_averageParm[5];
    static GLint m_patternParm[20];
    static GLint m_cropParm[20];


   static QGLFramebufferObject* newFBO();

    static void generatePruneTexture(QGLFramebufferObject*, uchar*);
    static void modifyPruneTexture(int,
				   QGLFramebufferObject*,
				   QGLFramebufferObject*,
				   QVariantList vlist=QVariantList());
     
    static void copyChannelTexture(int, int,
				   QGLFramebufferObject*,
				   QGLFramebufferObject*);

    static void copyBuffer(bool);

    static void saveImage(QString);
    static void saveRaw(QString);

    static bool standardChecks();
    static void genBuffer(int, int);

};

#endif
