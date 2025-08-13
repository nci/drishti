#include <QFileDialog>
#include <QMessageBox>
#include <QByteArray>

#include "savemoviedialog.h"

SaveMovieDialog::SaveMovieDialog(QWidget *parent,
				 QString dir,
				 int start,
				 int end,
				 int step) :
  QDialog(parent)
{
  ui.setupUi(this);

  ui.buttonBox->button(QDialogButtonBox::Ok)->setDefault(false);
  ui.buttonBox->button(QDialogButtonBox::Ok)->setAutoDefault(false);
    
  m_dir = dir;
  ui.m_startFrame->setValue(start);
  ui.m_endFrame->setValue(end);
  ui.m_stepFrame->setValue(step);

}

void SaveMovieDialog::keyPressEvent(QKeyEvent *e) {}

QString SaveMovieDialog::fileName() { return ui.m_fileName->text(); }
int SaveMovieDialog::startFrame() { return ui.m_startFrame->value();}
int SaveMovieDialog::endFrame() { return ui.m_endFrame->value();}
int SaveMovieDialog::stepFrame() { return ui.m_stepFrame->value();}
int SaveMovieDialog::frameRate() { return ui.m_frameRate->value();}

bool SaveMovieDialog::movieMode()
{
  if (ui.m_movieMode->currentIndex() == 0)
    return true; // save Mono image frames
  else
    return false; // save Stereo image frames
}

void
SaveMovieDialog::on_m_fileName_editingFinished()
{
  QString flnm = ui.m_fileName->text();
  QString path = QFileInfo(flnm).absolutePath();
  if (path == qApp->applicationDirPath())
    {
      QFileInfo finfo(m_dir, flnm);
      flnm = finfo.absoluteFilePath();

      QByteArray exten = flnm.toLatin1().right(4).toLower();
      if (exten != ".mp4")
	flnm += ".mp4";
      
      ui.m_fileName->setText(flnm);
      if (QFileInfo::exists(flnm))
	on_m_file_pressed();
    }
}


void
SaveMovieDialog::on_m_file_pressed()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Movie",
				      m_dir,
				      "Movie Files (*.mp4)");
  
  if (flnm.isEmpty())
    return;

  QByteArray exten = flnm.toLatin1().right(4).toLower();
  if (exten != ".mp4")
    flnm += ".mp4";

  ui.m_fileName->setText(flnm);
}
