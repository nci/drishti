QMap<QString, QMap<QString, MenuViewerFncPtr> >
Viewer::registerMenuFunctions()
{
  QMap<QString, QMap<QString, MenuViewerFncPtr> >  menuFnc;
  
  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Gi Lighting"] = &Viewer::showGiLightDialog;
    menuFnc[""] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Change Slice Ordering"] = &Viewer::changeSliceOrdering;
    m1["Image to Volume"] = &Viewer::image2volume;
    m1["Reslice"] = &Viewer::reslice;
    m1["Rescale"] = &Viewer::rescale;
    menuFnc["Data Ops"] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Clip"] = &Viewer::clip;
    m1["Crop"] = &Viewer::crop;
    m1["Blend"] = &Viewer::blend;
    m1["Disect"] = &Viewer::disect;
    m1["Glow"] = &Viewer::glow;
    m1["Displace"] = &Viewer::displace;
    m1["Path"] = &Viewer::path;
    menuFnc["3D Widgets"] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Caption"] = &Viewer::caption;
    m1["Color Bar"] = &Viewer::colorBar;
    m1["Scale Bar"] = &Viewer::scaleBar;
    menuFnc["Widgets"] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Float Precision"] = &Viewer::setFloatPrecision;
    m1["Volume"] = &Viewer::getVolume;
    m1["Surface Area"] = &Viewer::getSurfaceArea;
    m1["Path"] = &Viewer::path;
    menuFnc["Measurements"] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Add Point"] = &Viewer::addPoint;
    m1["Remove Points"] = &Viewer::removePoints;
    menuFnc["Points"] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Opacity Modulation"] = &Viewer::opmod;
    m1["Mop"] = &Viewer::mop;
    m1["Mix"] = &Viewer::mix;
    m1["Interpolate Volumes"] = &Viewer::interpolateVolumes;
    menuFnc["Misc"] = m1;
  }

  return menuFnc;
}

void
Viewer::showMenuFunctionHelp(QString fnc)
{
  QString helptext;

  bool done = false;
  QFile helpFile(":/viewer.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      if (keyword.trimmed() == fnc)
		{
		  line = in.readLine();
		  while (!line.isNull())
		    {
		      helptext += line;
		      helptext += "\n";
		      line = in.readLine();
		      if (line == "#end")
			{			  
			  break;
			  done = true;
			}
		    }
		}
	    }
	  if (done)
	    break;

	  line = in.readLine();
	}
    }  

  if (!helptext.isEmpty())
    QMessageBox::information(0, fnc, helptext);
}

void
Viewer::changeSliceOrdering()
{
  if (Global::volumeType() != Global::SingleVolume)
    {
      QMessageBox::information(0, "", "Will only on single volume.");
      return;
    }

  showMenuFunctionHelp("changesliceorder");

  m_Volume->pvlFileManager(0)->changeSliceOrdering();
  reloadData();

  uchar *tmplut = new unsigned char[Global::lutSize()*256*256*4];
  memcpy(tmplut, m_lut, Global::lutSize()*256*256*4);
  memset(m_lut, 0, Global::lutSize()*256*256*4);
  updateLookupTable();
  updateGL();

  memcpy(m_lut, tmplut, Global::lutSize()*256*256*4);
  updateLookupTable();
  updateGL();

  delete [] tmplut;

  QMessageBox::information(0, "", "Done.");
}

void
Viewer::image2volume()
{
  showMenuFunctionHelp("image2volume");

  QString pfile = m_hiresVolume->getResliceFileName();
  if (! pfile.isEmpty())
    {
      m_hiresVolume->saveImage2Volume(pfile);
      updateGL();
    }
}

void
Viewer::reslice()
{
  if (!m_hiresVolume->raised())
    {
      QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
      return;
    }

  showMenuFunctionHelp("reslice");

  float subsample = 1;
  int tagvalue = -1;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Reslice",
			       "Subsampling Level and Tag value",
			       QLineEdit::Normal,
			       QString("%1 %2").arg(subsample).arg(tagvalue),
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0) subsample = qMax(0.0f, list[0].toFloat(&ok));
      if (list.size() > 1) tagvalue = list[1].toInt(&ok);
    }

  m_hiresVolume->resliceVolume(camera()->position(),
			       camera()->viewDirection(),
			       camera()->rightVector(),
			       camera()->upVector(),
			       subsample,
			       0, tagvalue);

}

void
Viewer::rescale()
{
  if (!m_hiresVolume->raised())
    {
      QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
      return;
    }

  showMenuFunctionHelp("rescale");

  float subsample = 1;
  int tagvalue = -1;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Rescale",
			       "Subsampling Level and Tag value",
			       QLineEdit::Normal,
			       QString("%1 %2").arg(subsample).arg(tagvalue),
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0) subsample = qMax(0.0f, list[0].toFloat(&ok));
      if (list.size() > 1) tagvalue = list[1].toInt(&ok);
    }
  
  Vec smin = m_lowresVolume->volumeMin();
  Vec smax = m_lowresVolume->volumeMax();
  Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
  m_hiresVolume->resliceVolume(pos,
			       Vec(0,0,-1), Vec(1,0,0), Vec(0,1,0),
			       subsample,
			       0, tagvalue);

}

void
Viewer::colorBar()
{
  showMenuFunctionHelp("colorbar");

  int tfset = 0;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Color Bar",
			       "TF Set",
			       QLineEdit::Normal,
			       QString("%1").arg(tfset),
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0)
	tfset = qMax(0, list[0].toInt(&ok));
    }

  ColorBarObject cbo;
  cbo.set(QPointF(0.5, 0.5),
	  1, // vertical
	  tfset,
	  50, // width
	  200, // height
	  true); // color only
  GeometryObjects::colorbars()->add(cbo);
}

void
Viewer::scaleBar()
{
  showMenuFunctionHelp("scalebar");

  float nvox = 100;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Scale Bar",
			       "Number of Voxels",
			       QLineEdit::Normal,
			       QString("%1").arg(nvox),
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0)
	nvox= qMax(0.0f, list[0].toFloat(&ok));
    }

  ScaleBarObject sbo;
  sbo.set(QPointF(0.5, 0.5), nvox, true, true); // horizontal
  GeometryObjects::scalebars()->add(sbo);
}

void
Viewer::caption()
{
  showMenuFunctionHelp("caption");

  CaptionDialog cd(0,
		   "Caption",
		   QFont("Helvetica", 15),
		   QColor::fromRgbF(1,1,1,1),
		   QColor::fromRgbF(1,1,1,1),
		   0);
  cd.hideAngle(false);
  cd.move(QCursor::pos());
  if (cd.exec() == QDialog::Accepted)
    {
      QString text = cd.text();
      QFont font = cd.font();
      QColor color = cd.color();
      QColor haloColor = cd.haloColor();
      float angle = cd.angle();
      
      CaptionObject co;
      co.set(QPointF(0.5, 0.5),
	     text, font,
	     color, haloColor,
	     angle);
      GeometryObjects::captions()->add(co);
    }
}

void
Viewer::getVolume()
{
  if (!m_hiresVolume->raised())
    {
      QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
      return;
    }

  showMenuFunctionHelp("getvolume");

  int tag = -1;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Get Volume",
			       "Tag value",
			       QLineEdit::Normal,
			       QString("%1").arg(tag),
			       &ok);

  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0)
	tag = qMax(-1, list[0].toInt(&ok));
    }

  
  Vec smin = m_lowresVolume->volumeMin();
  Vec smax = m_lowresVolume->volumeMax();

  Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
  if (tag >= -1 && tag <= 255)
    {
      Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
      m_hiresVolume->resliceVolume(pos,
				   Vec(0,0,-1), Vec(1,0,0), Vec(0,1,0),
				   1, // no subsampling
				   1, tag); // use opacity to getVolume
    }
  else
    QMessageBox::critical(0, "Error",
			  "Tag value should be between 0 and 255");
}

void
Viewer::getSurfaceArea()
{
  if (!m_hiresVolume->raised())
    {
      QMessageBox::critical(0, "Error", "Cannot apply command in Lowres mode");
      return;
    }

  showMenuFunctionHelp("getsurfacearea");

  int tag = -1;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Get Volume",
			       "Tag value",
			       QLineEdit::Normal,
			       QString("%1").arg(tag),
			       &ok);

  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0)
	tag = qMax(-1, list[0].toInt(&ok));
    }

  
  Vec smin = m_lowresVolume->volumeMin();
  Vec smax = m_lowresVolume->volumeMax();
  if (tag >= -1 && tag <= 255)
    {
      Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
      m_hiresVolume->resliceVolume(pos,
				   Vec(0,0,-1), Vec(1,0,0), Vec(0,1,0),
				   1, // no subsampling
				   2, tag); // use opacity to getVolume
    }
  else
    QMessageBox::critical(0, "Error",
			  "Tag value should be between 0 and 255");
}

void
Viewer::setFloatPrecision()
{
  showMenuFunctionHelp("floatprecision");

  int fp = 2;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Float Precision",
			       "Float Precision",
			       QLineEdit::Normal,
			       QString("%1").arg(fp),
			       &ok);

  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0)
	fp = qMax(0, list[0].toInt(&ok));
		  
      Global::setFloatPrecision(fp);
    }
}

void
Viewer::addPoint()
{
  showMenuFunctionHelp("addpoint");

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Add Point",
			       "X Y Z value",
			       QLineEdit::Normal,
			       "0 0 0",
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);

      Vec pos;
      float x=0,y=0,z=0;
      if (list.size() > 0) x = list[0].toFloat(&ok);
      if (list.size() > 1) y = list[1].toFloat(&ok);
      if (list.size() > 2) z = list[2].toFloat(&ok);
      pos = Vec(x,y,z);
      GeometryObjects::hitpoints()->add(pos);
    }
}

void
Viewer::removePoints()
{
  showMenuFunctionHelp("removepoints");

  QStringList items;
  items << "All";
  items << "Selected";
  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      "Remove Points",
			      "Remove Points",
			      items,
			      0,
			      false, // text is not editable
			      &ok);
  if (!ok || str == "All")
    GeometryObjects::hitpoints()->clear();
  else
    GeometryObjects::hitpoints()->removeActive();
}

void
Viewer::interpolateVolumes()
{
  showMenuFunctionHelp("interpolatevolumes");

  QStringList items;
  items << "Off";
  items << "Color";
  items << "Value";
  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      "Interpolate Volumes",
			      "Interpolate Volumes",
			      items,
			      0,
			      false, // text is not editable
			      &ok);
  int iv = 0;
  if (ok)
    {
      if (str == "Color") iv = 1;
      if (str == "Value") iv = 2;
    }

  m_hiresVolume->setInterpolateVolumes(iv);
}

void
Viewer::mix()
{
  showMenuFunctionHelp("mix");

  int mv = 0;
  bool mc = false;
  bool mo = false;
  bool mt = false;
  bool mtok = false;

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Mix",
			       "Mix Parameters",
			       QLineEdit::Normal,
			       "",
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      int idx = 0;
      if (list.size() > 0)
	{
	  if (list[0] == "0") { mv = 0; idx++; }
	  else if (list[0] == "1") { mv = 1; idx++; }
	  else if (list[0] == "2") { mv = 2; idx++; }
	}

      while (idx < list.size())
	{
	  if (list[idx] == "no" || list[idx] == "off") { mc = false; mo = false; }
	  else if (list[idx] == "color") mc = true;
	  else if (list[idx] == "opacity") mo = true;	  
	  else if (list[idx] == "tag" || list[idx] == "tags")
	    {
	      mtok = true;
	      mt = true;	  
	      if (idx+1 < list.size())
		{
		  idx++;
		  if (list[idx] == "no" || list[idx] == "off")
		    mt = false;
		}
	    }
	  idx ++;
	}

      if (!mtok)
	m_hiresVolume->setMix(mv, mc, mo);
      else
	{
	  updateTagColors();
	  m_hiresVolume->setMixTag(mt);
	  m_rcViewer.setMixTag(mt);
	}
    }
}

void
Viewer::mop()
{
  processMorphologicalOperations();
}

void
Viewer::path()
{
  showMenuFunctionHelp("path");

  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() > 1)
    {
      GeometryObjects::paths()->addPath(pts);
      
      // now remove points that were used to make the path
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    QMessageBox::critical(0, "Error", "Need at least 2 points to form a path"); 
}

void
Viewer::crop()
{
  showMenuFunctionHelp("crop");

  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 2)
    {
      GeometryObjects::crops()->addCrop(pts);

      // now remove points that were used to make the crop
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    QMessageBox::critical(0, "Error", "Need exactly 2 points to form a crop/dissect/blend/displace/glow"); 
}

void
Viewer::blend()
{
  showMenuFunctionHelp("blend");

  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 2)
    {
      GeometryObjects::crops()->addView(pts);

      // now remove points that were used to make the crop
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    QMessageBox::critical(0, "Error", "Need exactly 2 points to form a crop/dissect/blend/displace/glow"); 
}

void
Viewer::disect()
{
  showMenuFunctionHelp("disect");

  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 2)
    {
      GeometryObjects::crops()->addTear(pts);

      // now remove points that were used to make the crop
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    QMessageBox::critical(0, "Error", "Need exactly 2 points to form a crop/disect/blend/displace/glow"); 
}

void
Viewer::glow()
{
  showMenuFunctionHelp("glow");

  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 2)
    {
      GeometryObjects::crops()->addGlow(pts);

      // now remove points that were used to make the crop
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    QMessageBox::critical(0, "Error", "Need exactly 2 points to form a crop/disect/blend/displace/glow"); 
}

void
Viewer::displace()
{
  showMenuFunctionHelp("displace");

  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 2)
    {
      GeometryObjects::crops()->addDisplace(pts);

      // now remove points that were used to make the crop
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    QMessageBox::critical(0, "Error", "Need exactly 2 points to form a crop/disect/blend/displace/glow"); 
}

void
Viewer::clip()
{
  showMenuFunctionHelp("clip");
  
  QList<Vec> pts;
  if (GeometryObjects::hitpoints()->activeCount())
    pts = GeometryObjects::hitpoints()->activePoints();
  else
    pts = GeometryObjects::hitpoints()->points();
  
  if (pts.count() == 3)
    {
      GeometryObjects::clipplanes()->addClip(pts[0],
					     pts[1],
					     pts[2]);
      
      // now remove points that were used to make the clip
      if (GeometryObjects::hitpoints()->activeCount())
	GeometryObjects::hitpoints()->removeActive();
      else
	GeometryObjects::hitpoints()->clear();
    }
  else
    GeometryObjects::clipplanes()->addClip(); 
}

void
Viewer::opmod()
{
  showMenuFunctionHelp("opmod");

  float fop, bop;
  m_hiresVolume->getOpMod(fop, bop);

  bool ok;
  QString text;
  text = QInputDialog::getText(this,
			       "Opacity Modulation",
			       "Front and Back opacity modulation",
			       QLineEdit::Normal,
			       QString("%1 %2").arg(fop).arg(bop),
			       &ok);
  
  if (ok && !text.isEmpty())
    {      
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.size() > 0) bop = fop = list[0].toFloat(&ok);
      if (list.size() > 1) bop = list[1].toFloat(&ok);
      
      m_hiresVolume->setOpMod(fop, bop);
    }
}

void
Viewer::showGiLightDialog()
{
  if (LightHandler::openPropertyEditor())
    {
      m_hiresVolume->initShadowBuffers(true);
      updateGL();
    }
}
