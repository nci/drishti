#include "checkupdates.h"

CheckUpdate::CheckUpdate(QString versionString)
{
  m_versionString = versionString;

  m_http = new QHttp("anusf.anu.edu.au");
  m_http->get("http://anusf.anu.edu.au/~acl900/QDrishti/Version");
  
  connect(m_http, SIGNAL(requestFinished(int, bool)),
	  this, SLOT(finishedReading(int, bool)));

  connect(m_http, SIGNAL(readyRead(const QHttpResponseHeader&)),
	  this, SLOT(readyRead(const QHttpResponseHeader&)));
}

CheckUpdate::~CheckUpdate()
{
  if (m_http->state() != QHttp::Unconnected)
    m_http->close();
}

void
CheckUpdate::kill()
{
  if (m_http->state() != QHttp::Unconnected)
    m_http->close();
}

void
CheckUpdate::readyRead(const QHttpResponseHeader& resp)
{
  QString version(m_http->readAll());
  version = version.trimmed();
  if (m_versionString != version)
    {
      QMessageBox::information(0, "Drishti Update", "New Drishti version ready for download.\nVisit http://anusf.anu.edu.au/Vizlab/drishti to download the new version");
    }
}

void
CheckUpdate::finishedReading(int id, bool error)
{
  m_http->close();
}
