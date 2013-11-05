#include <QFileDialog>
#include "savemoviedialog.h"

SaveMovieDialog::SaveMovieDialog(QWidget *parent,
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
  
}

QString SaveMovieDialog::fileName() { return ui.m_fileName->text();}
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
SaveMovieDialog::on_m_file_pressed()
{
  QString flnm;
#if defined(Q_WS_WIN32)
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Movie",
				      m_dir,
				      "Movie Files (*.wmv)");
#elif defined(Q_WS_MAC)
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
