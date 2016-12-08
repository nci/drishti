QMap<QString, QMap<QString, MenuViewerFncPtr> >
Viewer::registerMenuFunctions()
{
  QMap<QString, QMap<QString, MenuViewerFncPtr> >  menuFnc;
  
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
    menuFnc["Measurements"] = m1;
  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1["Add Point"] = &Viewer::addPoint;
    m1["Remove Points"] = &Viewer::removePoints;
    menuFnc["Points"] = m1;
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
  updateGL();

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
	tfset = list[0].toInt(&ok);
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
	nvox= list[0].toFloat(&ok);
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
	tag = list[0].toInt(&ok);
    }

  
  Vec smin = m_lowresVolume->volumeMin();
  Vec smax = m_lowresVolume->volumeMax();

  Vec pos = Vec((smax.x+smin.x)*0.5,(smax.y+smin.y)*0.5,smax.z+10);
  if (tag >= -1)
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
	tag = list[0].toInt(&ok);
    }

  
  Vec smin = m_lowresVolume->volumeMin();
  Vec smax = m_lowresVolume->volumeMax();
  if (tag >= -1)
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
	fp = list[0].toInt(&ok);

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
