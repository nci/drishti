#ifndef MENU_VIEWER_FUNCTIONS_H
#define MENU_VIEWER_FUNCTIONS_H

QMap<QString, MenuViewerFncPtr>
Viewer::registerMenuFunctions()
{
  QMap<QString, MenuViewerFncPtr> menuFnc;

  menuFnc["Change Slice Ordering"] = &Viewer::changeSliceOrdering;
  menuFnc["Image to Volume"] = &Viewer::image2volume;
  menuFnc["Reslice"] = &Viewer::reslice;
  menuFnc["Rescale"] = &Viewer::rescale;

  return menuFnc;
}


void
Viewer::changeSliceOrdering()
{
  if (Global::volumeType() != Global::SingleVolume)
    {
      QMessageBox::information(0, "", "Will only on single volume.");
      return;
    }

  m_Volume->pvlFileManager(0)->changeSliceOrdering();
  reloadData();
  updateGL();

  QMessageBox::information(0, "", "Done.");
}

void
Viewer::image2volume()
{
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
#endif
