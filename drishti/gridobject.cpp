#include "global.h"
#include "staticfunctions.h"
#include "gridobject.h"
#include "volumeinformation.h"
#include "enums.h"

#include <QMessageBox>

//------------------------------------------------------------------
GridObjectUndo::GridObjectUndo() { clear(); }
GridObjectUndo::~GridObjectUndo() { clear(); }

void
GridObjectUndo::clear()
{
  m_rows.clear();
  m_cols.clear();
  m_points.clear();
  m_index = -1;
}

void
GridObjectUndo::clearTop()
{
  if (m_index == m_points.count()-1)
    return;

  while(m_index < m_points.count()-1)
    m_points.removeLast();   

  while(m_index < m_rows.count()-1)
    m_rows.removeLast();   

  while(m_index < m_cols.count()-1)
    m_cols.removeLast();   
}

void
GridObjectUndo::append(int c, int r, QList<Vec> p)
{
  clearTop();
  m_cols << c;
  m_rows << r;
  m_points << p;
  m_index = m_points.count()-1;
}

void GridObjectUndo::redo() { m_index = qMin(m_index+1, m_points.count()-1); }
void GridObjectUndo::undo() { m_index = qMax(m_index-1, 0); }

void
GridObjectUndo::colrow(int& c, int& r)
{ 
  if (m_index >= 0 && m_index < m_points.count())
    {
      c = m_cols[m_index];
      r = m_rows[m_index];
    }  
}

QList<Vec>
GridObjectUndo::points()
{
  QList<Vec> p;

  if (m_index >= 0 && m_index < m_points.count())
    return m_points[m_index];

  return p;
}
//------------------------------------------------------------------


GridObject
GridObject::get()
{
  GridObject po;
  po = *this;
  return po;
}

void
GridObject::set(const GridObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation

  m_cols = po.m_cols;
  m_rows = po.m_rows;

  m_showPointNumbers = po.m_showPointNumbers;
  m_showPoints = po.m_showPoints;
  m_color = po.m_color;
  m_opacity = po.m_opacity;
  m_points = po.m_points;

  m_undo.clear();
  updateUndo();
}

GridObject&
GridObject::operator=(const GridObject &po)
{
  m_pointPressed = -1;

  m_updateFlag = true; // for recomputation

  m_cols = po.m_cols;
  m_rows = po.m_rows;

  m_showPointNumbers = po.m_showPointNumbers;
  m_showPoints = po.m_showPoints;
  m_color = po.m_color;
  m_opacity = po.m_opacity;
  m_points = po.m_points;

  m_undo.clear();
  updateUndo();

  return *this;
}

GridObject::GridObject()
{
  m_undo.clear();

  m_pointPressed = -1;

  m_cols = 0;
  m_rows = 0;

  m_showPointNumbers = false;
  m_showPoints = true;
  m_updateFlag = false;
  m_color = Vec(0.9,0.5,0.2);
  m_opacity = 1;
  m_points.clear();
}

GridObject::~GridObject()
{
  m_undo.clear();

  m_cols = 0;
  m_rows = 0;

  m_points.clear();
}

bool GridObject::showPointNumbers() { return m_showPointNumbers; }
bool GridObject::showPoints() { return m_showPoints; }
Vec GridObject::color() { return m_color; }
float GridObject::opacity() { return m_opacity; }
int GridObject::columns() { return m_cols; }
int GridObject::rows() { return m_rows; }
QList<Vec> GridObject::points() { return m_points; }
Vec GridObject::getPoint(int i)
{
  if (i < m_points.count())
    return m_points[i];
  else
    return Vec(0,0,0);
}

void GridObject::setShowPointNumbers(bool flag) { m_showPointNumbers = flag; }
void GridObject::setShowPoints(bool flag) { m_showPoints = flag; }
void GridObject::setColor(Vec color)
{
  m_color = color;
}
void GridObject::setOpacity(float op)
{
  m_opacity = op;
}
void GridObject::setPoint(int i, Vec pt)
{
  if (i >= 0 && i < m_points.count())
    {
      m_points[i] = pt;
      m_updateFlag = true;
    }
  else
    {
      for(int j=0; j<m_points.count(); j++)
	m_points[j] += pt;
      m_updateFlag = true;
    }
  updateUndo();
}
void GridObject::normalize()
{
  for(int i=0; i<m_points.count(); i++)
    {
      Vec pt = m_points[i];
      pt = Vec((int)pt.x, (int)pt.y, (int)pt.z);
      m_points[i] = pt;
    }
  m_updateFlag = true;
  updateUndo();
}
void
GridObject::setColRow(int c, int r)
{
  m_cols = c;
  m_rows = r;
}
void
GridObject::setPoints(QList<Vec> pts)
{
  m_pointPressed = -1;

  if (pts.count() < m_rows*m_cols)
    {
      QMessageBox::information(0, QString("%1 points").arg(pts.count()),
			       "Number of points must be equal to cols*rows");
      return;
    }

  m_points = pts;

  m_updateFlag = true;
  updateUndo();
}

void
GridObject::insertRow(int r)
{
  if (r < 0 || r >= m_rows-1)
    return;

  QList<Vec> newp;
  for(int i=0; i<m_rows; i++)
    {
      for(int j=0; j<m_cols; j++)
	newp << m_points[i*m_cols+j];

      if (i == r)
	{
	  for(int j=0; j<m_cols; j++)
	    newp << (m_points[i*m_cols+j] + m_points[(i+1)*m_cols+j])/2;
	}
    }

  m_points.clear();
  m_points = newp;
  m_rows++;
  m_updateFlag = true;
  updateUndo();
}

void
GridObject::removeRow(int r)
{
  if (m_rows <= 2)
    {
      QMessageBox::information(0, "Error", "Must have atleast 2 rows");
      return;
    }

  if (r < 0 || r > m_rows-1)
    return;

  QList<Vec> newp;
  for(int i=0; i<m_rows; i++)
    {
      if (i != r)
	{
	  for(int j=0; j<m_cols; j++)
	    newp << m_points[i*m_cols+j];
	}
    }

  m_points.clear();
  m_points = newp;
  m_rows--;
  m_updateFlag = true;
  updateUndo();
}

void
GridObject::insertCol(int c)
{
  if (c < 0 || c >= m_cols-1)
    return;

  QList<Vec> newp;
  for(int i=0; i<m_rows; i++)
    {
      for(int j=0; j<m_cols; j++)
	{
	  newp << m_points[i*m_cols+j];
	  if (j == c)
	    newp << (m_points[i*m_cols+j] + m_points[i*m_cols+(j+1)])/2;
	}

    }

  m_points.clear();
  m_points = newp;
  m_cols++;
  m_updateFlag = true;
  updateUndo();
}

void
GridObject::removeCol(int c)
{
  if (m_cols <= 2)
    {
      QMessageBox::information(0, "Error", "Must have atleast 2 columns");
      return;
    }

  if (c < 0 || c > m_cols-1)
    return;

  QList<Vec> newp;
  for(int i=0; i<m_rows; i++)
    for(int j=0; j<m_cols; j++)
      {
	if (j != c)
	  newp << m_points[i*m_cols+j];
      }

  m_points.clear();
  m_points = newp;
  m_cols--;
  m_updateFlag = true;
  updateUndo();
}

void GridObject::setPointPressed(int p) { m_pointPressed = p; }
int GridObject::getPointPressed() { return m_pointPressed; }

QList<Vec> GridObject::gridPoints() { return m_grid; }

void
GridObject::regrid()
{
  Vec voxelScaling = Global::voxelScaling();
  Vec voxelSize = VolumeInformation::volumeInformation().voxelSize;
  m_grid.clear();
  for(int i=0; i<m_points.count(); i++)
    {
      Vec v = VECPRODUCT(m_points[i], voxelScaling);
      m_grid.append(v);
    }
}

void
GridObject::draw(QGLViewer *viewer,
		 bool active,
		 bool backToFront,
		 Vec lightPosition)
{
  if (m_updateFlag)
    {
      m_updateFlag = false;
      regrid();
    }

  drawLines(viewer, active, backToFront);
}


void
GridObject::drawLines(QGLViewer *viewer,
		      bool active,
		      bool backToFront)
{
  glEnable(GL_BLEND);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  Vec col = m_opacity*m_color;
  if (backToFront)
    {
      if (active)
        glLineWidth(7);
      else
        glLineWidth(3);

      glColor4f(col.x*0.5,
		col.y*0.5,
		col.z*0.5,
		m_opacity*0.5);

      for(int i=0; i<m_rows; i++)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int j=0; j<m_cols; j++)
	    glVertex3fv(m_grid[i*m_cols + j]);
	  glEnd();
	}
      for(int j=0; j<m_cols; j++)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int i=0; i<m_rows; i++)
	    glVertex3fv(m_grid[i*m_cols + j]);
	  glEnd();
	}
    }

  if (m_showPoints)
    {
      glColor3f(m_color.x,
		m_color.y,
		m_color.z);


      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);

      Vec voxelScaling = Global::voxelScaling();
      glPointSize(20);
      glBegin(GL_POINTS);
      for(int i=0; i<m_grid.count();i++)
	glVertex3fv(m_grid[i]);
      glEnd();


      if (m_pointPressed > -1)
	{
	  glColor3f(1,0,0);
	  Vec voxelScaling = Global::voxelScaling();
	  glPointSize(25);
	  glBegin(GL_POINTS);
	  glVertex3fv(m_grid[m_pointPressed]);
	  glEnd();
	}

      glPointSize(1);  

      glDisable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
  
      glDisable(GL_POINT_SMOOTH);
    }

  if (active)
    glLineWidth(3);
  else
    glLineWidth(1);

  glColor4f(col.x, col.y, col.z, m_opacity);

  for(int i=0; i<m_rows; i++)
    {
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<m_cols; j++)
	glVertex3fv(m_grid[i*m_cols + j]);
      glEnd();
    }
  for(int j=0; j<m_cols; j++)
    {
      glBegin(GL_LINE_STRIP);
      for(int i=0; i<m_rows; i++)
	glVertex3fv(m_grid[i*m_cols + j]);
      glEnd();
    }

  if (!backToFront)
    {
      if (active)
        glLineWidth(7);
      else
        glLineWidth(3);

      glColor4f(col.x*0.5,
		col.y*0.5,
		col.z*0.5,
		m_opacity*0.5);

      for(int i=0; i<m_rows; i++)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int j=0; j<m_cols; j++)
	    glVertex3fv(m_grid[i*m_cols + j]);
	  glEnd();
	}
      for(int j=0; j<m_cols; j++)
	{
	  glBegin(GL_LINE_STRIP);
	  for(int i=0; i<m_rows; i++)
	    glVertex3fv(m_grid[i*m_cols + j]);
	  glEnd();
	}
    }

  glLineWidth(1);
  glDisable(GL_LINE_SMOOTH);
}

void
GridObject::postdrawPointNumbers(QGLViewer *viewer)
{
  Vec voxelScaling = Global::voxelScaling();
  for(int i=0; i<m_grid.count();i++)
    {
      Vec pt = m_grid[i];
      Vec scr = viewer->camera()->projectedCoordinatesOf(pt);
      int x = scr.x;
      int y = scr.y;
      
      //---------------------
      x *= viewer->size().width()/viewer->camera()->screenWidth();
      y *= viewer->size().height()/viewer->camera()->screenHeight();
      //---------------------
      
      QString str = QString("%1").arg(i);
      QFont font = QFont();
      QFontMetrics metric(font);
      int ht = metric.height();
      int wd = metric.width(str);
      y += ht/2;
      
      StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::white);
    }
}

void
GridObject::postdraw(QGLViewer *viewer,
		     int x, int y,
		     bool grabsMouse)
{
  if (!grabsMouse &&
      !m_showPointNumbers)
    return;

  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  glDisable(GL_DEPTH_TEST);
  viewer->startScreenCoordinatesSystem();

  if (m_showPointNumbers)
    postdrawPointNumbers(viewer);

  viewer->stopScreenCoordinatesSystem();
  glEnable(GL_DEPTH_TEST);
}

void
GridObject::save(fstream& fout)
{
  char keyword[100];
  int len;
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "gridobjectstart");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "showpoints");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_showPoints, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_color.x;
  f[1] = m_color.y;
  f[2] = m_color.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_opacity, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "cols");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_cols, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "rows");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_rows, sizeof(int));

  int npts = m_points.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "points");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = m_points[i].x;
      f[1] = m_points[i].y;
      f[2] = m_points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "gridobjectend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
GridObject::load(fstream &fin)
{
  m_points.clear();
  m_grid.clear();

  bool done = false;
  char keyword[100];
  float f[3];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "gridobjectend") == 0)
	done = true;
      else if (strcmp(keyword, "showpoints") == 0)
	fin.read((char*)&m_showPoints, sizeof(bool));
      else if (strcmp(keyword, "color") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_color = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&m_opacity, sizeof(float));
      else if (strcmp(keyword, "cols") == 0)
	fin.read((char*)&m_cols, sizeof(int));
      else if (strcmp(keyword, "rows") == 0)
	fin.read((char*)&m_rows, sizeof(int));
      else if (strcmp(keyword, "points") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      m_points.append(Vec(f[0], f[1], f[2]));
	    }
	}
    }


  m_undo.clear();
  updateUndo();
  m_updateFlag = true;
}

void
GridObject::undo()
{
  m_undo.undo();
  m_undo.colrow(m_cols, m_rows);
  m_points = m_undo.points();
  m_updateFlag = true;
}

void
GridObject::redo()
{
  m_undo.redo();
  m_undo.colrow(m_cols, m_rows);
  m_points = m_undo.points();
  m_updateFlag = true;
}

void
GridObject::updateUndo()
{
  m_undo.append(m_cols, m_rows, m_points);
}

QList< QPair<Vec, Vec> >
GridObject::getPointsAndNormals()
{
  QList< QPair<Vec, Vec> > pn;

  for(int i=0; i<m_rows; i++)
    for(int j=0; j<m_cols; j++)
      {
	Vec vi0, vi1, vj0, vj1;
	vi0 = m_grid[qMax(0, i-1)*m_cols + j];
	vi1 = m_grid[qMin(m_rows-1, i+1)*m_cols + j];
	vj0 = m_grid[i*m_cols + qMax(0, j-1)];
	vj1 = m_grid[i*m_cols + qMin(m_cols-1, j+1)];

	Vec n = (vi1-vi0)^(vj1-vj0);

	pn.append(qMakePair(m_grid[i*m_cols+j], n.unit()));
      }

  return pn;	  
}
