#include "global.h"

QString Global::m_previousDirectory = "";
QString Global::previousDirectory() {return m_previousDirectory;}
void Global::setPreviousDirectory(QString d) {m_previousDirectory = d;}

QString Global::m_documentationPath = "";
QString
Global::documentationPath()
{
  if (m_documentationPath.isEmpty() == false)
    {
      if (m_documentationPath == "dontask")
	return "";
      else
	return m_documentationPath;
    }
  
#if defined(Q_OS_LINUX)
  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cd("doc");
  app.cd("import");

#elif defined(Q_OS_MAC)
  QDir app = QCoreApplication::applicationDirPath();

  app.cdUp();
  app.cdUp();
  app.cdUp();
  app.cd("Shared");
  app.cd("Docs");
  app.cd("import");

#elif defined(Q_OS_WIN32)

  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cd("docs");
  app.cd("import");

#else
  #error Unsupported platform.
#endif // __APPLE__

  QString page = QFileInfo(app, "drishtiimport.qhc").absoluteFilePath();
  QFileInfo f(page);

  if (f.exists())
    {
      m_documentationPath = f.absolutePath();
    }
  else
    {
      QString path;
      path = QFileDialog::getExistingDirectory(0,
			  "Drishti Import Documentation Directory",
			   QCoreApplication::applicationDirPath());
      if (path.isEmpty() == false)
	m_documentationPath = path;
      else
	m_documentationPath = "dontask";
    }
  m_documentationPath = QDir(m_documentationPath).canonicalPath();
  return m_documentationPath;
}

bool Global::m_rgbVolume = false;
void Global::setRGBVolume(bool flag) { m_rgbVolume = flag; }
bool Global::rgbVolume() { return m_rgbVolume; }

QStatusBar* Global::m_statusBar = 0;
QStatusBar* Global::statusBar() { return m_statusBar; }
void Global::setStatusBar(QStatusBar *sb) { m_statusBar = sb; }
