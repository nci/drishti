#include "global.h"
#include "paths.h"
#include "staticfunctions.h"
#include "propertyeditor.h"
#include "captiondialog.h"
#include "shaderfactory.h"

#include <QFileDialog>

int Paths::count() { return m_paths.count(); }

Paths::Paths()
{
  m_sameForAll = true;
  m_paths.clear();
}

Paths::~Paths()
{
  clear();
}
void 
Paths::clear()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->disconnect();

  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->removeFromMouseGrabberPool();

  for(int i=0; i<m_paths.count(); i++)
    delete m_paths[i];

  m_paths.clear();
}

bool
Paths::isInMouseGrabberPool(int i)
{
  if (i < m_paths.count())
    return m_paths[i]->isInMouseGrabberPool();
  else
    return false;
}
void
Paths::addInMouseGrabberPool(int i)
{
  if (i < m_paths.count())
    m_paths[i]->addInMouseGrabberPool();
}
void
Paths::addInMouseGrabberPool()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->addInMouseGrabberPool();
}
void
Paths::removeFromMouseGrabberPool(int i)
{
  if (i < m_paths.count())
    m_paths[i]->removeFromMouseGrabberPool();
}

void
Paths::removeFromMouseGrabberPool()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->removeFromMouseGrabberPool();
}


bool
Paths::grabsMouse()
{
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->grabsMouse())
	return true;
    }
  return false;
}


void
Paths::addPath(QList<Vec> pts)
{
  PathGrabber *pg = new PathGrabber();
  pg->setPoints(pts);
  m_paths.append(pg);

  makePathConnections();
}

void
Paths::addPath(PathObject po)
{
  PathGrabber *pg = new PathGrabber();
  pg->set(po);
  m_paths.append(pg);

  makePathConnections();
}

void
Paths::addIndexedPath(QString flnm)
{
  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine(); // ignore first line

  QList<Vec> pts;
  // first read all points
  while (! fd.atEnd())
    {
      line = fd.readLine(); // ignore first line
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 1)
	{
	  int npts = list[0].toInt();
	  for(int i=0; i<npts; i++)
	    {
	      if (fd.atEnd())
		break;
	      else
		{
		  line = fd.readLine();
		  QStringList list = line.split(" ", QString::SkipEmptyParts);
		  if (list.count() == 3)
		    {
		      float x = list[0].toFloat();
		      float y = list[1].toFloat();
		      float z = list[2].toFloat();
		      pts.append(Vec(x,y,z));
		    }
		}
	    }
	  break;
	}
    }

  // now read indices
  while (! fd.atEnd())
    {
      line = fd.readLine();
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() > 1)
	{
	  QList<Vec> ipts;
	  for(int i=0; i<list.count(); i++)
	    {
	      int j = list[i].toInt();
	      if (j>=0 && j<pts.count())
		ipts.append(pts[j]);
	    }
	  if (ipts.count() > 0)
	    {
	      PathGrabber *pg = new PathGrabber();
	      pg->setPoints(ipts);
	      m_paths.append(pg);
	    }
	}
    }

  makePathConnections();
  
}

void
Paths::addFibers(QString flnm)
{
  uchar *tagColors = Global::tagColors();

  QFile cfile;
  cfile.setFileName(flnm);
  if (!cfile.open(QFile::ReadOnly | QIODevice::Text))
    {
      QMessageBox::information(0, "Error", QString("Cannot read to %1").arg(flnm));
      return;
    }

  QTextStream in(&cfile);
  while (!in.atEnd())
    {
      QString line = in.readLine();
      if (line.contains("fiberstart"))
	{
	  int tag = 1;
	  int thickness = 1;
	  QList<Vec> pts;
	  bool done = false;
	  while(!done && !in.atEnd())
	    {
	      line = in.readLine();
	      QStringList words = line.split(" ", QString::SkipEmptyParts);
	      if (words.count() > 0)
		{
		  int t;
		  bool b;
		  if (words[0].contains("fiberend"))
		    done = true;
		  else if (words[0].contains("tag"))
		    {		  
		      if (words.count() > 1)
			tag = words[1].toInt();;
		    }
		  else if (words[0].contains("thickness"))
		    {
		      if (words.count() > 1)
			thickness = words[1].toInt();;
		    }
		  else if (words[0].contains("seeds"))
		    {
		      int npts;
		      if (words.count() > 1)
			npts = words[1].toInt();;
		      for(int ni=0; ni<npts; ni++)
			{
			  line = in.readLine();
			  QStringList words = line.split(" ", QString::SkipEmptyParts);
			  if (words.count() == 3)
			    pts << Vec(words[0].toFloat(),
				       words[1].toFloat(),
				       words[2].toFloat());
			}
		    }
		}
	    }
	  if (pts.count() > 0)
	    {
	      PathGrabber *pg = new PathGrabber();
	      pg->setPoints(pts);
	      Vec color = Vec((float)tagColors[4*tag+0]/255.0,
			      (float)tagColors[4*tag+1]/255.0,
			      (float)tagColors[4*tag+2]/255.0);
	      pg->setColor(color);
	      pg->setRadX(0, thickness/2, true);
	      pg->setRadY(0, thickness/2, true);
	      pg->setSegments(1);
	      pg->setSections(20);
	      pg->setCapType(1);
	      pg->setTube(true);
	      pg->setShowPoints(false);
	      m_paths.append(pg);
	    }
	}
    }

  makePathConnections();
}

void
Paths::addPath(QString flnm)
{
  QFile fpath(flnm);
  fpath.open(QFile::ReadOnly);
  QTextStream fd(&fpath);

  QString line = fd.readLine();
  if (line.contains("#indexed", Qt::CaseInsensitive))
    {
      addIndexedPath(flnm);
      return;
    }


  while (! fd.atEnd())
    {
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      if (list.count() == 1)
	{
	  int npts = list[0].toInt();
	  QList<Vec> pts;
	  for(int i=0; i<npts; i++)
	    {
	      if (fd.atEnd())
		break;
	      else
		{
		  QString line = fd.readLine();
		  QStringList list = line.split(" ", QString::SkipEmptyParts);
		  if (list.count() == 3)
		    {
		      float x = list[0].toFloat();
		      float y = list[1].toFloat();
		      float z = list[2].toFloat();
		      pts.append(Vec(x,y,z));
		    }
		}
	    }
	 
	  if (pts.count() > 0)
	    {
	      PathGrabber *pg = new PathGrabber();
	      pg->setPoints(pts);
	      m_paths.append(pg);
	    }	  
	}
      line = fd.readLine();
    }

  makePathConnections();
}

QList<PathObject>
Paths::paths()
{
  QList<PathObject> po;
  for(int i=0; i<m_paths.count(); i++)
    po.append(m_paths[i]->get());

  return po;
}

void
Paths::setPaths(QList<PathObject> po)
{
  clear();

  for(int i=0; i<po.count(); i++)
    {
      PathGrabber *pg = new PathGrabber();
      pg->set(po[i]);
      m_paths.append(pg);
    }

  makePathConnections();
}

void
Paths::updateScaling()
{
  for(int i=0; i<m_paths.count();i++)
    m_paths[i]->computePathLength();
}

void
Paths::draw(QGLViewer *viewer,
	    Vec pn, float pnear, float pfar,
	    bool backToFront, Vec lightVec)
{
  for(int i=0; i<m_paths.count();i++)
    {
      m_paths[i]->draw(viewer,
		       pn, pnear, pfar,
		       m_paths[i]->grabsMouse(),
		       backToFront,
		       lightVec,
		       ShaderFactory::ptShader(),
		       ShaderFactory::ptShaderParm(),
		       ShaderFactory::pnShader(),
		       ShaderFactory::pnShaderParm());
    }
}

void
Paths::postdraw(QGLViewer *viewer)
{
  for(int i=0; i<m_paths.count();i++)
    {
      int x,y;
      m_paths[i]->mousePosition(x,y);
      m_paths[i]->postdraw(viewer,
			   x, y,
			   m_paths[i]->grabsMouse());
    }
}

void
Paths::postdrawInViewport(QGLViewer *viewer,
			  QVector4D vp, Vec cp, Vec cn, int ct)
{
  for(int i=0; i<m_paths.count();i++)
    {
      int x,y;
      m_paths[i]->mousePosition(x,y);
      float textscale = 1.0/qMax(0.01f, qMin(vp.z(), vp.w()));
      m_paths[i]->postdrawInViewport(viewer,
				     x, y,
				     m_paths[i]->grabsMouse(),
				     cp, cn, ct, textscale);
    }
}

bool
Paths::keyPressEvent(QKeyEvent *event)
{
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->grabsMouse())
	{
	  if (event->key() == Qt::Key_C)
	    {
	      bool b = m_paths[i]->closed();
	      m_paths[i]->setClosed(!b);
	      return true;
	    }
	  else if (event->key() == Qt::Key_F)
	    {
	      m_paths[i]->flip();
	      return true;
	    }
	  else if (event->key() == Qt::Key_G)
	    {
	      m_paths[i]->removeFromMouseGrabberPool();
	      return true;
	    }
	  else if (event->key() == Qt::Key_P)
	    {
	      bool b = m_paths[i]->showPoints();
	      m_paths[i]->setShowPoints(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_N)
	    {
	      bool b = m_paths[i]->showPointNumbers();
	      m_paths[i]->setShowPointNumbers(!b);    
	      return true;
	    }
	  else if (event->key() == Qt::Key_L)
	    {
	      if (event->modifiers() & Qt::ShiftModifier)
		{
		  m_paths[i]->setCaptionLabel(!m_paths[i]->captionLabel());
		}
	      else
		{
		  bool b = m_paths[i]->showLength();
		  m_paths[i]->setShowLength(!b);    
		}
	      return true;
	    }
	  else if (event->key() == Qt::Key_X)
	    {
	      m_paths[i]->setMoveAxis(PathGrabber::MoveX);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Y)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_paths[i]->redo();
	      else
		m_paths[i]->setMoveAxis(PathGrabber::MoveY);
	      return true;
	    }
	  else if (event->key() == Qt::Key_Z)
	    {
	      if (event->modifiers() & Qt::ControlModifier ||
		  event->modifiers() & Qt::MetaModifier)
		m_paths[i]->undo();
	      else
		m_paths[i]->setMoveAxis(PathGrabber::MoveZ);
	      return true;
	    }
	  else if (event->key() == Qt::Key_W)
	    {
	      m_paths[i]->setMoveAxis(PathGrabber::MoveAll);
	      return true;
	    }
	  else if (event->key() == Qt::Key_S)
	    {
	      int idx = m_paths[i]->pointPressed();
	      if (idx > -1)
		{
		  float radx = m_paths[i]->getRadX(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    radx--;
		  else
		    radx++;
		  radx = qMax(1.0f, radx);
		  m_paths[i]->setRadX(idx, radx, m_sameForAll);
		  return true;
		}
	    }
	  else if (event->key() == Qt::Key_T)
	    {
	      int idx = m_paths[i]->pointPressed();
	      if (idx > -1)
		{
		  float rady = m_paths[i]->getRadY(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    rady--;
		  else
		    rady++;
		  rady = qMax(1.0f, rady);
		  m_paths[i]->setRadY(idx, rady, m_sameForAll);
		}
	      else // switch to tube mode
		{
		  if (!(event->modifiers() & Qt::ShiftModifier))
		    {
		      bool b = m_paths[i]->tube();
		      m_paths[i]->setTube(!b);
		    }
		  else
		    loadCaption(i);
		}

	      return true;
	    }
	  else if (event->key() == Qt::Key_A)
	    {
	      int idx = m_paths[i]->pointPressed();
	      if (idx > -1)
		{
		  float a = m_paths[i]->getAngle(idx);
		  if (event->modifiers() & Qt::ShiftModifier)
		    a--;
		  else
		    a++;
		  m_paths[i]->setAngle(idx, a, m_sameForAll);
		  return true;
		}
	      else
		{
		  bool b = m_paths[i]->showAngle();
		  m_paths[i]->setShowAngle(!b);    
		  return true;
		}
		
	    }
	  else if (event->key() == Qt::Key_Delete ||
		   event->key() == Qt::Key_Backspace ||
		   event->key() == Qt::Key_Backtab)
	    {
	      m_paths[i]->removeFromMouseGrabberPool();
	      m_paths.removeAt(i);
	      return true;
	    }
	  
	  if (event->key() == Qt::Key_Space)
	    return openPropertyEditor(i);
	}
    }
  
  return true;
}

bool
Paths::openPropertyEditor(int i)
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_paths[i]->opacity());
  vlist << QVariant(0.0);
  vlist << QVariant(1.0);
  vlist << QVariant(0.1); // singlestep
  vlist << QVariant(1); // decimals
  plist["opacity"] = vlist;
  
  vlist.clear();
  vlist << QVariant("color");
  Vec pcolor = m_paths[i]->color();
  QColor dcolor = QColor::fromRgbF(pcolor.x,
				   pcolor.y,
				   pcolor.z);
  vlist << dcolor;
  plist["color"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_paths[i]->segments());
  vlist << QVariant(1);
  vlist << QVariant(100);
  plist["smoothness"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_paths[i]->sections());
  vlist << QVariant(1);
  vlist << QVariant(100);
  plist["sections"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->halfSection());
  plist["half section"] = vlist;
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << QVariant(m_paths[i]->capType());
  vlist << QVariant("flat");
  vlist << QVariant("round");
  vlist << QVariant("arrow");
  plist["cap style"] = vlist;
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << QVariant(m_paths[i]->arrowDirection());
  vlist << QVariant("forward");
  vlist << QVariant("backward");
  plist["arrow direction"] = vlist;
  
  vlist.clear();
  vlist << QVariant("double");
  vlist << QVariant(m_paths[i]->arrowHeadLength());
  vlist << QVariant(0.0);
  vlist << QVariant(1000.0);
  vlist << QVariant(1.0); // singlestep
  vlist << QVariant(1); // decimals
  plist["arrow length"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->arrowForAll());
  plist["arrows for all"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_sameForAll);
  plist["same for all"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->showAngle());
  plist["display angle"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->showLength());
  plist["display path length"] = vlist;
  
  vlist.clear();
  vlist << QVariant("color");
  pcolor = m_paths[i]->lengthColor();
  dcolor = QColor::fromRgbF(pcolor.x,
			    pcolor.y,
			    pcolor.z);
  vlist << dcolor;
  plist["text color"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_paths[i]->lengthTextDistance());
  vlist << QVariant(-1000);
  vlist << QVariant(1000);
  plist["length text distance"] = vlist;
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << QVariant(m_paths[i]->useType());
  vlist << QVariant("");
  vlist << QVariant("carve");
  vlist << QVariant("half");
  vlist << QVariant("slab");
  vlist << QVariant("half slab");
  vlist << QVariant("blend");
  vlist << QVariant("half blend");
  vlist << QVariant("slab blend");
  vlist << QVariant("half slab blend");
  plist["use for"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->keepInside());
  plist["keep inside"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->keepEnds());
  plist["keep ends"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_paths[i]->blendTF());
  vlist << QVariant(0);
  vlist << QVariant(15);
  plist["tfset"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->allowInterpolate());
  plist["interpolate"] = vlist;
  
  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(m_paths[i]->add());
  plist["continuous add"] = vlist;
  
  QVector4D vp = m_paths[i]->viewport();
  QString vpstr = QString("%1 %2 %3 %4").  \
    arg(vp.x()).			   \
    arg(vp.y()).			   \
    arg(vp.z()).			   \
    arg(vp.w()); 
  vlist.clear();
  vlist << QVariant("string");
  vlist << QVariant(vpstr);
  plist["viewport"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(m_paths[i]->viewportTF());
  vlist << QVariant(-1);
  vlist << QVariant(15);
  plist["viewport tfset"] = vlist;
  
  vlist.clear();
  vlist.clear();
  vlist << QVariant("combobox");
  if (m_paths[i]->viewportStyle())
    vlist << QVariant(0);
  else
    vlist << QVariant(1);
  vlist << QVariant("horizontal");
  vlist << QVariant("vertical");
  plist["viewport style"] = vlist;
  
  vlist.clear();
  plist["command"] = vlist;
  
  
  vlist.clear();
  QFile helpFile(":/paths.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }	      
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "continuous add";
  keys << "gap";
  keys << "color";
  keys << "opacity";
  keys << "gap";
  keys << "smoothness";
  keys << "sections";
  keys << "half section";
  keys << "gap";
  keys << "cap style";
  keys << "arrow direction";
  keys << "arrows for all";
  keys << "arrow length";
  keys << "gap";
  keys << "display angle";
  keys << "display path length";
  keys << "text color";
  keys << "length text distance";
  keys << "gap";
  keys << "same for all";
  keys << "gap";
  keys << "gap";
  keys << "use for";
  keys << "keep inside";
  keys << "keep ends";
  keys << "tfset";
  keys << "interpolate";
  keys << "gap";
  keys << "viewport";
  keys << "viewport tfset";
  keys << "viewport style";
  keys << "command";
  keys << "commandhelp";
  
  
  propertyEditor.set("Path Parameters", plist, keys);
  
  
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return true;
  
  keys = vmap.keys();
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
      
      
      if (pair.second)
	{
	  if (keys[ik] == "continuous add")
	    {
	      // reset "continuous add" for paths first
	      for(int p=0; p<m_paths.count(); p++)
		m_paths[p]->setAdd(false);
	      // now set "continuous add" for required path
	      m_paths[i]->setAdd(pair.first.toBool());
	    }
	  else if (keys[ik] == "color")
	    {
	      QColor color = pair.first.value<QColor>();
	      float r = color.redF();
	      float g = color.greenF();
	      float b = color.blueF();
	      pcolor = Vec(r,g,b);
	      m_paths[i]->setColor(pcolor);
	    }
	  else if (keys[ik] == "text color")
	    {
	      QColor color = pair.first.value<QColor>();
	      float r = color.redF();
	      float g = color.greenF();
	      float b = color.blueF();
	      pcolor = Vec(r,g,b);
	      m_paths[i]->setLengthColor(pcolor);
	    }
	  else if (keys[ik] == "opacity")
	    m_paths[i]->setOpacity(pair.first.toDouble());
	  else if (keys[ik] == "sections")
	    m_paths[i]->setSections(pair.first.toInt());
	  else if (keys[ik] == "smoothness")
	    m_paths[i]->setSegments(pair.first.toInt());
	  else if (keys[ik] == "half section")
	    m_paths[i]->setHalfSection(pair.first.toBool());
	  else if (keys[ik] == "cap style")
	    m_paths[i]->setCapType(pair.first.toInt());
	  else if (keys[ik] == "arrow direction")
	    m_paths[i]->setArrowDirection(pair.first.toInt());
	  else if (keys[ik] == "arrows for all")
	    m_paths[i]->setArrowForAll(pair.first.toBool());
	  else if (keys[ik] == "arrow length")
	    m_paths[i]->setArrowHeadLength(pair.first.toDouble());
	  else if (keys[ik] == "display path length")
	    m_paths[i]->setShowLength(pair.first.toBool());
	  else if (keys[ik] == "display angle")
	    m_paths[i]->setShowAngle(pair.first.toBool());
	  else if (keys[ik] == "length text distance")
	    m_paths[i]->setLengthTextDistance(pair.first.toInt());
	  else if (keys[ik] == "same for all")
	    {
	      m_sameForAll = pair.first.toBool();
	      m_paths[i]->setSameForAll(m_sameForAll);
	    }
	  else if (keys[ik] == "use for")
	    m_paths[i]->setUseType(pair.first.toInt());
	  else if (keys[ik] == "keep inside")
	    m_paths[i]->setKeepInside(pair.first.toBool());
	  else if (keys[ik] == "keep ends")
	    m_paths[i]->setKeepEnds(pair.first.toBool());
	  else if (keys[ik] == "tfset")
	    m_paths[i]->setBlendTF(pair.first.toInt());
	  else if (keys[ik] == "interpolate")
	    m_paths[i]->setAllowInterpolate(pair.first.toBool());
	  else if (keys[ik] == "viewport tfset")
	    m_paths[i]->setViewportTF(pair.first.toInt());
	  else if (keys[ik] == "viewport style")
	    m_paths[i]->setViewportStyle(pair.first.toInt() == 0);
	  else if (keys[ik] == "viewport")
	    {
	      vpstr = pair.first.toString();
	      QStringList list = vpstr.split(" ", QString::SkipEmptyParts);
	      QVector4D vp = m_paths[i]->viewport();
	      if (list.count() == 4)
		{
		  float x = list[0].toFloat();
		  float y = list[1].toFloat();
		  float z = list[2].toFloat();
		  float w = list[3].toFloat();
		  if (x < 0.0f || x > 1.0f ||
		      y < 0.0f || y > 1.0f ||
		      z < 0.0f || z > 1.0f ||
		      w < 0.0f || w > 1.0f)
		    QMessageBox::information(0, "",
					     QString("Values for viewport must be between 0.0 and 1.0 : %1 %2 %3 %4"). \
					     arg(x).arg(y).arg(z).arg(w));
		  else
		    vp = QVector4D(x,y,z,w);
		}
	      else if (list.count() == 3)
		{
		  float x = list[0].toFloat();
		  float y = list[1].toFloat();
		  float z = list[2].toFloat();
		  if (x < 0.0f || x > 1.0f ||
		      y < 0.0f || y > 1.0f ||
		      z < 0.0f || z > 1.0f)
		    QMessageBox::information(0, "",
					     QString("Values for viewport must be between 0.0 and 1.0 : %1 %2 %3"). \
					     arg(x).arg(y).arg(z));
		  else
		    vp = QVector4D(x,y,z,z);
		}
	      else if (list.count() == 2)
		{
		  float x = list[0].toFloat();
		  float y = list[1].toFloat();
		  if (x < 0.0f || x > 1.0f ||
		      y < 0.0f || y > 1.0f)
		    QMessageBox::information(0, "",
					     QString("Values for viewport must be between 0.0 and 1.0 : %1 %2"). \
					     arg(x).arg(y));
		  else
		    vp = QVector4D(x,y,1.0,0.5);
		}
	      else
		{
		  QMessageBox::information(0, "", "Switching off the viewport");
		  vp = QVector4D(-1,-1,-1,-1);
		}
	      
	      m_paths[i]->setViewport(vp);
	    }
	}
    }
  
  QString cmd = propertyEditor.getCommandString();
  if (!cmd.isEmpty())
    processCommand(i, cmd);	
  
  emit updateGL();
  return true;
}

void
Paths::makePathConnections()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->disconnect();

  for(int i=0; i<m_paths.count(); i++)
    {
      connect(m_paths[i], SIGNAL(selectForEditing(int, int)),
	      this, SLOT(selectForEditing(int, int)));

      connect(m_paths[i], SIGNAL(deselectForEditing()),
	      this, SLOT(deselectForEditing()));
    }
}
void
Paths::deselectForEditing()
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->setPointPressed(-1);
}

void
Paths::selectForEditing(int mouseButton,
			int p0)
{
  int idx = -1;
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->grabsMouse())
	{
	  idx = i;
	  break;
	}
    }
  if (idx == -1)
    return;

  if (mouseButton == Qt::RightButton)
    {
      if ((m_paths[idx]->points()).count() <= 2)
	emit showMessage("Sorry cannot remove current point. Must have atleast 2 points in a path", true);
      else if (m_paths[idx]->pointPressed() == -1)
	  emit showMessage("No point selected for removal", true);
      else
	m_paths[idx]->removePoint(m_paths[idx]->pointPressed());
    }
  else if (mouseButton == Qt::LeftButton && p0 > -1)
    m_paths[idx]->insertPointAfter(p0);
//  else
//    m_paths[idx]->setPointPressed(m_paths[idx]->pointPressed());
}

void
Paths::processCommand(int idx, QString cmd)
{
  bool ok;
  QString origCmd = cmd;
  cmd = cmd.toLower();
  QStringList list = cmd.split(" ", QString::SkipEmptyParts);
  
  int li = 0;
  while (li < list.count())
    {
      if (list[li] == "planar")
	{
	  if (list.count()-li > 3)
	    {
	      int v0 = list[li+1].toInt(&ok);
	      int v1 = list[li+2].toInt(&ok);
	      int v2 = list[li+3].toInt(&ok);
	      m_paths[idx]->makePlanar(v0,v1,v2);
	      li += 3;
	    }
	  else
	    m_paths[idx]->makePlanar();
	}
      else if (list[li] == "circle")
	{
	  m_paths[idx]->makeCircle();
	}
      else if (list[li] == "resample")
	{
	  int npts = -1;
	  if (list.count()-li > 1)
	    {
	      npts = list[li+1].toInt();
	      li += 1;
	    }	  
	  m_paths[idx]->makeEquidistant(npts);
	}
      else if (list[li] == "normalize" ||
	  list[li] == "normalise")
	{
	  m_paths[idx]->normalize();
	}
      else if (list[li] == "setradius" ||
	       list[li] == "radius" ||
	       list[li] == "height")
	{
	  if (list.count()-li > 1)
	    {
	      float rad = list[li+1].toFloat(&ok);
	      m_paths[idx]->setRadX(0, rad, true);
	      m_paths[idx]->setRadY(0, rad, true);
	      li++;
	    }
	  else
	    QMessageBox::information(0, "Path Command Error",
				     "No radius given");
	}
      else if (list[li] == "setrads" || list[li] == "rads")
	{
	  if (list.count()-li > 1)
	    {
	      if (list.count()-li > 2)
		{
		  int pidx = list[li+1].toInt(&ok);
		  float radx = list[li+2].toFloat(&ok);
		  m_paths[idx]->setRadX(pidx, radx, false);
		  li+=2;
		}
	      else
		{
		  float radx = list[li+1].toFloat(&ok);
		  m_paths[idx]->setRadX(0, radx, true);
		  li++;
		}
	    }
	  else
	    QMessageBox::information(0, "Path Command Error",
				     "No S radius given");
	}
      else if (list[li] == "setradt" || list[li] == "radt")
	{
	  if (list.count()-li > 1)
	    {
	      if (list.count()-li > 2)
		{
		  int pidx = list[li+1].toInt(&ok);
		  float rady = list[li+2].toFloat(&ok);
		  m_paths[idx]->setRadY(pidx, rady, false);
		  li+=2;
		}
	      else
		{
		  float rady = list[li+1].toFloat(&ok);
		  m_paths[idx]->setRadY(0, rady, true);
		  li++;
		}
	    }
	  else
	    QMessageBox::information(0, "Path Command Error",
				     "No T radius given");
	}
      else if (list[li] == "setangle" || list[li] == "angle")
	{
	  if (list.count()-li > 1)
	    {
	      if (list.count()-li > 2)
		{
		  int pidx = list[li+1].toInt(&ok);
		  float a = list[li+2].toFloat(&ok);
		  m_paths[idx]->setAngle(pidx, a, false);
		  li+=2;
		}
	      else
		{
		  float a = list[li+1].toFloat(&ok);
		  m_paths[idx]->setAngle(0, a, true);
		  li++;
		}
	    }
	  else
	    QMessageBox::information(0, "Path Command Error",
				     "No angle given");
	}
      else if (list[li] == "showprofile")
	{
	  int radius = 0;
	  if (list.size()-li > 1)
	    {
	      radius = list[li+1].toInt(&ok);
	      radius = qMin(radius, (int)200);
	      li++;
	    }

	  int segments = m_paths[idx]->segments();
	  QList<Vec> pathPoints = m_paths[idx]->getPointPath();
	  emit showProfile(segments, radius, pathPoints);
	}
      else if (list[li] == "showthicknessprofile")
	{
	  int searchType = 0;
	  if (list.size()-li > 1)
	    {
	      searchType = qBound(0, list[li+1].toInt(&ok), 1);
	      li++;
	    }
	  int segments = m_paths[idx]->segments();
	  QList< QPair<Vec, Vec> > pathPointNormals = m_paths[idx]->getPointAndNormalPath();
	  emit showThicknessProfile(searchType, segments, pathPointNormals);
	}
      else if (list[li] == "mop")
	{
	  if (list.size()-li > 1)
	    {
	      QList<Vec> ppoints = m_paths[idx]->getPointPath();
	      if (m_paths[idx]->closed())
		ppoints.removeLast();

	      QList<Vec> pathPoints;
	      Vec voxelScaling = Global::voxelScaling();
	      for (int i=0; i<ppoints.count(); i++)
		{
		  Vec pt = VECPRODUCT(ppoints[i], voxelScaling);
		  pathPoints << pt;
		}

	      if (list[li+1] == "patch")
		{
		  li++;
		  int thick=1;
		  int val = 255;
		  if (list.size()-li > 1) thick = list[li+1].toInt(&ok);
		  if (list.size()-li > 2) val = list[li+2].toInt(&ok);

		  emit fillPathPatch(pathPoints, thick, val);
		  return;
		}
	      else if (list[li+1] == "paintpatch")
		{
		  li++;
		  int thick=1;
		  int tag = 0;
		  if (list.size()-li > 1) thick = list[li+1].toInt(&ok);
		  if (list.size()-li > 2) tag = list[li+2].toInt(&ok);

		  emit paintPathPatch(pathPoints, thick, tag);
		  return;
		}
	      else
		{
		  int docarve = 1;
		  if (list[li+1] == "paint")
		    docarve = 0;
		  else if (list[li+1] == "carve")
		    docarve = 1;
		  else if (list[li+1] == "restore")
		    docarve = 2;
		  else if (list[li+1] == "set")
		    docarve = 3;
		  else
		    {
		      QMessageBox::information(0, "Error",
					       "Second parameter has to be one of paint/carve/restore/set.");
		      return;
		    }

		  li++;
		  float rad = 0;
		  float decay = 0;
		  int tag = -1;
		  if (docarve == 0)
		    {
		      rad = m_paths[idx]->getRadX(0)/2;
		      if (list.size()-li > 1)
			{
			  tag = list[li+1].toInt(&ok);
			  tag = qBound(0, tag, 255);
			  li++;
			}
		    }
		  else
		    {
		      if (list.size()-li > 1)
			{
			  rad = list[li+1].toFloat(&ok);
			  rad = qBound(0.0f, rad, 200.0f);
			  li++;
			}
		      if (list.size()-li > 1)
			{
			  decay = list[li+1].toFloat(&ok);
			  decay = qBound(0.0f, decay, rad);
			  li++;
			}
		    }
		  
		  emit sculpt(docarve, pathPoints, rad, decay, tag);
		}
	    }
	  else
	    QMessageBox::information(0, "Error",
				     "No operation specified for mop.");
	}
      else if (list[li] == "moves")
	{
	  if (list.size()-li > 1)
	    {
	      if (list[li+1] == "-")
		m_paths[idx]->translate(true, false);
	      else
		m_paths[idx]->translate(true, true);

	      li++;
	    }
	  else
	    m_paths[idx]->translate(true, true);
	}
      else if (list[li] == "movet")
	{
	  if (list.size()-li > 1)
	    {
	      if (list[li+1] == "-")
		m_paths[idx]->translate(false, false);
	      else
		m_paths[idx]->translate(false, true);

	      li++;
	    }
	  else
	    m_paths[idx]->translate(false, true);
	}
      else if (list[li].contains("reslice"))
	{
	  bool fullThickness = false;	  
	  if (list[li] == "reslicefull")
	    fullThickness = true;
	  
	  int subsample = 1;
	  if (list.size()-li > 1)
	    {
	      subsample = qMax(1, list[li+1].toInt(&ok));
	      li++;
	    }

	  int tagvalue = -1;
	  if (list.size()-li > 1)
	    {
	      tagvalue = list[li+1].toInt(&ok);
	      li++;
	    }

	  emit extractPath(idx, fullThickness, subsample, tagvalue);
	}
      else if (list[li] == "nocaption")
	{
	  m_paths[idx]->resetCaption();
	  li ++;
	}
      else if (list[li] == "caption")
	{
	  if (list.size()-li > 1 &&
	      list[li+1] == "no")
	    {
	      m_paths[idx]->resetCaption();
	      li ++;
	    }
	  else
	    loadCaption(idx);
	}
      else if (list[li] == "noimage")
	{
	  m_paths[idx]->resetImage();
	  li ++;
	}
      else if (list[li] == "image")
	{
	  if (list.size()-li > 1 &&
	      list[li+1] == "no")
	    {
	      m_paths[idx]->resetImage();
	      li ++;
	    }
	  else
	    m_paths[idx]->loadImage();
	}
      else if (list[li] == "addcamerapath")
	{
	  QList<Vec> points = m_paths[idx]->points();
	  QList<Vec> saxis = m_paths[idx]->saxis();
	  QList<Vec> taxis = m_paths[idx]->taxis();
	  QList<Vec> tang = m_paths[idx]->tangents();
	  emit addToCameraPath(points, tang, saxis, taxis);
	}
      else if (list[li] == "save")
	{
	  QString flnm;
	  flnm = QFileDialog::getSaveFileName(0,
					      "Save path points to text file",
					      Global::previousDirectory(),
					      "Files (*.*)");
	  
	  if (flnm.isEmpty())
	    return;

	  QList<Vec> pts = m_paths[idx]->points();

	  QFile fpath(flnm);
	  fpath.open(QFile::WriteOnly | QFile::Text);
	  QTextStream fd(&fpath);
	  fd << pts.count() << "\n";
	  for(int pi=0; pi < pts.count(); pi++)
	    fd << pts[pi].x << " " << pts[pi].y << " " << pts[pi].z << "\n";
	  
	  fd.flush();
	}
      else
	QMessageBox::information(0, "Path Command Error",
				 QString("Cannot understand the command : ") +
				 cmd);

      li++;
    }
}

void
Paths::loadCaption(int idx)
{
  QString ct = m_paths[idx]->captionText();
  QFont cf = m_paths[idx]->captionFont();
  QColor cc = m_paths[idx]->captionColor();
  QColor chc = m_paths[idx]->captionHaloColor();

  CaptionDialog cd(0, ct, cf, cc, chc);
  cd.hideOpacity(true);
  cd.move(QCursor::pos());
  if (cd.exec() != QDialog::Accepted)
    return;

  m_paths[idx]->setCaption(cd.text(),
			   cd.font(),
			   cd.color(),
			   cd.haloColor());
}


PathGrabber*
Paths::checkIfGrabsMouse(int x, int y, Camera *cam)
{
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->isInMouseGrabberPool())
	{
	  m_paths[i]->checkIfGrabsMouse(x, y, cam);
	  if (m_paths[i]->grabsMouse())
	    return m_paths[i];
	}
    }
  return NULL;
}
void
Paths::mousePressEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->mousePressEvent(e,c);
}
void
Paths::mouseMoveEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->mouseMoveEvent(e,c);
}
void
Paths::mouseReleaseEvent(QMouseEvent *e, Camera *c)
{
  for(int i=0; i<m_paths.count(); i++)
    m_paths[i]->mouseReleaseEvent(e,c);
}

bool
Paths::continuousAdd()
{
  bool cadd = false;
  for(int i=0; i<m_paths.count(); i++)
    cadd |= m_paths[i]->add();
  return cadd;
}
void
Paths::addPoint(Vec pt)
{
  for(int i=0; i<m_paths.count(); i++)
    if (m_paths[i]->add())
      {
	m_paths[i]->addPoint(pt);
	return;
      }
}

int
Paths::inViewport(int x, int y, int ow, int oh)
{
  float ar = (float)ow/(float)oh;
  float y1 = oh-y;
  for(int i=0; i<m_paths.count(); i++)
    {
      QVector4D vp = m_paths[i]->viewport();
      if (m_paths[i]->viewportTF() >= 0 &&
	  m_paths[i]->viewportTF() < Global::lutSize() &&
	  vp.x() >= 0.0)
	{
	  int vx, vy, vh, vw;
	  vx = vp.x()*ow;
	  vy = vp.y()*oh;
	  vw = vp.z()*ow;
	  vh = vp.w()*oh;
	  
	  if (x >= vx && x <= vx+vw &&
	      y1 >= vy && y1 <= vy+vh)
	    return i;
	}
    }
  return -1;
}

void
Paths::setViewportGrabbed(int i, bool f)
{
  if (i>= 0 && i<m_paths.count())
    m_paths[i]->setViewportGrabbed(f);;
}
void
Paths::resetViewportGrabbed()
{
  for(int i=0; i<m_paths.count(); i++)
    {
      m_paths[i]->setViewportGrabbed(false);
      if (m_paths[i]->getPointPressed() > -1)
	{
	  m_paths[i]->setPointPressed(-1);
	  emit updateGL();
	}
    }
}
int
Paths::viewportGrabbed()
{
  for(int i=0; i<m_paths.count(); i++)
    {
      if (m_paths[i]->viewportGrabbed())
	return i;
    }

  return -1;
}

void
Paths::getRequiredParameters(int i,
			     Vec voxelSize,
			     int ow, int oh,
			     int& vx, int& vy, int& vh, int& vw,
			     int& mh, int& mw,
			     int& shiftx, float& scale)  
{
  QVector4D vp = m_paths[i]->viewport();
  vx = vp.x()*ow;
  vy = oh-vp.y()*oh;
  vw = vp.z()*ow;
  vh = vp.w()*oh;
  vx+=1; vy+=1;
  vw-=2; vh-=2;
  mh = vy-vh/2;
  mw = vx+vw/2;
  shiftx = 5;

  QList<Vec> pathPoints = m_paths[i]->pathPoints();
  QList<Vec> pathX = m_paths[i]->pathX();
  QList<float> radX = m_paths[i]->pathradX();
  
  for(int np=0; np<pathPoints.count(); np++)
    pathX[np] = VECPRODUCT(voxelSize,pathX[np]);
    
  float maxheight = 0;
  for(int np=0; np<pathPoints.count(); np++)
    {
      float ht = (pathX[np]*radX[np]).norm();
      maxheight = max(maxheight, ht);
    }
  
  float imglength = 0;
  for(int np=1; np<pathPoints.count(); np++)
    {
      Vec p0 = VECPRODUCT(voxelSize,pathPoints[np]);
      Vec p1 = VECPRODUCT(voxelSize,pathPoints[np-1]);
      imglength += (p0-p1).norm();
    }
  scale = (float)(vw-11)/imglength;

  if (2*maxheight*scale > vh-21)
    scale = (float)(vh-21)/(2*maxheight);

  shiftx = 5 + ((vw-11) - (imglength*scale))*0.5;
}

int
Paths::getPointPressed(int i, Vec voxelSize,
		       QList<Vec> pathPoints,
		       QPoint scr,
		       int mh, int vh, int shiftx, float scale)
{
  int idx = -1;

  float y = mh-vh/2+5;
  if (fabs(y-scr.y()) < vh)
    {
      if (fabs((float)shiftx - scr.x()) < 20) // first point
	return 0;
      
      int nseg = m_paths[i]->segments();
      int npts = pathPoints.count()/nseg;
      npts += (pathPoints.count()%nseg > 0);
      float clen = 0;
      int ptn = 0;
      for(int np=1; np<npts; np++)
	{
	  for (int s=0; s<nseg; s++)
	    {
	      ptn++;
	      Vec p0 = VECPRODUCT(voxelSize,pathPoints[ptn]);
	      Vec p1 = VECPRODUCT(voxelSize,pathPoints[ptn-1]);
	      clen += (p0-p1).norm();
	    }
	  if (fabs(shiftx+clen*scale - scr.x()) < 20)
	    {
	      idx = np;
	      break;
	    }
	}
    }

  return idx;
}

bool
Paths::viewportKeypressEvent(int i, QKeyEvent *event,
			     QPoint scr,
			     Vec voxelSize,
			     int ow, int oh)
{
  if (i < 0 || i > m_paths.count())
    return false;

  if (event->key() == Qt::Key_Delete)
    {
      QMessageBox::information(0, "", "Switching off the viewport");
      m_paths[i]->setViewport(QVector4D(-1,-1,-1,-1));
      return true;
    }
  else if (event->key() == Qt::Key_Space)
    return openPropertyEditor(i);
  else if (event->key() == Qt::Key_F)
    {
      m_paths[i]->flip();
      return true;
    }
  else if (event->key() == Qt::Key_Z)
    {
      if (event->modifiers() & Qt::ControlModifier ||
	  event->modifiers() & Qt::MetaModifier)
	{
	  m_paths[i]->undo();
	  return true;
	}      
    }


  // handle other keystrokes
  int vx, vy, vh, vw;
  int mh, mw;
  int shiftx;
  float scale;
  getRequiredParameters(i, voxelSize, ow, oh,
			vx, vy, vh, vw,
			mh, mw,
			shiftx, scale);

  QList<Vec> pathPoints = m_paths[i]->pathPoints();
  QList<Vec> pathX = m_paths[i]->pathX();
  QList<Vec> pathY = m_paths[i]->pathY();
  QList<float> radX = m_paths[i]->pathradX();
  QList<float> radY = m_paths[i]->pathradY();
  
  for(int np=0; np<pathPoints.count(); np++)
    pathX[np] = VECPRODUCT(voxelSize,pathX[np]);
  for(int np=0; np<pathPoints.count(); np++)
    pathY[np] = VECPRODUCT(voxelSize,pathY[np]);  
    
  int idx = getPointPressed(i, voxelSize,
			    pathPoints,
			    scr,
			    mh, vh, shiftx, scale);

  m_paths[i]->setPointPressed(idx);
  
  bool sameForAll = false;
  if (idx == -1)
    sameForAll = true;

  if (event->key() == Qt::Key_S)
    {
      float radx;
      if (idx > -1)
	radx = m_paths[i]->getRadX(idx);
      else
	radx = m_paths[i]->getRadX(0);
      if (event->modifiers() & Qt::ShiftModifier)
	radx--;
      else
	radx++;
      radx = qMax(1.0f, radx);
      m_paths[i]->setRadX(idx, radx, m_sameForAll);
      return true;
    }
  else if (event->key() == Qt::Key_T)
    {
      float rady;
      if (idx > -1)
	rady = m_paths[i]->getRadY(idx);
      else
	rady = m_paths[i]->getRadY(0);

      if (event->modifiers() & Qt::ShiftModifier)
	rady--;
      else
	rady++;
      rady = qMax(1.0f, rady);
      m_paths[i]->setRadY(idx, rady, m_sameForAll);	  
      return true;
    }
  else if (event->key() == Qt::Key_A)
    {
      float a;
      if (idx > -1)
	a = m_paths[i]->getAngle(idx);
      else
	a = m_paths[i]->getAngle(0);

      if (event->modifiers() & Qt::ShiftModifier)
	a--;
      else
	a++;
      m_paths[i]->setAngle(idx, a, m_sameForAll);
      return true;
    }
  
  return false;
}

void
Paths::modThickness(bool radT,
		    int i, int mag,
		    QPoint scr,
		    Vec voxelSize,
		    int ow, int oh)
{
  if (i < 0 || i > m_paths.count())
    return;

  int vx, vy, vh, vw;
  int mh, mw;
  int shiftx;
  float scale;
  getRequiredParameters(i, voxelSize, ow, oh,
			vx, vy, vh, vw,
			mh, mw,
			shiftx, scale);
			  
  QList<Vec> pathPoints = m_paths[i]->pathPoints();
  
  int idx = getPointPressed(i, voxelSize,
			    pathPoints,
			    scr,
			    mh, vh, shiftx, scale);

  m_paths[i]->setPointPressed(idx);
  
  bool sameForAll = false;
  if (idx == -1)
    sameForAll = true;

  if (radT) // modulate rady
    {
      float rady;
      if (idx > -1)
	rady = m_paths[i]->getRadY(idx);
      else    
	rady = m_paths[i]->getRadY(0);
      
      rady += mag;
      rady = qMax(1.0f, rady);
      m_paths[i]->setRadY(idx, rady, sameForAll);
    }
  else // modulate radx
    {
      float radx;
      if (idx > -1)
	radx = m_paths[i]->getRadX(idx);
      else    
	radx = m_paths[i]->getRadX(0);
      
      radx -= mag;
      radx = qMax(1.0f, radx);
      m_paths[i]->setRadX(idx, radx, sameForAll);
    }
    
}

void
Paths::translate(int i, int mag, int moveType,
		 QPoint scr,
		 Vec voxelSize,
		 int ow, int oh)
{
  if (i < 0 || i > m_paths.count())
    return;

  int vx, vy, vh, vw;
  int mh, mw;
  int shiftx;
  float scale;
  getRequiredParameters(i, voxelSize, ow, oh,
			vx, vy, vh, vw,
			mh, mw,
			shiftx, scale);
			  
  QList<Vec> pathPoints = m_paths[i]->pathPoints();

  int idx = getPointPressed(i, voxelSize,
			    pathPoints,
			    scr,
			    mh, vh, shiftx, scale);

  m_paths[i]->setPointPressed(idx);
  
  m_paths[i]->translate(idx, moveType, mag);
}

void
Paths::rotate(int i, int mag,
	      QPoint scr,
	      Vec voxelSize,
	      int ow, int oh)
{
  if (i < 0 || i > m_paths.count())
    return;

  int vx, vy, vh, vw;
  int mh, mw;
  int shiftx;
  float scale;
  getRequiredParameters(i, voxelSize, ow, oh,
			vx, vy, vh, vw,
			mh, mw,
			shiftx, scale);
			  
  QList<Vec> pathPoints = m_paths[i]->pathPoints();
  
  int idx = getPointPressed(i, voxelSize,
			    pathPoints,
			    scr,
			    mh, vh, shiftx, scale);

  m_paths[i]->setPointPressed(idx);
  
  m_paths[i]->rotate(idx, mag);
}

void
Paths::drawViewportBorders(QGLViewer *viewer)
{
  int ow = viewer->width();
  int oh = viewer->height();
  viewer->startScreenCoordinatesSystem();

  for (int i=0; i<m_paths.count(); i++)
    {
      QVector4D vp = m_paths[i]->viewport();
      // render only when textured plane and viewport active
      if (m_paths[i]->viewportTF() >= 0 &&
	  m_paths[i]->viewportTF() < Global::lutSize() &&
	  vp.x() >= 0.0)
	{
	  int vx, vy, vh, vw;
	  vx = vp.x()*ow;
	  vy = oh-vp.y()*oh;
	  vw = vp.z()*ow;
	  vh = vp.w()*oh;

	  vx+=1; vy+=1;
	  vw-=2; vh-=2;

	  glLineWidth(2);
	  if (m_paths[i]->viewportGrabbed())
	    {
	      glLineWidth(3);
	      glColor3f(0.0, 1.0, 0.3);
	    }
	  else
	    {
	      Vec pathColor = m_paths[i]->color();
	      glColor3dv(pathColor);
	    }
	  
	  glBegin(GL_LINE_STRIP);
	  glVertex2i(vx, vy);
	  glVertex2i(vx, vy-vh);
	  glVertex2i(vx+vw, vy-vh);
	  glVertex2i(vx+vw, vy);
	  glVertex2i(vx, vy);
	  glEnd();
	  if (m_paths[i]->viewportGrabbed())
	    {
	      glLineWidth(2);
	      glColor3f(0.8, 0.8, 0.8);
	    }
	  else
	    {
	      glLineWidth(1);
	      glColor3f(0.2, 0.2, 0.2);
	    }
	  glBegin(GL_LINE_STRIP);
	  glVertex2i(vx, vy);
	  glVertex2i(vx, vy-vh);
	  glVertex2i(vx+vw, vy-vh);
	  glVertex2i(vx+vw, vy);
	  glVertex2i(vx, vy);
	  glEnd();
	}
    }

  viewer->stopScreenCoordinatesSystem();
}

bool
Paths::viewportsVisible()
{
  for (int i=0; i<m_paths.count(); i++)
    {
      QVector4D vp = m_paths[i]->viewport();
      // render only when textured plane and viewport active
      if (m_paths[i]->viewportTF() >= 0 &&
	  m_paths[i]->viewportTF() < Global::lutSize() &&
	  vp.x() >= 0.0)
	return true;
    }
  return false;
}
