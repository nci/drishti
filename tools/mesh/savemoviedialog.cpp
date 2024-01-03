#include <QFileDialog>
#include <QMessageBox>
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
      ui.m_fileName->setText(flnm);
      if (QFileInfo::exists(flnm))
	on_m_file_pressed();
    }
}


void
SaveMovieDialog::on_m_file_pressed()
{
  QString flnm;
#if defined(Q_OS_WIN)
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Movie",
				      m_dir,
				      "Movie Files (*.wmv)");
#elif defined(Q_OS_OSX)
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Movie",
				      m_dir,
				      "Movie Files (*.mov)");
#else
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Movie",
				      m_dir,
				      "Movie Files (*.mp4)");
#endif 

  if (flnm.isEmpty())
    return;

  ui.m_fileName->setText(flnm);
}
