#include "global.h"
#include "profileviewer.h"

#include <QGraphicsTextItem>
#include <QFileDialog>

ProfileViewer::ProfileViewer(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  m_minV = m_maxV = 1.0;
  m_scene = new QGraphicsScene();

  m_graphValues.clear();
  m_graphValMin.clear();
  m_graphValMax.clear();

  m_width = 500;
  m_height = 200;

  m_scene->addRect(0, 0, m_width, m_height);

  m_captionString = "Caption";
  m_captionTextItem = m_scene->addText(m_captionString);
  m_captionTextItem->setPos(0, m_height); // bottom

  ui.graphicsView->setScene(m_scene);
  ui.graphicsView->setRenderHints(QPainter::Antialiasing |
				  QPainter::TextAntialiasing |
				  QPainter::HighQualityAntialiasing |
				  QPainter::SmoothPixmapTransform);
}
void
ProfileViewer::on_caption_editingFinished()
{
  QFont fnt("Helvetica", 12);
  QFontMetrics fmt(fnt);

  m_captionString = ui.caption->text();
  m_captionTextItem->setFont(fnt);
  m_captionTextItem->setPlainText(m_captionString);
  int tw = fmt.width(m_captionString);
  m_captionTextItem->setPos(50 + (m_width-tw)/2, -10-fmt.descent()); // bottom
}

void
ProfileViewer::setValueRange(float vmin, float vmax)
{
  m_minV = vmin;
  m_maxV = vmax;
}
void
ProfileViewer::setGraphValues(float vmin, float vmax,
			      QList<uint> index,
			      QList<float> values)
{
  m_minV = vmin;
  m_maxV = vmax;
  m_index = index;
  m_graphValues = values;
  m_graphValMin.clear();
  m_graphValMax.clear();
}
void
ProfileViewer::setGraphValues(float vmin, float vmax,
			      QList<uint> index,
			      QList<float> values,
			      QList<float> valMin,
			      QList<float> valMax)
{
  m_minV = vmin;
  m_maxV = vmax;
  m_index = index;
  m_graphValues = values;
  m_graphValMin = valMin;
  m_graphValMax = valMax;
}

#define REMAP(v) ((v-m_minV)/(m_maxV-m_minV))
void
ProfileViewer::generateScene()
{
  int sw = 50;
  int sh = 10;

  m_height = ui.graphicsView->height()-51;
  m_width = ui.graphicsView->width()-101;

  delete m_scene;
  m_scene = new QGraphicsScene();

//  m_scene->setBackgroundBrush(QBrush(Qt::lightGray,
//				     Qt::CrossPattern));

  m_captionTextItem = m_scene->addText(m_captionString);
  on_caption_editingFinished(); // update Caption

  m_scene->addRect(sw, sh, m_width, m_height);
  QGraphicsTextItem *txt;

  m_scene->addLine(sw, sh+m_height, 45, sh+m_height);
  QString stxt = QString("%1").arg(m_minV);
  txt = m_scene->addText(stxt);
  QFontMetrics fmt(txt->font());
  int tw = fmt.width(stxt);
  int th = fmt.height()/2;
  txt->setPos(40-tw, sh+m_height-th-fmt.descent()); // bottom

  m_scene->addLine(50, sh, 45, sh);
  stxt = QString("%1").arg(m_maxV);
  txt = m_scene->addText(stxt);
  tw = fmt.width(stxt);
  txt->setPos(40-tw,sh-th); // top

  // -- horizontal grid lines
  int nh=m_height/50;
  if (nh > 1)
    {
      for(int i=1; i<nh-1; i++)
	{
	  float frc = (float)i/(float)(nh-1);
	  float ht = sh+m_height*frc;
	  m_scene->addLine(45, ht, 50+m_width, ht,
			   QPen(QColor(0,200,0,128)));

	  float v = m_maxV*(1-frc) + m_minV*frc;
	  stxt = QString("%1").arg(v);
	  txt = m_scene->addText(stxt);
	  tw = fmt.width(stxt);
	  txt->setPos(40-tw, ht-th); // bottom
	}
    }

  float endpt = m_index[m_index.size()-1];
  for(int id=0; id<m_index.size()-1; id++)
    {
      float frcw = (float)m_index[id]/endpt;
      float xpos = 50 + m_width*frcw;
      m_scene->addLine(xpos, sh+m_height,
		       xpos, sh+m_height+5);

      stxt = QString("%1").arg(id);
      txt = m_scene->addText(stxt);
      tw = fmt.width(stxt)/2;
      txt->setPos(xpos-tw, sh+m_height+5);

      // -- vertical grid lines
      m_scene->addLine(xpos, sh, xpos, sh+m_height,
		       QPen(QColor(0, 255, 0, 200)));
    }

  if (m_graphValues.size() > 1)
    {
      QPainterPath pp;
      pp.moveTo(50,
		sh+m_height*(1-REMAP(m_graphValues[0])));
      int npts = m_graphValues.size(); 
      for(int i=1; i<npts; i++)
	{
	  float frcw = (float)i/(float)(npts-1);
	  float frch = REMAP(m_graphValues[i]);
	  pp.lineTo(50 + m_width*frcw,
		    sh+m_height*(1-frch));      
	}
      m_scene->addPath(pp);
    }

  if (m_graphValMin.size() > 1)
    {
      QPainterPath pp;
      pp.moveTo(50,
		sh+m_height*(1-REMAP(m_graphValMin[0])));
      int npts = m_graphValMin.size(); 
      for(int i=1; i<npts; i++)
	{
	  float frcw = (float)i/(float)(npts-1);
	  float frch = REMAP(m_graphValMin[i]);
	  pp.lineTo(50 + m_width*frcw,
		    sh+m_height*(1-frch));      
	}

      m_scene->addPath(pp, QPen(QColor(0, 0, 200, 128)));
    }

  if (m_graphValMax.size() > 1)
    {
      QPainterPath pp;
      pp.moveTo(50,
		sh+m_height*(1-REMAP(m_graphValMax[0])));
      int npts = m_graphValMax.size(); 
      for(int i=1; i<npts; i++)
	{
	  float frcw = (float)i/(float)(npts-1);
	  float frch = REMAP(m_graphValMax[i]);
	  pp.lineTo(50 + m_width*frcw,
		    sh+m_height*(1-frch));      
	}

      m_scene->addPath(pp, QPen(QColor(200, 0, 0, 128)));
    }

  ui.graphicsView->setScene(m_scene);
}

void
ProfileViewer::on_saveImage_clicked()
{
  QPixmap pix = QPixmap::grabWidget(ui.graphicsView);
  
  QString imgFile = QFileDialog::getSaveFileName(0,
			     "Save profile image to",
			     Global::previousDirectory()+"/profile.png",
			     "Image Files (*.png *.tif *.bmp *.jpg)");

  if (imgFile.isEmpty())
    return;

  QFileInfo f(imgFile);
  Global::setPreviousDirectory(f.absolutePath());	      
  pix.save(imgFile);
}

void
ProfileViewer::resizeEvent(QResizeEvent *event)
{
  generateScene();
}
