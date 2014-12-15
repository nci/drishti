#include "global.h"
#include "landmarks.h"
#include "staticfunctions.h"
#include "volumeinformation.h"

#include <fstream>
using namespace std;

#include <QMessageBox>
#include <QTextStream>

#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

int Landmarks::count() { return m_points.count(); }

void Landmarks::setPointSize(int sz) { m_pointSize = sz; }
int Landmarks::pointSize() { return m_pointSize; }

void Landmarks::setPointColor(Vec col) { m_pointColor = col; }
Vec Landmarks::pointColor() { return m_pointColor; }

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

  m_grab = false;
  m_grabbed = -1;
  m_pressed = -1;
  m_moveAxis = MoveAll;

  removeFromMouseGrabberPool();
}

Landmarks::Landmarks()
{
  m_table = new QWidget();
  m_tableWidget = new QTableWidget();

  QGroupBox *bG = new QGroupBox();
  QPushButton *reorderB = new QPushButton("Reorder");
  QPushButton *saveB = new QPushButton("Save");
  QPushButton *loadB = new QPushButton("Load");
  QPushButton *clearB = new QPushButton("DeleteAll");

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(reorderB);
  hbox->addWidget(saveB);
  hbox->addWidget(loadB);
  hbox->addWidget(clearB);
  hbox->addStretch(4);

  bG->setLayout(hbox);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(bG);
  vbox->addWidget(m_tableWidget);

  m_table->setLayout(vbox);

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
  item << "Text";
  item << "Coordinate";
  m_tableWidget->setHorizontalHeaderLabels(item);
  m_tableWidget->setColumnWidth(0, 50);
  m_tableWidget->setColumnWidth(1, 200);

  connect(reorderB, SIGNAL(clicked()), this, SLOT(reorderLandmarks()));
  connect(saveB, SIGNAL(clicked()), this, SLOT(saveLandmarks()));
  connect(loadB, SIGNAL(clicked()), this, SLOT(loadLandmarks()));
  connect(clearB, SIGNAL(clicked()), this, SLOT(clearAllLandmarks()));

  connect(m_tableWidget, SIGNAL(cellChanged(int, int)),
	  this, SLOT(updateLandmarks(int, int)));


  clear();
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
	      str += QString(" c(%1 %2 %3)").\
		arg(pt.x*voxelSize.x, 0, 'f', Global::floatPrecision()).\
		arg(pt.y*voxelSize.y, 0, 'f', Global::floatPrecision()).\
		arg(pt.z*voxelSize.z, 0, 'f', Global::floatPrecision());
	    }

	  QFont font = QFont();
	  QFontMetrics metric(font);
	  int ht = metric.height();
	  int wd = metric.width(str);

	  //---------------------
	  x *= viewer->size().width()/viewer->camera()->screenWidth();
	  y *= viewer->size().height()/viewer->camera()->screenHeight();
	  //---------------------

	  y += ht/2;

	  StaticFunctions::renderText(x+12, y, str, font, Qt::black, Qt::cyan);
	}
    }
  viewer->stopScreenCoordinatesSystem();
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
				      "Files (*.landmark)");  
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
				      "Files (*.landmark)");  
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
