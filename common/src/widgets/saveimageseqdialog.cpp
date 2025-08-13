#include <QFileDialog>
#include "saveimageseqdialog.h"

SaveImageSeqDialog::SaveImageSeqDialog(QWidget *parent,
				       QString dir,
				       int start,
				       int end,
				       int step) :
  QDialog(parent)
{
  ui.setupUi(this);

  m_dir = dir;
  ui.m_startFrame->setValue(start);
  ui.m_endFrame->setValue(end);
  ui.m_stepFrame->setValue(step);
  ui.m_imageMode->setCurrentIndex(0);
}

QString SaveImageSeqDialog::fileName() { return ui.m_fileName->text();}
int SaveImageSeqDialog::startFrame() { return ui.m_startFrame->value();}
int SaveImageSeqDialog::endFrame() { return ui.m_endFrame->value();}
int SaveImageSeqDialog::stepFrame() { return ui.m_stepFrame->value();}
int SaveImageSeqDialog::imageMode()
{
  return(ui.m_imageMode->currentIndex());
}

void
SaveImageSeqDialog::on_m_file_pressed()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Images",
				      m_dir,
				      "Image Files (*.png *.tif *.bmp *.jpg *.ppm *.xbm *.xpm)");

  if (flnm.isEmpty())
    return;

  ui.m_fileName->setText(flnm);
}
