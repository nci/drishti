#include "global.h"
#include "clipplane.h"
#include "staticfunctions.h"
#include "../../common/src/widgets/dcolordialog.h"
#include <QMessageBox>
#include <QInputDialog>

int ClipPlanes::count() { return m_clips.count(); }

void
ClipPlanes::setBrick0Xform(double xf[16])
{
  memcpy(m_Xform, xf, 16*sizeof(double));
  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->setXform(xf);
}

void
ClipPlanes::reset()
{
  clear();
}

ClipPlanes::ClipPlanes()
{
  m_clips.clear();
}

ClipPlanes::~ClipPlanes()
{
  clear();
}

void
ClipPlanes::setBounds(Vec dmin, Vec dmax)
{
  m_dataMin = dmin;
  m_dataMax = dmax;
}

void
ClipPlanes::show()
{
  for(int i=0; i<m_clips.count(); i++)
    {
      m_clips[i]->setShow(true);
      m_clips[i]->addInMouseGrabberPool();
    }
}

void
ClipPlanes::hide()
{
  for(int i=0; i<m_clips.count(); i++)
    {
      m_clips[i]->setShow(false);
      m_clips[i]->removeFromMouseGrabberPool();
    }
}

bool
ClipPlanes::show(int i)
{
  if (i >= 0 && i < m_clips.count())
    return m_clips[i]->show();
  else
    return false;
}

void
ClipPlanes::setShow(int i, bool flag)
{
  if (i >= 0 && i < m_clips.count())
    {
      m_clips[i]->setShow(flag);
      if (flag)
	m_clips[i]->addInMouseGrabberPool();
      else
	m_clips[i]->removeFromMouseGrabberPool();
    }
}

void 
ClipPlanes::clear()
{
  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->disconnect();

  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_clips.count(); i++)
    delete m_clips[i];

  m_clips.clear();
}

bool
ClipPlanes::isInMouseGrabberPool(int i)
{
  if (i < m_clips.count())
    return m_clips[i]->isInMouseGrabberPool();
  else
    return false;
}
void
ClipPlanes::addInMouseGrabberPool(int i)
{
  if (i < m_clips.count())
    m_clips[i]->addInMouseGrabberPool();
}
void
ClipPlanes::addInMouseGrabberPool()
{
  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->addInMouseGrabberPool();
}
void
ClipPlanes::removeFromMouseGrabberPool(int i)
{
  if (i < m_clips.count())
    m_clips[i]->removeFromMouseGrabberPool();
}

void
ClipPlanes::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->removeFromMouseGrabberPool();
}


bool
ClipPlanes::grabsMouse()
{
  for(int i=0; i<m_clips.count(); i++)
    {
      if (m_clips[i]->grabsMouse())
	return true;
    }
  return false;
}


void
ClipPlanes::addClip()
{
  ClipGrabber *cp = new ClipGrabber();
  Vec midpt = (m_dataMax+m_dataMin)/2;
  cp->setPosition(midpt);
  m_clips.append(cp);

  makeClipConnections();
  emit addClipper();
}

void
ClipPlanes::addClip(Vec p0, Vec p1, Vec p2)
{  
  ClipGrabber *cp = new ClipGrabber();

  Vec midpt = (p0+p1+p2)/3;
  cp->setPosition(midpt);

  Vec ax = (p0-p1).unit();
  Vec bx = (p2-p1).unit();
  Vec cx = cross(ax, bx);
  bx = cross(cx, ax);

  Quaternion q;
  q.setFromRotatedBasis(ax, bx, cx);
  cp->setOrientation(q);

  m_clips.append(cp);

  makeClipConnections();
  emit addClipper();
}

void
ClipPlanes::setClip(int i, Vec p0, Vec p1, Vec p2)
{  
  ClipGrabber *cp;

  if (m_clips.count() <= i)
    {
      cp = new ClipGrabber();
      m_clips.append(cp);
    }
  else
    cp = m_clips[i];
  
  Vec midpt = (p0+p1+p2)/3;
  cp->setPosition(midpt);

  Vec ax = (p0-p1).unit();
  Vec bx = (p2-p1).unit();
  Vec cx = cross(ax, bx);
  bx = cross(cx, ax);

  Quaternion q;
  q.setFromRotatedBasis(ax, bx, cx);
  cp->setOrientation(q);
}

QList<int>
ClipPlanes::tfset()
{
  QList<int> tf;
  for(int i=0; i<m_clips.count(); i++)
    tf.append(m_clips[i]->tfset());
  return tf;
}

QList<bool>
ClipPlanes::showSlice()
{
  QList<bool> ss;
  for(int i=0; i<m_clips.count(); i++)
    ss.append(m_clips[i]->showSlice());
  return ss;
}

QList<bool>
ClipPlanes::showOtherSlice()
{
  QList<bool> ss;
  for(int i=0; i<m_clips.count(); i++)
    ss.append(m_clips[i]->showOtherSlice());
  return ss;
}

QList<QVector4D>
ClipPlanes::viewport()
{
  QList<QVector4D> vp;
  for(int i=0; i<m_clips.count(); i++)
    vp.append(m_clips[i]->viewport());
  return vp;
}

QList<bool>
ClipPlanes::viewportType()
{
  QList<bool> vp;
  for(int i=0; i<m_clips.count(); i++)
    vp.append(m_clips[i]->viewportType());
  return vp;
}

QList<float>
ClipPlanes::viewportScale()
{
  QList<float> vp;
  for(int i=0; i<m_clips.count(); i++)
    vp.append(m_clips[i]->viewportScale());
  return vp;
}

QList<int>
ClipPlanes::thickness()
{
  QList<int> vp;
  for(int i=0; i<m_clips.count(); i++)
    vp.append(m_clips[i]->thickness());
  return vp;
}

QList<bool>
ClipPlanes::applyClip()
{
  QList<bool> vp;
  for(int i=0; i<m_clips.count(); i++)
    vp.append(m_clips[i]->apply());
  return vp;
}

QList<Vec>
ClipPlanes::positions()
{
  QList<Vec> pos;
  for(int i=0; i<m_clips.count(); i++)
    pos.append(m_clips[i]->position());
  return pos;
}

QList<Vec>
ClipPlanes::normals()
{
  QList<Vec> normals;
  for(int i=0; i<m_clips.count(); i++)
    {
      Vec tang = m_clips[i]->m_tang;
      //Vec voxelScaling = Global::voxelScaling();
      //tang = VECPRODUCT(tang, voxelScaling);
      //tang.normalize();
      normals.append(tang);
    }
  return normals;
}

QList<Vec>
ClipPlanes::xaxis()
{
  QList<Vec> xaxis;
  for(int i=0; i<m_clips.count(); i++)
    {
      Vec v = m_clips[i]->m_xaxis;
      //Vec voxelScaling = Global::voxelScaling();
      //v = VECPRODUCT(v, voxelScaling);
      //v.normalize();
      xaxis.append(v);
    }
  return xaxis;
}

QList<Vec>
ClipPlanes::yaxis()
{
  QList<Vec> yaxis;
  for(int i=0; i<m_clips.count(); i++)
    {
      Vec v = m_clips[i]->m_yaxis;
      //Vec voxelScaling = Global::voxelScaling();
      //v = VECPRODUCT(v, voxelScaling);
      //v.normalize();
      yaxis.append(v);
    }
  return yaxis;
}


void
ClipPlanes::translate(int i, Vec v)
{
  if (i>= 0 && i<m_clips.count())
    m_clips[i]->translate(v);
}

void
ClipPlanes::rotate(int i, int axis, float a)
{
  if (i>= 0 && i<m_clips.count())
    {
      if (axis == 0) m_clips[i]->rotate(m_clips[i]->m_xaxis, a);
      if (axis == 1) m_clips[i]->rotate(m_clips[i]->m_yaxis, a);
      if (axis == 2) m_clips[i]->rotate(m_clips[i]->m_tang, a);
    }
}


ClipInformation
ClipPlanes::clipInfo()
{
  ClipInformation ci;
  for(int i=0; i<m_clips.count(); i++)
    {
      ci.show.append(m_clips[i]->show());
      ci.pos.append(m_clips[i]->position());
      ci.rot.append(m_clips[i]->orientation());
      ci.solidColor.append(m_clips[i]->solidColor());
      ci.color.append(m_clips[i]->color());
      ci.applyFlip.append(m_clips[i]->flip());
      ci.apply.append(m_clips[i]->apply());
      ci.imageName.append(m_clips[i]->imageName());
      ci.imageFrame.append(m_clips[i]->imageFrame());
      ci.captionText.append(m_clips[i]->captionText());
      ci.captionFont.append(m_clips[i]->captionFont());
      ci.captionColor.append(m_clips[i]->captionColor());
      ci.captionHaloColor.append(m_clips[i]->captionHaloColor());
      ci.opacity.append(m_clips[i]->opacity());
      ci.stereo.append(m_clips[i]->stereo());
      ci.scale1.append(m_clips[i]->scale1());
      ci.scale2.append(m_clips[i]->scale2());
      ci.tfSet.append(m_clips[i]->tfset());
      ci.gridX.append(m_clips[i]->gridX());
      ci.gridY.append(m_clips[i]->gridY());
      ci.opmod.append(m_clips[i]->opmod());
      ci.viewport.append(m_clips[i]->viewport());
      ci.viewportType.append(m_clips[i]->viewportType());
      ci.viewportScale.append(m_clips[i]->viewportScale());
      ci.thickness.append(m_clips[i]->thickness());
      ci.showSlice.append(m_clips[i]->showSlice());
      ci.showOtherSlice.append(m_clips[i]->showOtherSlice());
      ci.showThickness.append(m_clips[i]->showThickness());
    }

  return ci;
}

void
ClipPlanes::set(ClipInformation ci)
{
  for (int i=0; i<m_clips.size(); i++)
    delete m_clips[i];

  m_clips.clear();
  
  for (int i=0; i<ci.pos.size(); i++)
    { 
      ClipGrabber *c = new ClipGrabber();
      c->setShow(ci.show[i]);
      c->setPosition(ci.pos[i]);
      c->setOrientation(ci.rot[i]);
      c->setSolidColor(ci.solidColor[i]);
      c->setColor(ci.color[i]);
      c->setFlip(ci.applyFlip[i]);
      c->setApply(ci.apply[i]);
      c->setImage(ci.imageName[i], ci.imageFrame[i]);
      c->setCaption(ci.captionText[i],
		    ci.captionFont[i],
		    ci.captionColor[i],
		    ci.captionHaloColor[i]);
      c->setOpacity(ci.opacity[i]);
      c->setStereo(ci.stereo[i]);
      c->setScale1(ci.scale1[i]);
      c->setScale2(ci.scale2[i]);
      c->setTFset(ci.tfSet[i]);
      c->setGridX(ci.gridX[i]);
      c->setGridY(ci.gridY[i]);
      c->setOpmod(ci.opmod[i]);
      c->setViewport(ci.viewport[i]);
      c->setViewportType(ci.viewportType[i]);
      c->setViewportScale(ci.viewportScale[i]);
      c->setThickness(ci.thickness[i]);
      c->setShowSlice(ci.showSlice[i]);
      c->setShowOtherSlice(ci.showOtherSlice[i]);
      c->setShowThickness(ci.showThickness[i]);
      m_clips.append(c);
    }

  makeClipConnections();
}


void ClipPlanes::updateScaling() { }

void
ClipPlanes::draw(QGLViewer *viewer, bool backToFront)
{
  //Vec dataSize = m_dataMax-m_dataMin+Vec(1,1,1);
  Vec dataSize = m_dataMax-m_dataMin;

  float widgetSize = 0.2*qMax(dataSize.x,
			      qMax(dataSize.y, dataSize.z));

  for(int i=0; i<m_clips.count();i++)
    m_clips[i]->draw(viewer, backToFront, widgetSize);
}

void
ClipPlanes::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_clips.count();i++)
    {
      int x,y;
      m_clips[i]->mousePosition(x,y);
      m_clips[i]->postdraw(viewer,
			   x, y,
			   m_clips[i]->grabsMouse());
    }
}

bool
ClipPlanes::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_clips.count(); i++)
    {
      if (m_clips[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_G)
	    {
	      m_clips[i]->removeFromMouseGrabberPool();
	      return true;
	    }

	  if (event->key() == Qt::Key_Up || 
	      event->key() == Qt::Key_Down ||
	      event->key() == Qt::Key_Left || 
	      event->key() == Qt::Key_Right)
	    {
	      int shift = -1;
	      if (event->key() == Qt::Key_Up ||
		  event->key() == Qt::Key_Right)
		shift = 1;

	      if (event->modifiers() & Qt::ShiftModifier)
		shift *= 10;

	      if (event->key() == Qt::Key_Left || 
		  event->key() == Qt::Key_Right)
		{
		  if (m_clips[i]->moveAxis() < ClipObject::MoveY0)
		    m_clips[i]->rotate(m_clips[i]->m_xaxis, shift);
		  else if (m_clips[i]->moveAxis() < ClipObject::MoveZ)
		    m_clips[i]->rotate(m_clips[i]->m_yaxis, shift);
		  else
		    m_clips[i]->rotate(m_clips[i]->m_tang, shift);
		}
	      else
		{
		  if (m_clips[i]->moveAxis() < ClipObject::MoveY0)
		    m_clips[i]->translate(shift*m_clips[i]->m_xaxis);
		  else if (m_clips[i]->moveAxis() < ClipObject::MoveZ)
		    m_clips[i]->translate(shift*m_clips[i]->m_yaxis);
		  else
		    m_clips[i]->translate(shift*m_clips[i]->m_tang);
		}
	      return true;
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_clips[i]->removeFromMouseGrabberPool();
	      m_clips.removeAt(i);
	      emit removeClipper(i);
	      return true;
	    }
	  else if (event->key() == Qt::Key_V)
	    {
	      setShow(i, false);
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_clips[i]->setMoveAxis(ClipGrabber::MoveX0);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y &&
		   !(event->modifiers() & Qt::ControlModifier ||
		     event->modifiers() & Qt::MetaModifier) )
	    {
	      m_clips[i]->setMoveAxis(ClipGrabber::MoveY0);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z &&
		   !(event->modifiers() & Qt::ControlModifier ||
		     event->modifiers() & Qt::MetaModifier) )
	    {
	      m_clips[i]->setMoveAxis(ClipGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_C)
	    {
	      Vec clr = m_clips[i]->color();
	      QColor dcolor = QColor::fromRgbF(clr.x,
					       clr.y,
					       clr.z);
	      QColor color = DColorDialog::getColor(dcolor);
	      if (color.isValid())
		{
		  float r = color.redF();
		  float g = color.greenF();
		  float b = color.blueF();
		  m_clips[i]->setColor(Vec(r,g,b));
		}
	      return true;
	    }
	  else if (event->key() == Qt::Key_O)
	    {
	      float op = m_clips[i]->opacity();
	      op = QInputDialog::getDouble(0,
					   QWidget::tr("Clip plane opacity"),
					   QWidget::tr("Opacity"),
					   op, 0.0, 1.0,
					   1, NULL, Qt::WindowFlags(),
					   0.1);
	      m_clips[i]->setOpacity(op);
	      return true;
	    }
	}
    }


  if (event->key() == Qt::Key_C &&
      !(event->modifiers() & Qt::ControlModifier ||
	event->modifiers() & Qt::MetaModifier) )
    {
      addClip();
      return true;
    }

  
  return false;
}

void
ClipPlanes::makeClipConnections()
{
  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->disconnect();

  for(int i=0; i<m_clips.count(); i++)
    {
      connect(m_clips[i], SIGNAL(selectForEditing()),
	      this, SLOT(selectForEditing()));

      connect(m_clips[i], SIGNAL(deselectForEditing()),
	      this, SLOT(deselectForEditing()));
    }
}
void
ClipPlanes::deselectForEditing()
{
  for(int i=0; i<m_clips.count(); i++)
    m_clips[i]->setActive(false);
}

void
ClipPlanes::selectForEditing()
{
  int idx = -1;
  for(int i=0; i<m_clips.count(); i++)
    {
      if (m_clips[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }
  if (idx == -1)
    return;

  m_clips[idx]->setActive(true);
}

void ClipPlanes::processCommand(int idx, QString cmd) { }



void
ClipPlanes::drawPoints(int ic, QList<Vec> hpts)
{
  Vec voxelScaling = Global::voxelScaling();
  Vec pos = VECPRODUCT(m_clips[ic]->position(), voxelScaling);
  Vec cnormal = m_clips[ic]->m_tang;
  int clipThickness = m_clips[ic]->thickness();

  glEnable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
  glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POINT_SMOOTH);
  glBegin(GL_POINTS);
  for(int ip=0; ip<hpts.count(); ip++)
    {
      Vec pt = VECPRODUCT(hpts[ip], voxelScaling);
      if (qAbs((pt-pos)*cnormal)<(clipThickness+1.0))
	glVertex3f(pt.x, pt.y, pt.z);
    }
  glEnd();
  glDisable(GL_POINT_SPRITE);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_POINT_SMOOTH);
}
