QMap<QString, QMap<QString, MenuViewerFncPtr> >
Viewer::registerMenuFunctions()
{
  QMap<QString, QMap<QString, MenuViewerFncPtr> >  menuFnc;
  
//  {
//    QMap<QString, MenuViewerFncPtr> m1;
//    m1["Clip"] = &Viewer::clip;
//    m1["Path"] = &Viewer::path;
//    menuFnc["3D Widgets"] = m1;
//  }

  {
    QMap<QString, MenuViewerFncPtr> m1;
    m1[tr("Screen Caption")] = &Viewer::caption;
    //m1["Scale Bar"] = &Viewer::scaleBar;
    menuFnc[""] = m1;
  }

//  {
//    QMap<QString, MenuViewerFncPtr> m1;
//    m1["Add Point"] = &Viewer::addPoint;
//    m1["Remove Points"] = &Viewer::removePoints;
//    menuFnc["Points"] = m1;
//  }

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
  int cdW = cd.width();
  int cdH = cd.height();
  cd.move(QCursor::pos() - QPoint(cdW/2, cdH/2));
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
