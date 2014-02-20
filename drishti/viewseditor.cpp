#include "global.h"
#include "viewseditor.h"
#include "geometryobjects.h"

ViewsEditor::ViewsEditor(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  m_imgSize = 100;
  m_leftMargin = 40;
  m_topMargin = 50;

  m_viewInfo.clear();

  clear();

  m_hiresMode = false;
}

void
ViewsEditor::setHiresMode(bool flag)
{
  m_hiresMode = flag;
}

void
ViewsEditor::clear()
{
  m_inTransition = false;

  m_row = 0;
  ui.up->setDisabled(true);
  ui.down->setDisabled(true);
  ui.restore->setDisabled(true);
  ui.groupBox->setTitle("0 Views");

  for(int i=0; i<m_viewInfo.size(); i++)
    delete m_viewInfo[i];
  m_viewInfo.clear();

  m_Rect.clear();
  m_selected = -1;
}

void
ViewsEditor::resizeEvent(QResizeEvent *event)
{
  calcRect();
}

void
ViewsEditor::calcRect()
{
  int ncols = (size().width()-m_leftMargin-9)/m_imgSize;
  int gap = 5;

  int rowmx = m_row + (size().height()-m_topMargin-gap)/m_imgSize;
  m_Rect.clear();
  for(int i=0; i<m_viewInfo.count(); i++)
    {
      int r,c;
      r = i/ncols;
      c = i%ncols;
      
      if (r < rowmx && r >= m_row)
	{
	  QRect rect = QRect(c*m_imgSize + gap + m_leftMargin,
			     (r-m_row)*m_imgSize + m_topMargin + gap,
			     m_imgSize-2*gap, m_imgSize-2*gap);
	  m_Rect.append(rect);
	}
      else
	m_Rect.append(QRect(-1,-1, 0, 0));
    }

  m_maxRows = (m_viewInfo.count()-1)/ncols;

  if (m_row == 0)
    ui.up->setDisabled(true);
  else
    ui.up->setEnabled(true);

  if (rowmx <= m_maxRows)
    ui.down->setEnabled(true);
  else
    ui.down->setDisabled(true);

}

void
ViewsEditor::drawView(QPainter *p,
		      QColor penColor, QColor brushColor,
		      int i, QRect rect, QImage img)
{
  if (rect.y()+rect.height()+11 < size().height() &&
      rect.y() > m_topMargin)
    {      
      QRect prect = rect.adjusted(5, 5, -5, -5);
      p->drawImage(prect, img, QRect(0, 0, 100, 100));


      p->setPen(QPen(brushColor, 2,
		     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p->setBrush(Qt::lightGray);
      p->drawRoundRect(rect.x()+5,
		       rect.y()+rect.height()-20,
		       20, 15);
      p->setPen(Qt::black);
      if (i < 10)
	p->drawText(rect.x()+12, rect.y()+rect.height()-8,
		    QString("%1").arg(i));
      else
	p->drawText(rect.x()+8, rect.y()+rect.height()-8,
		    QString("%1").arg(i));


      if (m_inTransition)
	{
	  p->setPen(Qt::transparent);
	  int fade = m_fade;
	  if (m_fade > 255) fade = 511-fade;
	  p->setBrush(QColor(170, 170, 170, fade));
	  p->drawRect(prect);
	}


      p->setBrush(Qt::transparent);
      p->setPen(QPen(penColor, 7,
		     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p->drawLine(QPoint(rect.x(), rect.y()+3),
		  QPoint(rect.x()+rect.width(), rect.y()+3));
      p->setPen(QPen(brushColor, 4,
		     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p->drawLine(QPoint(rect.x(), rect.y()+3),
		  QPoint(rect.x()+rect.width(), rect.y()+3));
    }
}

void
ViewsEditor::paintEvent(QPaintEvent *event)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  
  for(int i=0; i<m_viewInfo.count(); i++)
    {
      QRect rect = m_Rect[i];

      QColor penColor, brushColor;
      
      if (i != m_selected)
	{
	  brushColor = QColor(170, 170, 170);
	  penColor = QColor(100, 100, 100);
	}
      else
	{
	  brushColor = QColor(250, 150, 100);
	  penColor = QColor(50, 30, 10);
	}

      drawView(&p, 
	       penColor, brushColor,
	       i+1, rect, m_viewInfo[i]->image());      
    }    


  if (m_inTransition)
    QTimer::singleShot(5, this, SLOT(updateTransition()));
}

void
ViewsEditor::addView(float stepStill,
		     float stepDrag,
		     bool drawBox,
		     bool drawAxis,
		     Vec backgroundColor,
		     Vec pos, Quaternion rot,
		     float focusDistance,
		     QImage image,
		     int renderQuality, 
		     LightingInformation lightInfo,
		     QList<BrickInformation> brickInfo,
		     Vec bmin, Vec bmax,
		     QList<SplineInformation> splineInfo,
		     int sz, int st,
		     QString xl, QString yl, QString zl)
{
  int volumeNumber=0, volumeNumber2=0, volumeNumber3=0, volumeNumber4=0;
  volumeNumber = Global::volumeNumber();
  if (Global::volumeType() == Global::DoubleVolume)
    volumeNumber2 = Global::volumeNumber(1);
  if (Global::volumeType() == Global::TripleVolume)
    volumeNumber3 = Global::volumeNumber(2);
  if (Global::volumeType() == Global::QuadVolume)
    volumeNumber4 = Global::volumeNumber(3);

  ViewInformation *vfi = new ViewInformation();
  vfi->setStepsizeStill(stepStill);
  vfi->setStepsizeDrag(stepDrag);
  vfi->setRenderQuality(renderQuality);
  vfi->setDrawBox(drawBox);
  vfi->setDrawAxis(drawAxis);
  vfi->setBackgroundColor(backgroundColor);
  vfi->setBackgroundImageFile(Global::backgroundImageFile());
  vfi->setFocusDistance(focusDistance);
  vfi->setVolumeNumber(volumeNumber);
  vfi->setVolumeNumber2(volumeNumber2);
  vfi->setVolumeNumber3(volumeNumber3);
  vfi->setVolumeNumber4(volumeNumber4);
  vfi->setPosition(pos);
  vfi->setOrientation(rot);
  vfi->setTagColors(Global::tagColors());
  vfi->setLightInfo(lightInfo);
  vfi->setClipInfo(GeometryObjects::clipplanes()->clipInfo());
  vfi->setBrickInfo(brickInfo);
  vfi->setSplineInfo(splineInfo);
  vfi->setVolumeBounds(bmin, bmax);
  vfi->setImage(image);
  vfi->setTick(sz, st, xl, yl, zl);
  vfi->setCaptions(GeometryObjects::captions()->captions());
  vfi->setPoints(GeometryObjects::hitpoints()->points());
  vfi->setPaths(GeometryObjects::paths()->paths());
  vfi->setPathGroups(GeometryObjects::pathgroups()->paths());
  vfi->setTrisets(GeometryObjects::trisets()->get());
  vfi->setNetworks(GeometryObjects::networks()->get());

  m_viewInfo.append(vfi);

  calcRect();
  ui.groupBox->setTitle(QString("%1 Views").arg(m_viewInfo.count()));
  update();
  emit showMessage("View added to the collection", false);
}

void
ViewsEditor::mousePressEvent(QMouseEvent *event)
{
  QPoint clickPos = event->pos();

  ui.restore->setDisabled(true);
  m_selected = -1;
  for(int i=0; i<m_Rect.count(); i++)
    {
      if (m_Rect[i].contains(clickPos))
	{
	  m_selected = i;
	  ui.restore->setEnabled(true);
	}
    }
  update();
}

void
ViewsEditor::on_add_pressed()
{
  if (!m_hiresMode)
    {
      emit showMessage("Add view available only in Hires Mode. Press F2 to switch to Hires mode", true);
      qApp->processEvents();
      return;
    }

  emit currentView();
}

void
ViewsEditor::on_remove_pressed()
{
  if (!m_hiresMode)
    {
      emit showMessage("Remove view available only in Hires Mode. Press F2 to switch to Hires mode", true);
      qApp->processEvents();
      return;
    }

  if (m_selected < 0)
    {
      emit showMessage("No view selected for removal", true);
      qApp->processEvents();
      return;
    }

  QList<QRect>::iterator itfRect=m_Rect.begin()+m_selected;
  m_Rect.erase(itfRect);

  QList<ViewInformation*>::iterator itfImage=m_viewInfo.begin()+m_selected;
  m_viewInfo.erase(itfImage);

  m_selected = -1;

  calcRect();
  ui.groupBox->setTitle(QString("%1 Views").arg(m_viewInfo.count()));
  update();

  emit showMessage("Selected view removed", false);
}

void
ViewsEditor::on_restore_pressed()
{
  if (m_selected < 0)
    {
      emit showMessage("No view selected for restore", true);
      qApp->processEvents();
      return;
    }

  
  float stepStill = m_viewInfo[m_selected]->stepsizeStill();
  float stepDrag = m_viewInfo[m_selected]->stepsizeDrag();
  int renderQuality = m_viewInfo[m_selected]->renderQuality();
  bool drawBox = m_viewInfo[m_selected]->drawBox();
  bool drawAxis = m_viewInfo[m_selected]->drawAxis();
  Vec backgroundColor = m_viewInfo[m_selected]->backgroundColor();
  QString backgroundImage = m_viewInfo[m_selected]->backgroundImageFile();
  Vec pos = m_viewInfo[m_selected]->position();
  Quaternion rot = m_viewInfo[m_selected]->orientation();
  float focus = m_viewInfo[m_selected]->focusDistance();
  Vec bmin, bmax;
  m_viewInfo[m_selected]->volumeBounds(bmin, bmax);
  LightingInformation lightInfo = m_viewInfo[m_selected]->lightInfo();
  QList<BrickInformation> brickInfo = m_viewInfo[m_selected]->brickInfo();
  QList<SplineInformation> splineInfo = m_viewInfo[m_selected]->splineInfo();
  int sz, st;
  QString xl, yl, zl;
  m_viewInfo[m_selected]->getTick(sz, st, xl, yl, zl);

  if (Global::volumeType() == Global::SingleVolume)
    {
      int volnum = m_viewInfo[m_selected]->volumeNumber();
      emit updateVolInfo(volnum);
      emit updateVolumeBounds(volnum, bmin, bmax);
    }
  else if (Global::volumeType() == Global::DoubleVolume)
    {
      int volnum1 = m_viewInfo[m_selected]->volumeNumber();
      int volnum2 = m_viewInfo[m_selected]->volumeNumber2();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolumeBounds(volnum1, volnum2, bmin, bmax);
    }
  else if (Global::volumeType() == Global::TripleVolume)
    {
      int volnum1 = m_viewInfo[m_selected]->volumeNumber();
      int volnum2 = m_viewInfo[m_selected]->volumeNumber2();
      int volnum3 = m_viewInfo[m_selected]->volumeNumber3();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolInfo(2, volnum3);
      emit updateVolumeBounds(volnum1, volnum2, volnum3, bmin, bmax);
    }
  else if (Global::volumeType() == Global::QuadVolume)
    {
      int volnum1 = m_viewInfo[m_selected]->volumeNumber();
      int volnum2 = m_viewInfo[m_selected]->volumeNumber2();
      int volnum3 = m_viewInfo[m_selected]->volumeNumber3();
      int volnum4 = m_viewInfo[m_selected]->volumeNumber4();
      emit updateVolInfo(volnum1);
      emit updateVolInfo(1, volnum2);
      emit updateVolInfo(2, volnum3);
      emit updateVolInfo(3, volnum4);
      emit updateVolumeBounds(volnum1, volnum2, volnum3, volnum4, bmin, bmax);
    }
  //emit updateVolInfo(volnum);
  //emit updateVolumeBounds(volnum, bmin, bmax);  
  emit updateLookFrom(pos, rot, focus);
  emit updateLightInfo(lightInfo);
  emit updateBrickInfo(brickInfo);
  emit updateParameters(stepStill, stepDrag,
			renderQuality,
			drawBox, drawAxis, 
			backgroundColor,
			backgroundImage,
			sz, st, xl, yl, zl);

  // update transfer functions only after stepsizes have been updated
  emit updateTransferFunctionManager(splineInfo);
  
  Global::setTagColors(m_viewInfo[m_selected]->tagColors());
  emit updateTagColors();

  GeometryObjects::captions()->setCaptions(m_viewInfo[m_selected]->captions());
  GeometryObjects::hitpoints()->setPoints(m_viewInfo[m_selected]->points());
  GeometryObjects::paths()->setPaths(m_viewInfo[m_selected]->paths());
  GeometryObjects::pathgroups()->setPaths(m_viewInfo[m_selected]->pathgroups());
  GeometryObjects::trisets()->set(m_viewInfo[m_selected]->trisets());
  GeometryObjects::networks()->set(m_viewInfo[m_selected]->networks());
  GeometryObjects::clipplanes()->set(m_viewInfo[m_selected]->clipInfo());

  emit updateGL();

  emit showMessage(QString("View %1").arg(m_selected+1), false);
  qApp->processEvents();
}

void
ViewsEditor::on_up_pressed()
{
  m_inTransition = false;
  if (m_row > 0)
    {
      m_fade = 0;
      m_inTransition = true;
    }

  m_row = qMax(0, m_row-1);

  QTimer::singleShot(5, this, SLOT(updateTransition()));
}

void
ViewsEditor::on_down_pressed()
{
  m_inTransition = false;
  if (m_row < m_maxRows)
    {
      m_fade = 0;
      m_inTransition = true;
    }

  m_row = qMin(m_maxRows, m_row+1);

  QTimer::singleShot(5, this, SLOT(updateTransition()));
}

void
ViewsEditor::updateTransition()
{
  m_fade = qMin(m_fade+4, 511);

  if (m_fade >= 255 && m_fade <= 257)
    calcRect();

  if (m_fade >= 511)
    m_inTransition = false;

  update();
}

void
ViewsEditor::load(fstream &fin)
{
  char keyword[100];

  clear();

  int n;
  fin.read((char*)&n, sizeof(int));

  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "done") == 0)
	break;

      if (strcmp(keyword, "viewstart") == 0)
	{
	  ViewInformation *vi = new ViewInformation();
	  vi->load(fin);
	  m_viewInfo.append(vi);
	}
    }

  ui.groupBox->setTitle(QString("%1 Views").arg(m_viewInfo.count()));
  update();
}

void
ViewsEditor::save(fstream &fout)
{
  QString keyword;
  keyword = "views";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  

  int n = m_viewInfo.count();
  fout.write((char*)&n, sizeof(int));

  for(int vi=0; vi<n; vi++)
    m_viewInfo[vi]->save(fout);

  keyword = "done";
  fout.write((char*)(keyword.toLatin1().data()), keyword.length()+1);  
}
