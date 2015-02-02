#include "global.h"
#include "landmarks.h"
#include "staticfunctions.h"
#include "volumeinformation.h"
#include "dcolordialog.h"

#include <fstream>
using namespace std;

#include <QMessageBox>
#include <QTextStream>

#include <QGroupBox>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QLabel>

int Landmarks::count() { return m_points.count(); }

void Landmarks::setPointSize(int sz)
{
  m_pointSize = sz;
  m_pspinB->setValue(m_pointSize);
}
int Landmarks::pointSize() { return m_pointSize; }

void Landmarks::setTextSize(int sz)
{
  m_textSize = sz;
  m_tspinB->setValue(m_textSize);
}
int Landmarks::textSize() { return m_textSize; }

void Landmarks::setPointColor(Vec col) { m_pointColor = col; }
Vec Landmarks::pointColor() { return m_pointColor; }

void Landmarks::setTextColor(Vec col) { m_textColor = col; }
Vec Landmarks::textColor() { return m_textColor; }

int Landmarks::moveAxis() { return m_moveAxis; }
void Landmarks::setMoveAxis(int type) { m_moveAxis = type; }

Vec
Landmarks::point(int i)
{
  if (i >= 0 && i < m_points.count())
    return m_points[i];
  else
    return Vec(-1,-1,-1);
}

void
Landmarks::setPoint(int i, Vec p)
{
  if (i >= 0 && i < m_points.count())
    m_points[i] = p;
}

QList<Vec> Landmarks::points() { return m_points; }

void
Landmarks::setPoints(QList<Vec> p)
{
  clear();
  m_points.clear();
  m_text.clear();
  addPoints(p);

  addInMouseGrabberPool();
}

void
Landmarks::addPoints(QList<Vec> p)
{
  removeFromMouseGrabberPool();

  m_points += p;
  int tc = m_text.count();
  for(int i=tc; i<m_points.count(); i++)
    m_text << "";

  addInMouseGrabberPool();

  updateTable();
}

void
Landmarks::clear()
{
  m_table->hide();

  m_points.clear();
  m_text.clear();

  m_showCoordinates = false;
  m_showText = true;
  m_showNumber = true;

  m_pointColor = Vec(0.5,1.0,0.5);
  m_pointSize = 10;

  m_textColor = Vec(1.0,1.0,1.0);
  m_textSize = 10;

  m_grab = false;
  m_grabbed = -1;
  m_pressed = -1;
  m_moveAxis = MoveAll;

  m_distances.clear();
  m_angles.clear();

  m_projectLineDistances.clear();
  m_projectLinePoints.clear();

  m_projectPlanePoints.clear();

  removeFromMouseGrabberPool();
}

Landmarks::Landmarks()
{
  m_table = new QWidget();
  m_table->setWindowTitle("Landmarks");

  m_tableWidget = new QTableWidget();

  //---------------
  QGroupBox *bG1 = new QGroupBox();
  QHBoxLayout *hbox1 = new QHBoxLayout();

  QPushButton *reorderB = new QPushButton("Reorder");
  QPushButton *saveB = new QPushButton("Save");
  QPushButton *loadB = new QPushButton("Load");
  QPushButton *clearB = new QPushButton("DeleteAll");

  hbox1->addWidget(reorderB);
  hbox1->addWidget(saveB);
  hbox1->addWidget(loadB);
  hbox1->addWidget(clearB);
  hbox1->addStretch(4);
  bG1->setLayout(hbox1);
  //---------------


  //---------------
  QGroupBox *bG2 = new QGroupBox();
  QPushButton *pcolorB = new QPushButton("Color");
  m_pspinB = new QSpinBox();
  {
    QHBoxLayout *hbox2 = new QHBoxLayout();
    QLabel *lbl1 = new QLabel("Point ");
    QLabel *lbl2 = new QLabel("Size ");
    m_pspinB->setRange(5, 20);
    
    hbox2->addWidget(lbl1);
    hbox2->addWidget(pcolorB);
    hbox2->addSpacing(20);
    hbox2->addWidget(lbl2);
    hbox2->addWidget(m_pspinB);
    hbox2->addStretch();
    bG2->setLayout(hbox2);
  }
  //---------------


  //---------------
  QGroupBox *bG3 = new QGroupBox();
  QPushButton *tcolorB = new QPushButton("Color");
  m_tspinB = new QSpinBox();
  {
    QHBoxLayout *hbox2 = new QHBoxLayout();
    QLabel *lbl1 = new QLabel("Text");
    QLabel *lbl2 = new QLabel("Size ");
    m_tspinB->setRange(8, 40);
    
    hbox2->addWidget(lbl1);
    hbox2->addWidget(tcolorB);
    hbox2->addSpacing(20);
    hbox2->addWidget(lbl2);
    hbox2->addWidget(m_tspinB);
    hbox2->addStretch();
    bG3->setLayout(hbox2);
  }
  //---------------

  //---------------
  QGroupBox *bG4 = new QGroupBox();
  m_distEdit = new QLineEdit();
  m_angleEdit = new QLineEdit();
  m_projOnLineEdit = new QLineEdit();
  m_projOnPlaneEdit = new QLineEdit();
  {
    QGridLayout *gbox = new QGridLayout();
    QLabel *lbl1 = new QLabel("Distances ");
//    QLabel *lbl2 = new QLabel("Angles ");
    QLabel *lbl3 = new QLabel("Line Projection");
    QLabel *lbl4 = new QLabel("Plane Projection");
    
    gbox->addWidget(lbl1, 0, 0);
    gbox->addWidget(m_distEdit, 0, 1);
//    gbox->addWidget(lbl2, 1, 0);
//    gbox->addWidget(m_angleEdit, 1, 1);
    gbox->addWidget(lbl3, 1, 0);
    gbox->addWidget(m_projOnLineEdit, 1, 1);
    gbox->addWidget(lbl4, 2, 0);
    gbox->addWidget(m_projOnPlaneEdit, 2, 1);
    gbox->setColumnStretch(1, 1);
    bG4->setLayout(gbox);
  }
  //---------------


  //---------------
  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(bG1);
  vbox->addWidget(bG2);
  vbox->addWidget(bG3);
  vbox->addWidget(bG4);
  vbox->addWidget(m_tableWidget);

  m_table->setLayout(vbox);
  //---------------


  m_tableWidget->horizontalHeader()->setSectionsMovable(true);
  m_tableWidget->verticalHeader()->setSectionsMovable(true);
  //m_tableWidget->horizontalHeader()->setSectionsClickable(true);
  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  m_tableWidget->setShowGrid(true);
  m_tableWidget->setAlternatingRowColors(false);
  m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  m_tableWidget->setColumnCount(2);

  QStringList item;
  item.clear();
  item << "Name";
  item << "Coordinate";
  m_tableWidget->setHorizontalHeaderLabels(item);
  m_tableWidget->setColumnWidth(0, 50);
  m_tableWidget->setColumnWidth(1, 200);

  connect(reorderB, SIGNAL(clicked()), this, SLOT(reorderLandmarks()));
  connect(saveB, SIGNAL(clicked()), this, SLOT(saveLandmarks()));
  connect(loadB, SIGNAL(clicked()), this, SLOT(loadLandmarks()));
  connect(clearB, SIGNAL(clicked()), this, SLOT(clearAllLandmarks()));

  connect(pcolorB, SIGNAL(clicked()), this, SLOT(changePointColor()));
  connect(m_pspinB, SIGNAL(valueChanged(int)), this, SLOT(changePointSize(int)));

  connect(tcolorB, SIGNAL(clicked()), this, SLOT(changeTextColor()));
  connect(m_tspinB, SIGNAL(valueChanged(int)), this, SLOT(changeTextSize(int)));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(updateLandmarks(int, int)));

  connect(m_distEdit, SIGNAL(editingFinished()), this, SLOT(updateDistances()));
  connect(m_angleEdit, SIGNAL(editingFinished()), this, SLOT(updateAngles()));
  connect(m_projOnLineEdit, SIGNAL(editingFinished()), this, SLOT(updateProjectOnLine()));
  connect(m_projOnPlaneEdit, SIGNAL(editingFinished()), this, SLOT(updateProjectOnPlane()));

  clear();

  m_pspinB->setValue(m_pointSize);
  m_tspinB->setValue(m_textSize);
}

Landmarks::~Landmarks()
{
  m_table->close();

  clear();
}

void
Landmarks::setMouseGrab(bool g)
{ 
  m_grab = g;
  if (!m_grab)
    removeFromMouseGrabberPool();
  else
    addInMouseGrabberPool();
}
void Landmarks::toggleMouseGrab()
{
  m_grab = !m_grab;
  if (!m_grab)
    removeFromMouseGrabberPool();
  else
    addInMouseGrabberPool();
}

bool
Landmarks::grabsMouse()
{
  return (m_grabbed >= 0);
}

void
Landmarks::checkIfGrabsMouse(int x, int y,
			     const Camera* const camera)
{
  m_grabbed = -1;

  if (m_pressed >= 0)
    { // mouse button pressed so keep grabbing
      m_grabbed = m_pressed;
      setGrabsMouse(true);
      return;
    }

  Vec voxelScaling = Global::voxelScaling();
  for (int i=0; i<m_points.count(); i++)
    {
      Vec pos = VECPRODUCT(m_points[i], voxelScaling);
      pos = camera->projectedCoordinatesOf(pos);
      QPoint hp(pos.x, pos.y);
      if ((hp-QPoint(x,y)).manhattanLength() < 10)
	{
	  m_grabbed = i;
	  setGrabsMouse(true);
	  return;
	}
    }

    setGrabsMouse(false);
}

void
Landmarks::mousePressEvent(QMouseEvent* const event,
			   Camera* const camera)
{
  m_prevPos = event->pos();
  m_pressed = m_grabbed;
}

void
Landmarks::mouseMoveEvent(QMouseEvent* const event,
			  Camera* const camera)
{
  if (m_pressed < 0)
    return;
  
  QPoint delta = event->pos() - m_prevPos;
  Vec trans(delta.x(), -delta.y(), 0.0f);

  // Scale to fit the screen mouse displacement
  trans *= 2.0 * tan(camera->fieldOfView()/2.0) *
           fabs((camera->frame()->coordinatesOf(Vec(0,0,0))).z) /
           camera->screenHeight();
  // Transform to world coordinate system.
  trans = camera->frame()->orientation().rotate(trans);

  if (m_moveAxis == MoveX)
    trans = Vec(trans.x,0,0);
  else if (m_moveAxis == MoveY)
    trans = Vec(0,trans.y,0);
  else if (m_moveAxis == MoveZ)
    trans = Vec(0,0,trans.z);
  
  Vec voxelScaling = Global::voxelScaling();
  trans = VECDIVIDE(trans, voxelScaling);

  m_points[m_pressed] += trans;

  m_prevPos = event->pos();
}

void
Landmarks::mouseReleaseEvent(QMouseEvent* const event,
			   Camera* const camera)
{
  if (m_pressed >= 0)
    {
      Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;
      QTableWidgetItem *wiC;
      wiC = m_tableWidget->item(m_pressed, 1);
      wiC->setText(QString("%1 %2 %3").	\
		   arg(m_points[m_pressed].x*voxelSize.x). \
		   arg(m_points[m_pressed].y*voxelSize.y). \
		   arg(m_points[m_pressed].z*voxelSize.z));
    }

  m_pressed = -1;
}

bool
Landmarks::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Delete ||
      event->key() == Qt::Key_Backspace ||
      event->key() == Qt::Key_Backtab)
    {
      if (m_grabbed >=0 && m_grabbed < m_points.count())
	{
	  m_points.removeAt(m_grabbed);
	  m_text.removeAt(m_grabbed);
	  m_grabbed = -1;
	  m_pressed = -1;
	  updateTable();
	  return true;
	}
    }
  else if (event->key() == Qt::Key_Space)
    {
      updateTable();
      if (m_table->isVisible())
	m_table->hide();
      else
	m_table->show();
      return true;
    }
  else if (event->key() == Qt::Key_N)
    {
      m_showNumber = !m_showNumber;
      return true;
    }
  else if (event->key() == Qt::Key_T)
    {
      m_showText = !m_showText;
      return true;
    }
  else if (event->key() == Qt::Key_C)
    {
      m_showCoordinates = !m_showCoordinates;
      return true;
    }
  else if (event->key() == Qt::Key_X)
    {
      setMoveAxis(Landmarks::MoveX);
      return true;
    }
  else if (event->key() == Qt::Key_Y)
    {
      setMoveAxis(Landmarks::MoveY);
      return true;
    }
  else if (event->key() == Qt::Key_Z)
    {
      setMoveAxis(Landmarks::MoveZ);
      return true;
    }
  else if (event->key() == Qt::Key_W)
    {
      setMoveAxis(Landmarks::MoveAll);
      return true;
    }
}

void
Landmarks::draw(QGLViewer *viewer, bool backToFront)
{
  if (m_points.count() == 0)
    return;

  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);


  Vec voxelScaling = Global::voxelScaling();

  //--------------------
  // draw grabbed point
  if (m_grabbed >= 0)
    {
      glColor3f(1, 0.5, 0.5);
      glPointSize(m_pointSize+10);
      glBegin(GL_POINTS);
      Vec pt = m_points[m_grabbed];
      pt = VECPRODUCT(pt, voxelScaling);
      glVertex3fv(pt);
      glEnd();
    }
  //--------------------


  //--------------------
  // draw rest of the points
  glColor3fv(m_pointColor);
  glPointSize(m_pointSize);
  glBegin(GL_POINTS);
  for(int i=0; i<m_points.count();i++)
    {
      if (i != m_grabbed)
	{
	  Vec pt = m_points[i];
	  pt = VECPRODUCT(pt, voxelScaling);
	  
	  glVertex3fv(pt);
	}
    }
  glEnd();
  //--------------------


  glPointSize(1);

  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  
  glDisable(GL_POINT_SMOOTH);


  //drawProjectOnLine(viewer);

  //drawProjectOnPlane(viewer);
}

void
Landmarks::drawProjectOnLine(QGLViewer *viewer)
{
  if (m_projectLinePoints.count() == 0)
    return;

  Vec voxelScaling = Global::voxelScaling();

  Vec v0 = Vec(m_projectLineDistances[0][0],
	       m_projectLineDistances[0][1],
	       m_projectLineDistances[0][2]);
  v0 = VECPRODUCT(v0, voxelScaling);

  Vec v1 = Vec(m_projectLineDistances[1][0],
	       m_projectLineDistances[1][1],
	       m_projectLineDistances[1][2]);
  v1 = VECPRODUCT(v1, voxelScaling);

  Vec v = (v1-v0).unit();

  // --- draw projected points
  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);
  glColor3fv(m_pointColor);
  glPointSize(m_pointSize);
  glBegin(GL_POINTS);
  for(int i=0; i<m_projectLinePoints.count();i++)
    {
      int j = m_projectLinePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      float t = ((pt-v0)*v);      
      Vec ptp = v0 + t*v;      
      
      glVertex3fv(ptp);
    }
  glEnd();
  glPointSize(1);
  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_POINT_SMOOTH);
  // ----


  // --- draw projection lines
  glLineWidth(1);
  glBegin(GL_LINES);

  for(int i=0; i<m_projectLinePoints.count();i++)
    {
      int j = m_projectLinePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      float t = ((pt-v0)*v);      
      Vec ptp = v0 + t*v;      
      
      glVertex3fv(pt);
      glVertex3fv(ptp);
    }
  glEnd();
  // ----
}

void
Landmarks::postdraw(QGLViewer *viewer)
{
  if (m_points.count() == 0)
    return;

  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  QColor textColor = QColor(m_textColor.z*255,
			    m_textColor.y*255,
			    m_textColor.x*255);

			    
  for(int i=0; i<m_points.count(); i++)
    {
      if (m_showNumber ||
	  m_showText ||
	  m_showCoordinates)
	{
	  Vec pt = m_points[i];

	  Vec spt = VECPRODUCT(pt, voxelScaling);
	  Vec scr = viewer->camera()->projectedCoordinatesOf(spt);
	  int x = scr.x;
	  int y = scr.y;
	  
	  QString str;

	  if (m_showNumber)
	    str = QString("%1 ").arg(i+1);

	  if (m_showText)
	    str += QString("%1 ").arg(m_text[i]);

	  if (m_showCoordinates)
	    {
	      str += QString(" (%1 %2 %3)").\
		arg(pt.x*voxelSize.x, 0, 'f', Global::floatPrecision()).\
		arg(pt.y*voxelSize.y, 0, 'f', Global::floatPrecision()).\
		arg(pt.z*voxelSize.z, 0, 'f', Global::floatPrecision());
	    }

	  QFont font = QFont("Helvetica", m_textSize, QFont::Bold);
	  QFontMetrics metric(font);
	  int ht = metric.height();
	  int wd = metric.width(str);

	  //---------------------
	  x *= viewer->size().width()/viewer->camera()->screenWidth();
	  y *= viewer->size().height()/viewer->camera()->screenHeight();
	  //---------------------

	  y += ht/2;

	  StaticFunctions::renderText(x+m_pointSize/2, y,
				      str, font,
				      QColor(10,10,10,100), textColor);
	}
    }

  postdrawLength(viewer);

  postdrawLineProjection(viewer);
  postdrawProjectedLineLength(viewer);

  postdrawPlaneProjection(viewer);
  postdrawProjectedPlaneLength(viewer);

  viewer->stopScreenCoordinatesSystem();
}

void
Landmarks::postdrawLineProjection(QGLViewer *viewer)
{
  if (m_projectLinePoints.count() == 0)
    return;

  QColor textColor = QColor(m_textColor.z*255,
			    m_textColor.y*255,
			    m_textColor.x*255);

  Vec voxelScaling = Global::voxelScaling();

  Vec v0 = Vec(m_projectLineDistances[0][0],
	       m_projectLineDistances[0][1],
	       m_projectLineDistances[0][2]);
  v0 = VECPRODUCT(v0, voxelScaling);

  Vec v1 = Vec(m_projectLineDistances[1][0],
	       m_projectLineDistances[1][1],
	       m_projectLineDistances[1][2]);
  v1 = VECPRODUCT(v1, voxelScaling);

  Vec v = (v1-v0).unit();

  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, 0xAAAA);
  glBegin(GL_LINES);
  for(int i=0; i<m_projectLinePoints.count();i++)
    {
      int j = m_projectLinePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x = scr.x;
      int y = scr.y;
      glVertex2f(x, y);

      float t = ((pt-v0)*v);      
      pt = v0 + t*v;      
      scr = viewer->camera()->projectedCoordinatesOf(pt);
      x = scr.x;
      y = scr.y;      
      glVertex2f(x, y);
    }
  glEnd();
  glDisable(GL_LINE_STIPPLE);


//  for(int i=0; i<m_projectLinePoints.count();i++)
//    {
//      int j = m_projectLinePoints[i]-1;
//      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
//      float t = ((pt-v0)*v);      
//      pt = v0 + t*v;      
//      
//      //Vec spt = VECPRODUCT(pt, voxelScaling);
//      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
//      int x = scr.x;
//      int y = scr.y;
//      
//      QString str;
//      
//      if (m_showNumber)
//	str = QString("%1 ").arg(j+1);
//      
//      if (m_showText)
//	str += QString("%1 ").arg(m_text[j]);
//      
//      QFont font = QFont("Helvetica", m_textSize);
//      QFontMetrics metric(font);
//      int ht = metric.height();
//      int wd = metric.width(str);
//      
//      //---------------------
//      x *= viewer->size().width()/viewer->camera()->screenWidth();
//      y *= viewer->size().height()/viewer->camera()->screenHeight();
//      //---------------------
//
//      x -= wd/2;
//      y += ht/2;
//      StaticFunctions::renderText(x+m_pointSize/2, y+m_pointSize/2,
//				  str, font,
//				  QColor(10,10,10,100), textColor);
//    }
}

void
Landmarks::postdrawLength(QGLViewer *viewer)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  QColor textColor = QColor(m_textColor.z*255,
			    m_textColor.y*255,
			    m_textColor.x*255);

  glColor3f(m_textColor.x,
	    m_textColor.y,
	    m_textColor.y);

  int m_lengthTextDistance = 0;

  for (int i=0; i<m_distances.count(); i++)
    {
      int p0, p1;
      p0 = m_distances[i][0]-1;
      p1 = m_distances[i][1]-1;

      Vec pt = VECPRODUCT(m_points[p0], voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x0 = scr.x;
      int y0 = scr.y;
      //---------------------
      x0 *= viewer->size().width()/viewer->camera()->screenWidth();
      y0 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
      
      pt = VECPRODUCT(m_points[p1], voxelScaling);
      scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x1 = scr.x;
      int y1 = scr.y;
      //---------------------
      x1 *= viewer->size().width()/viewer->camera()->screenWidth();
      y1 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
  
  
      float perpx = (y1-y0);
      float perpy = -(x1-x0);
      float dlen = sqrt(perpx*perpx + perpy*perpy);
      perpx/=dlen; perpy/=dlen;
      
      float angle = atan2(-perpx, perpy);
      bool angleFixed = false;
      angle *= 180.0/3.1415926535;
      if (perpy < 0) { angle = 180+angle; angleFixed = true; }
      
      float px = perpx * m_lengthTextDistance;
      float py = perpy * m_lengthTextDistance;
      
      glLineWidth(1);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(2, 0xAAAA);
      glBegin(GL_LINES);
      glVertex2f(x0, y0);
      glVertex2f(x0+(px*1.1), y0+(py*1.1));
      glVertex2f(x1, y1);
      glVertex2f(x1+(px*1.1), y1+(py*1.1));
      glEnd();
      glDisable(GL_LINE_STIPPLE);
      
      glBegin(GL_LINES);
      glVertex2f(x0+px, y0+py);
      glVertex2f(x1+px, y1+py);
      glEnd();

      pt = m_points[p0]-m_points[p1];
      pt = VECPRODUCT(pt, voxelSize);
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      QString str = QString("%1 %2").				\
	arg(pt.norm(), 0, 'f', Global::floatPrecision()).	\
	arg(pvlInfo.voxelUnitStringShort()); 
      
      glPushMatrix();
      glLoadIdentity();
      
      if (str.endsWith("um"))
	{
	  str.chop(2);
	  str += QChar(0xB5);
	  str += "m";
	}
      QFont font = QFont("Helvetica", m_textSize);
      int x = (x0+x1)/2 + px*1.3;
      int y = (y0+y1)/2 + py*1.3;
      StaticFunctions::renderRotatedText(x,y,
					 str, font,
					 QColor(10,10,10,100), textColor,
					 -angle,
					 true); // (0,0) is bottom left
      
      glPopMatrix();
    }
}

void
Landmarks::postdrawProjectedLineLength(QGLViewer *viewer)
{
  if (m_projectLinePoints.count() == 0)
    return;

  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  QColor textColor = QColor(m_textColor.z*255,
			    m_textColor.y*255,
			    m_textColor.x*255);

  glColor3f(m_textColor.x,
	    m_textColor.y,
	    m_textColor.y);

  Vec v0 = Vec(m_projectLineDistances[0][0],
	       m_projectLineDistances[0][1],
	       m_projectLineDistances[0][2]);
  v0 = VECPRODUCT(v0, voxelScaling);

  Vec v1 = Vec(m_projectLineDistances[1][0],
	       m_projectLineDistances[1][1],
	       m_projectLineDistances[1][2]);
  v1 = VECPRODUCT(v1, voxelScaling);

  Vec v = (v1-v0).unit();


  for (int i=2; i<m_projectLineDistances.count(); i++)
    {
      int p0, p1;
      p0 = m_projectLineDistances[i][0]-1;
      p1 = m_projectLineDistances[i][1]-1;
      int m_lengthTextDistance = m_projectLineDistances[i][2];

      float t = ((m_points[p0]-v0)*v);      
      Vec pt0 = v0 + t*v;      
      Vec pt = VECPRODUCT(pt0, voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x0 = scr.x;
      int y0 = scr.y;
      //---------------------
      x0 *= viewer->size().width()/viewer->camera()->screenWidth();
      y0 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
      
      t = ((m_points[p1]-v0)*v);      
      Vec pt1 = v0 + t*v;      
      pt = VECPRODUCT(pt1, voxelScaling);
      scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x1 = scr.x;
      int y1 = scr.y;
      //---------------------
      x1 *= viewer->size().width()/viewer->camera()->screenWidth();
      y1 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
  
  
      float perpx = (y1-y0);
      float perpy = -(x1-x0);
      float dlen = sqrt(perpx*perpx + perpy*perpy);
      perpx/=dlen; perpy/=dlen;
      
      float angle = atan2(-perpx, perpy);
      bool angleFixed = false;
      angle *= 180.0/3.1415926535;
      if (perpy < 0) { angle = 180+angle; angleFixed = true; }
      
      float px = perpx * m_lengthTextDistance;
      float py = perpy * m_lengthTextDistance;
      
      glLineWidth(1);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(2, 0xAAAA);
      glBegin(GL_LINES);
      glVertex2f(x0, y0);
      glVertex2f(x0+(px*1.1), y0+(py*1.1));
      glVertex2f(x1, y1);
      glVertex2f(x1+(px*1.1), y1+(py*1.1));
      glEnd();
      glDisable(GL_LINE_STIPPLE);
      
      glBegin(GL_LINES);
      glVertex2f(x0+px, y0+py);
      glVertex2f(x1+px, y1+py);
      glEnd();

      pt = pt0 - pt1;
      pt = VECPRODUCT(pt, voxelSize);
      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      QString str = QString("%1 %2").				\
	arg(pt.norm(), 0, 'f', Global::floatPrecision()).	\
	arg(pvlInfo.voxelUnitStringShort()); 
      
      glPushMatrix();
      glLoadIdentity();
      
      if (str.endsWith("um"))
	{
	  str.chop(2);
	  str += QChar(0xB5);
	  str += "m";
	}
      QFont font = QFont("Helvetica", m_textSize);
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      int x = (x0+x1)/2 + px;
      int y = (y0+y1)/2 + py;
      StaticFunctions::renderRotatedText(x,y,
					 str, font,
					 QColor(10,10,10,200), textColor,
					 -angle,
					 true); // (0,0) is bottom left
      
      glPopMatrix();
    }
}

void
Landmarks::updateTable()
{
  // clear all information from the table
  while(m_tableWidget->rowCount() > 0)
    m_tableWidget->removeRow(0);


  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;
  for (int i=0; i<m_points.count(); i++)
    {
      m_tableWidget->insertRow(i);
      m_tableWidget->setRowHeight(i, 25);

      QTableWidgetItem *wiT;
      wiT = new QTableWidgetItem(m_text[i]);
      m_tableWidget->setItem(i, 0, wiT);

      QTableWidgetItem *wiC;
      wiC = new QTableWidgetItem(QString("%1 %2 %3").\
				 arg(m_points[i].x*voxelSize.x). \
				 arg(m_points[i].y*voxelSize.y). \
				 arg(m_points[i].z*voxelSize.z));
      m_tableWidget->setItem(i, 1, wiC);
    }
}

void
Landmarks::reorderLandmarks()
{
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  QList<Vec> pts = m_points;
  QList<QString> txt = m_text;
  m_points.clear();
  m_text.clear();
  m_points.reserve(m_tableWidget->rowCount());
  m_text.reserve(m_tableWidget->rowCount());

  for (int i=0; i<m_tableWidget->rowCount(); i++)
    m_points << Vec(0,0,0);
  for (int i=0; i<m_tableWidget->rowCount(); i++)
    m_text << "";

  for (int i=0; i<m_tableWidget->rowCount(); i++)
    {
      int li = m_tableWidget->verticalHeader()->visualIndex(i);
      m_points[li] = pts[i];
      m_text[li] = txt[i];
    }

  emit updateGL();
  updateTable();
}

void
Landmarks::updateLandmarks(int r, int c)
{
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  QTableWidgetItem *wi;
  wi = m_tableWidget->item(r, c);
  QString cc = wi->text().simplified();

  if (c == 0)
    {
      cc.remove(' ');
      m_text[r] = cc;
      wi->setText(cc);
    }
  else if (c == 1)
    {
      QStringList cv = cc.split(" ", QString::SkipEmptyParts);

      float x, y, z;
      x = cv[0].toFloat()/voxelSize.x;
      y = cv[1].toFloat()/voxelSize.y;
      z = cv[2].toFloat()/voxelSize.z;
      m_points[r] = Vec(x,y,z);
    }

  emit updateGL();
}

void
Landmarks::saveLandmarks()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save landmarks to text file",
				      Global::previousDirectory(),
				      "Files (*.landmark)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  if (flnm.isEmpty())
    return;

  QFile lf(flnm);
  if (lf.open(QFile::WriteOnly | QFile::Truncate))
    {
      int npts = m_points.count();
      QTextStream fp(&lf);
      fp << npts << "\n";
      for(int i=0; i<npts; i++)
	{
	  Vec pt = m_points[i];
	  fp << i+1 << "   ";
	  fp << m_text[i] << "   ";
	  fp << pt.x << " " << pt.y << " " << pt.z << "\n";
	}

      QMessageBox::information(0, "Save Landmarks", "Saved landmarks to "+flnm);
    }
  else
    QMessageBox::information(0, "Save Landmarks", "Cannot save landmarks to "+flnm);
}

void
Landmarks::loadLandmarks()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "load landmarks to text file",
				      Global::previousDirectory(),
				      "Files (*.landmark)",
				      0,
				      QFileDialog::DontUseNativeDialog);
  if (flnm.isEmpty())
    return;

  loadLandmarks(flnm);
}

void
Landmarks::loadLandmarks(QString flnm)
{
  QList<Vec> pts;
  QList<QString> txt;

  int npts;
  QFile lf(flnm);  
  if (lf.open(QFile::ReadOnly))
    {
      QTextStream fp(&lf);
      QString str;
      str = fp.readLine();
      QStringList cv = str.split(" ", QString::SkipEmptyParts);
      if (cv.count() == 1)
	{
	  npts = str.toFloat();

	  for(int i=0; i<npts; i++)
	    pts << Vec(0,0,0);
	  for(int i=0; i<npts; i++)
	    txt << "";

	  for(int i=0; i<npts; i++)
	    {
	      str = fp.readLine();      
	      QStringList cv = str.split(" ", QString::SkipEmptyParts);
	      int id;
	      QString t;
	      float x,y,z;
	      id = i+1;	      
	      if (cv.count() == 3)
		{
		  x = cv[0].toFloat();
		  y = cv[1].toFloat();
		  z = cv[2].toFloat();
		}
	      else if (cv.count() == 4)
		{
		  t = cv[0];
		  x = cv[1].toFloat();
		  y = cv[2].toFloat();
		  z = cv[3].toFloat();
		}
	      else if (cv.count() == 5)
		{
		  id = cv[0].toInt();
		  t = cv[1];
		  x = cv[2].toFloat();
		  y = cv[3].toFloat();
		  z = cv[4].toFloat();
		}
	      if (id>0 && id <= npts)
		{
		  pts[id-1] = Vec(x,y,z);
		  txt[id-1] = t;
		}
	    }
	}
      else
	{
	  do {
	    cv = str.split(" ", QString::SkipEmptyParts);
	    int id;
	    QString t;
	    float x,y,z;
	    if (cv.count() == 3)
	      {
		x = cv[0].toFloat();
		y = cv[1].toFloat();
		z = cv[2].toFloat();
	      }
	    else if (cv.count() == 4)
	      {
		t = cv[0];
		x = cv[1].toFloat();
		y = cv[2].toFloat();
		z = cv[3].toFloat();
	      }
	    else if (cv.count() == 5)
	      {
		id = cv[0].toInt();
		t = cv[1];
		x = cv[2].toFloat();
		y = cv[3].toFloat();
		z = cv[4].toFloat();
	      }
	    pts << Vec(x,y,z);
	    txt << t;

	    str = fp.readLine();      
	  }
	  while(!str.isNull());
	}
    }

  m_points.clear();
  m_text.clear();

  m_points << pts;
  m_text << txt;

  updateTable();
  m_table->show();

  QMessageBox::information(0, "Load Landmarks", "Loaded landmarks from "+flnm);
}

void
Landmarks::clearAllLandmarks()
{
  m_points.clear();
  m_text.clear();
  m_grabbed = -1;
  m_pressed = -1;
  updateTable();
  emit updateGL();
}

void
Landmarks::changePointColor()
{  
  QColor color = DColorDialog::getColor(QColor(m_pointColor.x*255,
					       m_pointColor.y*255,
					       m_pointColor.z*255));
  float x = color.red()/255.0f;
  float y = color.green()/255.0f;
  float z = color.blue()/255.0f;
  m_pointColor = Vec(x,y,z);

  emit updateGL();
}

void
Landmarks::changePointSize(int s)
{
  m_pointSize = s;
  
  emit updateGL();
}

void
Landmarks::changeTextColor()
{  
  QColor color = DColorDialog::getColor(QColor(m_textColor.x*255,
					       m_textColor.y*255,
					       m_textColor.z*255));
  float x = color.red()/255.0f;
  float y = color.green()/255.0f;
  float z = color.blue()/255.0f;
  m_textColor = Vec(x,y,z);

  emit updateGL();
}

void
Landmarks::changeTextSize(int s)
{
  m_textSize = s;
  
  emit updateGL();
}

QList< QList<int> > Landmarks::distances() { return m_distances; }
QList< QList<int> > Landmarks::angles() { return m_angles; }

void
Landmarks::setDistances(QList< QList<int> > d)
{
  m_distances.clear();

  QString s;
  for (int i=0; i<m_distances.count(); i++)
    {
      QList<int> lp = m_distances[i];
      if (lp.count() == 2)
	{
	  m_distances << lp;
	  s += QString("%1 %2 ,").arg(lp[0]).arg(lp[1]);
	}
    }
  
  s.chop(1); // remove the last ,

  m_distEdit->setText(s);
}

void
Landmarks::setAngles(QList< QList<int> > d)
{
  m_angles.clear();

  QString s;
  for (int i=0; i<m_distances.count(); i++)
    {
      QList<int> lp = m_distances[i];
      if (lp.count() == 3)
	{
	  m_angles << lp;
	  s += QString("%1 %2 %3 ,").arg(lp[0]).arg(lp[1]).arg(lp[2]);
	}
    }
  
  s.chop(1); // remove the last ,

  m_angleEdit->setText(s);
}

void
Landmarks::updateDistances()
{
  m_distances.clear();

  QString str = m_distEdit->text().simplified();

  if (str.length() == 0)
    return;

  QStringList dwords = str.split(",", QString::SkipEmptyParts);
  for (int i=0; i<dwords.count(); i++)
    {
      QStringList words = dwords[i].simplified().split(" ", QString::SkipEmptyParts);
      if (words.count() == 2)
	{
	  QList<int> lp;
	  lp << words[0].toInt();
	  lp << words[1].toInt();
	  
	  m_distances << lp;
	}
    }

  emit updateGL();
}

void
Landmarks::updateAngles()
{
  m_angles.clear();

  QString str = m_angleEdit->text().simplified();

  if (str.length() == 0)
    return;

  QStringList dwords = str.split(",", QString::SkipEmptyParts);
  for (int i=0; i<dwords.count(); i++)
    {
      QStringList words = dwords[i].simplified().split(" ", QString::SkipEmptyParts);
      if (words.count() == 3)
	{
	  QList<int> lp;
	  lp << words[0].toInt();
	  lp << words[1].toInt();
	  lp << words[2].toInt();
	  
	  m_angles << lp;
	}
    }

  emit updateGL();
}

void
Landmarks::updateProjectOnLine()
{
  m_projectLineDistances.clear();
  m_projectLinePoints.clear();

  QString str = m_projOnLineEdit->text().simplified();

  if (str.length() == 0)
    return;

  QStringList dwords = str.split(";", QString::SkipEmptyParts);
  if (dwords.count() != 2)
    {
      QMessageBox::information(0, "", "Format - 2 coordinates to define projection line followed by point pairs and separations for distance calculation.\nFor e.g\n 0.0 0.0 0.0 , 1.0 0.0 0.0 ; 1 2 0, 2 3 10, 2 10 20");
      return;
    }

  QStringList dwords0 = dwords[0].split(",", QString::SkipEmptyParts);
  if (dwords0.count() != 2)
    {
      QMessageBox::information(0, "", "Format - 2 coordinates to define projection line followed by point pairs and separations for distance calculation.\nFor e.g\n 0.0 0.0 0.0 , 1.0 0.0 0.0 ; 1 2 0, 2 3 10, 2 10 20");
      return;
    }

  for (int i=0; i<dwords0.count(); i++) // get projection line coordinates
    {
      QStringList words = dwords0[i].split(" ", QString::SkipEmptyParts);
      if (words.count() == 3)
	{
	  QList<float> lp;
	  lp << words[0].toFloat();
	  lp << words[1].toFloat();
	  lp << words[2].toFloat();	  
	  m_projectLineDistances << lp;
	}
      else
	{
	  QMessageBox::information(0, "", "Format - 2 coordinates to define projection line followed by point pairs for distance calculation.\nFor e.g\n 0.0 0.0 0.0 , 1.0 0.0 0.0 ; 1 2 , 2 3, 2 10");
	  m_projectLineDistances.clear();
	  return;
	}
    }
  
  // now get projection points
  QStringList words = dwords[1].split(",", QString::SkipEmptyParts);
  QMap<int, int> map;
  for (int i=0; i<words.count(); i++)
    {
      QStringList wrd = words[i].simplified().split(" ", QString::SkipEmptyParts);
      if (wrd.count() == 3)
	{
	  // point0, point1, text separation
	  QList<float> lp;
	  lp << wrd[0].toInt();
	  lp << wrd[1].toInt();
	  lp << wrd[2].toInt();

	  map[lp[0]] = lp[0];
	  map[lp[1]] = lp[1];
	  m_projectLineDistances << lp;
	}
    }

  m_projectLinePoints = map.values();

  emit updateGL();
}

QList< QList<float> > Landmarks::projectOnLine() { return m_projectLineDistances; }

void
Landmarks::setProjectOnLine(QList< QList<float> > d)
{
  m_projectLineDistances.clear();
  m_projectLinePoints.clear();
  
  QMap<int, int> map;
  QString s;
  for (int i=0; i<d.count(); i++)
    {
      QList<float> lp = d[i];
      if (lp.count() == 3)
	{
	  m_projectLineDistances << lp;
	  s += QString("%1 %2 %3").arg(lp[0]).arg(lp[1]).arg(lp[2]);
	  if (i == 0) s += " , ";
	  else if (i == 1) s += " ; ";
	  else
	    {
	      s += " , ";
	      map[lp[0]] = lp[0];
	      map[lp[1]] = lp[1];
	      map[lp[2]] = lp[2];
	    }
	}
    }

  m_projectLinePoints = map.values();
  
  s.chop(3); // remove the last ,

  m_projOnLineEdit->setText(s);
}

void
Landmarks::updateProjectOnPlane()
{
  m_projectPlanePoints.clear();

  QString str = m_projOnPlaneEdit->text().simplified();

  if (str.length() == 0)
    return;

  QStringList dwords = str.split(";", QString::SkipEmptyParts);
  if (dwords.count() != 2)
    {
      QMessageBox::information(0, "", "Format - 2 coordinates to define projection line followed by points to project on plane for distance calculation.\nFor e.g\n 0.0 0.0 0.0 , 1.0 0.0 0.0 ; 1, 3");
      return;
    }

  QStringList dwords0 = dwords[0].split(",", QString::SkipEmptyParts);
  if (dwords0.count() != 2)
    {
      QMessageBox::information(0, "", "Format - 2 coordinates to define projection line followed by points to project on plane for distance calculation.\nFor e.g\n 0.0 0.0 0.0 , 1.0 0.0 0.0 ; 1, 3");
      return;
    }

  for (int i=0; i<dwords0.count(); i++) // get projection line coordinates
    {
      QStringList words = dwords0[i].split(" ", QString::SkipEmptyParts);
      if (words.count() == 3)
	{
	  m_projectPlanePoints << words[0].toFloat();
	  m_projectPlanePoints << words[1].toFloat();
	  m_projectPlanePoints << words[2].toFloat();	  
	}
      else
	{
	  QMessageBox::information(0, "", "Format - 2 coordinates to define projection line followed by points to project on plane for distance calculation.\nFor e.g\n 0.0 0.0 0.0 , 1.0 0.0 0.0 ; 1, 3");
	  m_projectPlanePoints.clear();
	  return;
	}
    }

  // now get projection points
  QStringList words = dwords[1].split(",", QString::SkipEmptyParts);
  for (int i=0; i<words.count(); i++)
    m_projectPlanePoints << words[i].toInt();

  emit updateGL();
}

QList<float> Landmarks::projectOnPlane() { return m_projectPlanePoints; }

void
Landmarks::setProjectOnPlane(QList<float> d)
{
  m_projectPlanePoints.clear();
  m_projectPlanePoints = d;

  QString s;
  for (int i=0; i<d.count(); i++)
    {
      s += QString("%1 ").arg(d[i]);
      if (i == 2) s += ", ";
      else if (i == 5) s += "; ";
      else if (i > 5)
	s += ", ";
    }

  s.chop(2); // remove the last ,

  m_projOnPlaneEdit->setText(s);
}

void
Landmarks::drawProjectOnPlane(QGLViewer *viewer)
{
  if (m_projectPlanePoints.count() == 0)
    return;

  Vec voxelScaling = Global::voxelScaling();

  Vec p0 = Vec(m_projectPlanePoints[0],
	       m_projectPlanePoints[1],
	       m_projectPlanePoints[2]);
  p0 = VECPRODUCT(p0, voxelScaling);
  
  Vec norm = Vec(m_projectPlanePoints[3],
		 m_projectPlanePoints[4],
		 m_projectPlanePoints[5]).unit();

  // --- draw projected points
  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);
  glColor3fv(m_pointColor);
  glPointSize(m_pointSize);
  glBegin(GL_POINTS);
  for(int i=6; i<m_projectPlanePoints.count();i++)
    {
      int j = m_projectPlanePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);

      
      float t = ((pt-p0)*norm);      
      Vec ptp = pt - t*norm;      
      
      glVertex3fv(ptp);
    }
  glEnd();
  glPointSize(1);
  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_POINT_SMOOTH);
  // ----


  // --- draw projection lines
  glLineWidth(1);
  glBegin(GL_LINES);

  for(int i=6; i<m_projectPlanePoints.count();i++)
    {
      int j = m_projectPlanePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      
      float t = ((pt-p0)*norm);      
      Vec ptp = pt - t*norm;      

      glVertex3fv(pt);
      glVertex3fv(ptp);
    }
  glEnd();
  // ----
}

void
Landmarks::postdrawPlaneProjection(QGLViewer *viewer)
{
  if (m_projectPlanePoints.count() == 0)
    return;

  QColor textColor = QColor(m_textColor.z*255,
			    m_textColor.y*255,
			    m_textColor.x*255);

  Vec voxelScaling = Global::voxelScaling();

  Vec p0 = Vec(m_projectPlanePoints[0],
	       m_projectPlanePoints[1],
	       m_projectPlanePoints[2]);
  p0 = VECPRODUCT(p0, voxelScaling);
  
  Vec norm = Vec(m_projectPlanePoints[3],
		 m_projectPlanePoints[4],
		 m_projectPlanePoints[5]).unit();

  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, 0xAAAA);
  glBegin(GL_LINES);
  for(int i=6; i<m_projectPlanePoints.count();i++)
    {
      int j = m_projectPlanePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x = scr.x;
      int y = scr.y;
      glVertex2f(x, y);

      float t = ((pt-p0)*norm);      
      Vec ptp = pt - t*norm;      
      scr = viewer->camera()->projectedCoordinatesOf(ptp);
      x = scr.x;
      y = scr.y;      
      glVertex2f(x, y);
    }
  glEnd();
  glDisable(GL_LINE_STIPPLE);
}

void
Landmarks::postdrawProjectedPlaneLength(QGLViewer *viewer)
{
  if (m_projectPlanePoints.count() == 0)
    return;

  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;

  QColor textColor = QColor(m_textColor.z*255,
			    m_textColor.y*255,
			    m_textColor.x*255);

  glColor3f(m_textColor.x,
	    m_textColor.y,
	    m_textColor.y);

  Vec p0 = Vec(m_projectPlanePoints[0],
	       m_projectPlanePoints[1],
	       m_projectPlanePoints[2]);
  p0 = VECPRODUCT(p0, voxelScaling);
  
  Vec norm = Vec(m_projectPlanePoints[3],
		 m_projectPlanePoints[4],
		 m_projectPlanePoints[5]).unit();

  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);
  glPointSize(m_pointSize);
  glBegin(GL_POINTS);
  for(int i=6; i<m_projectPlanePoints.count();i++)
    {
      int j = m_projectPlanePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      float dtn = ((pt-p0)*norm);      
      Vec ptp = pt - dtn*norm;      
      Vec scr = viewer->camera()->projectedCoordinatesOf(ptp);
      int x0 = scr.x;
      int y0 =  scr.y;
      //---------------------
      x0 *= viewer->size().width()/viewer->camera()->screenWidth();
      y0 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------

      glVertex2f(x0, y0);
    }
  glEnd();

  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  
  glDisable(GL_POINT_SMOOTH);


  int m_lengthTextDistance = 0;
  for(int i=6; i<m_projectPlanePoints.count();i++)
    {
      int j = m_projectPlanePoints[i]-1;
      Vec pt = VECPRODUCT(m_points[j], voxelScaling);
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x0 = scr.x;
      int y0 =  scr.y;
      //---------------------
      x0 *= viewer->size().width()/viewer->camera()->screenWidth();
      y0 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------

      float dtn = ((pt-p0)*norm);      
      Vec ptp = pt - dtn*norm;      
      scr = viewer->camera()->projectedCoordinatesOf(ptp);
      int x1 = scr.x;
      int y1 = scr.y;      
      //---------------------
      x1 *= viewer->size().width()/viewer->camera()->screenWidth();
      y1 *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------

      float perpx = (y1-y0);
      float perpy = -(x1-x0);
      float dlen = sqrt(perpx*perpx + perpy*perpy);
      perpx/=dlen; perpy/=dlen;
      
      float angle = atan2(-perpx, perpy);
      bool angleFixed = false;
      angle *= 180.0/3.1415926535;
      if (perpy < 0) { angle = 180+angle; angleFixed = true; }
      
      float px = perpx * m_lengthTextDistance;
      float py = perpy * m_lengthTextDistance;
      
      glLineWidth(1);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(2, 0xAAAA);
      glBegin(GL_LINES);
      glVertex2f(x0, y0);
      glVertex2f(x0+(px*1.1), y0+(py*1.1));
      glVertex2f(x1, y1);
      glVertex2f(x1+(px*1.1), y1+(py*1.1));
      glEnd();
      glDisable(GL_LINE_STIPPLE);
      
      glBegin(GL_LINES);
      glVertex2f(x0+px, y0+py);
      glVertex2f(x1+px, y1+py);
      glEnd();

      VolumeInformation pvlInfo = VolumeInformation::volumeInformation();
      QString str = QString("%1 %2").				\
	arg(dtn, 0, 'f', Global::floatPrecision()).	\
	arg(pvlInfo.voxelUnitStringShort()); 
      
      glPushMatrix();
      glLoadIdentity();
      
      if (str.endsWith("um"))
	{
	  str.chop(2);
	  str += QChar(0xB5);
	  str += "m";
	}
      QFont font = QFont("Helvetica", m_textSize);
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      int x = (x0+x1)/2 + px;
      int y = (y0+y1)/2 + py;
      StaticFunctions::renderRotatedText(x,y,
					 str, font,
					 QColor(10,10,10,200), textColor,
					 -angle,
					 true); // (0,0) is bottom left
      
      glPopMatrix();
    }
}

