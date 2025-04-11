#include <QtGui>
#include "common.h"
#include "scriptsplugin.h"
#include <math.h>

#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDir>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QDataStream>


#if defined(Q_OS_WIN32)
#define isnan _isnan
#endif

ScriptsPlugin::~ScriptsPlugin()
{
  QByteArray Data;
  QString mesg = "exit";
  Data.append(mesg);
  qint64 res = m_sendingSocket.writeDatagram(Data, QHostAddress::LocalHost, 7760);

  m_sendingSocket.close();
  m_listeningSocket.close();
  m_process.close();
}


QStringList
ScriptsPlugin::registerPlugin()
{
  QStringList regString;
  regString << "script";
  regString << "Script : ";
  
  return regString;
}

void
ScriptsPlugin::setValue(QString key, float val)
{
}

void
ScriptsPlugin::setValue(QString key, QString val)
{
  //QMessageBox::information(0, key, val);
  
  if (key == "start")
    {
      m_jsonflnm = val;
      QString m_scriptDir = QFileInfo(m_jsonflnm).absolutePath();
      
      QFile fl(m_jsonflnm);
      if (fl.open(QIODevice::ReadOnly))
	{
	  QByteArray bytes = fl.readAll();
	  fl.close();

	  //QMessageBox::information(0, "JSON File", QString(bytes));
	  
	  QJsonParseError jsonError;
	  QJsonDocument document = QJsonDocument::fromJson( bytes, &jsonError );
	  if (jsonError.error != QJsonParseError::NoError )
	    {
	      QMessageBox::information(0, "Error",
				       QString("fromJson failed: %1").	\
				       arg(jsonError.errorString()));
	      return;
	    }

	  if (document.isObject() )
	    {
	      QJsonObject jsonObj = document.object(); 
	      QStringList keys = jsonObj.keys();
	      for (auto key : keys)
		{
		  auto value = jsonObj.take(key);

		  //QMessageBox::information(0, QString(key), value.toString());
		  
		  if (key == "executable")
		    m_executable = value.toString();		
		  if (key == "interpreter")
		    m_interpreter = value.toString();
		  if (key == "script")
		    m_script = m_scriptDir + QDir::separator() + value.toString();
		}	  
	    }
	  else
	    {
	      QMessageBox::information(0, "Error", "Error in json "+m_jsonflnm);
	      return;
	    }
	}
      else
	{
	  QMessageBox::information(0, "Error", "Cannot open "+m_jsonflnm);
	  return;
	}
    }
  
  //QMessageBox::information(0, m_interpreter, m_script);  

  
  m_listeningSocket.bind(QHostAddress::LocalHost, 7761);
  

  m_sharedFile.setFileName(QDir::tempPath()+ QDir::separator() +"DrishtiImportShare");
   
  //----------
#if defined(Q_OS_WIN32)
  m_process.start("cmd.exe");
#else
  m_process.start("/bin/bash");
#endif
  
  QString cmd;
  if (!m_executable.isEmpty())
    cmd = m_executable;
  else if (!m_interpreter.isEmpty() &&
	   !m_script.isEmpty())
    cmd = m_interpreter+" "+m_script;  
  
  m_process.write(cmd.toUtf8() + "\n");
  qApp->processEvents();
  //----------

  
  //delay for the process to start so that sockets are in place for the external script
  QThread::msleep(2000);  // 2 sec sleep
}


void
ScriptsPlugin::init()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
ScriptsPlugin::clear()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
ScriptsPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
ScriptsPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString ScriptsPlugin::description() { return m_description; }
int ScriptsPlugin::voxelType() { return m_voxelType; }
int ScriptsPlugin::voxelUnit() { return m_voxelUnit; }
int ScriptsPlugin::headerBytes() { return m_headerBytes; }

void
ScriptsPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;

//  if (m_voxelType == _UChar ||
//      m_voxelType == _Char ||
//      m_voxelType == _UShort ||
//      m_voxelType == _Short)
//    return;
}
float ScriptsPlugin::rawMin() { return m_rawMin; }
float ScriptsPlugin::rawMax() { return m_rawMax; }


QList<uint>
ScriptsPlugin::histogram()
{  
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      for(int i=0; i<256; i++)
	m_histogram.append(0);
    }
  else
    for(int i=0; i<65536; i++)
      m_histogram.append(0);
//  else if (m_voxelType == _UShort ||
//	   m_voxelType == _Short)
//    {
//      for(int i=0; i<65536; i++)
//	m_histogram.append(0);
//    }
  
  //-----------------------------------
    
  //-----------------------------------
  QByteArray Data;
  QString mesg = "histogram";
  Data.append(mesg);
  qint64 res = m_sendingSocket.writeDatagram(Data, QHostAddress::LocalHost, 7760);
  
  // block until we get reply
  while(!m_listeningSocket.waitForReadyRead())
    QThread::msleep(1); // sleep for a 10 msec

  QNetworkDatagram datagram = m_listeningSocket.receiveDatagram();

  m_sharedFile.open(QFile::ReadWrite);
  int len;
  m_sharedFile.read((char*)&len, 4);
  qint64 *hist = new qint64[len];
  m_sharedFile.read((char*)hist, len*8);
  for (int i=0; i<len; i++)
    m_histogram[i] = hist[i];
  m_sharedFile.close();
  
  delete [] hist;
  
  return m_histogram;
}



void
ScriptsPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
ScriptsPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
}

bool
ScriptsPlugin::setFile(QStringList files)
{    
  {
    QByteArray Data;
    QString mesg = "mmap : " + m_sharedFile.fileName();
    Data.append(mesg);
    qint64 res = m_sendingSocket.writeDatagram(Data, QHostAddress::LocalHost, 7760);
  }
  
  
  
  //-----------------------------------  
  QByteArray Data;
  QString mesg = "setfiles : ";
  mesg += files.join(" , ");
  Data.append(mesg);
  qint64 res = m_sendingSocket.writeDatagram(Data, QHostAddress::LocalHost, 7760);

  // block until we get reply
  while(!m_listeningSocket.waitForReadyRead())
    {
      QThread::msleep(10); // sleep for a 10 msec
    }
    
  while (m_listeningSocket.hasPendingDatagrams())
    {
      QNetworkDatagram datagram = m_listeningSocket.receiveDatagram();
	
      QStringList words = QString(datagram.data()).split(" : ");
      if (words[0] == "voxeltype")
	m_voxelType = words[1].toInt();
      if (words[0] == "header")
	m_skipBytes = m_headerBytes = words[1].toInt();
      if (words[0] == "dim")
	{
	  QStringList nxyz = words[1].split(",");
	  if (nxyz.count() == 3)
	    {
	      m_depth = nxyz[0].toInt();
	      m_width = nxyz[1].toInt();
	      m_height= nxyz[2].toInt();
	    }
	  else
	    {
	      QMessageBox::information(0, "Error",
				       QString("Dimensions not correct : %1").arg(nxyz.count())+words[1]);
	      return false;
	    }
	}
      if (words[0] == "rawminmax")
	{
	  QStringList rmm = words[1].split(",");
	  if (rmm.count() == 2)
	    {
	      m_rawMin = rmm[0].toFloat();
	      m_rawMax = rmm[1].toFloat();
	    }
	  else
	    {
	      QMessageBox::information(0, "Error",
				       QString("Raw Min Max not correct : %1").arg(rmm.count())+words[1]);
	      return false;
	    }	    
	}
    }
    
  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;
  
//  {
//    QString mesg;
//    mesg = QString("voxeltype : %1\n").arg(m_voxelType);
//    mesg += QString("bytes per voxel : %1\n").arg(m_bytesPerVoxel);
//    mesg += QString("header : %1\n").arg(m_headerBytes);
//    mesg += QString("dim : %1 %2 %3\n").arg(m_depth).arg(m_width).arg(m_height);		    
//    mesg += QString("raw min max : %1 %2\n").arg(m_rawMin).arg(m_rawMax);		      
//    QMessageBox::information(0, "Volume Data Information", mesg);
//  }

  //----------------------------------- 
    
  m_fileName = files;

  return true;
}

void
ScriptsPlugin::getDepthSlice(int slc,
			     uchar *slice)
{
  qint64 nbytes = m_width*m_height*m_bytesPerVoxel;
  if (slc < 0 || slc >= m_depth)
    {
      memset(slice, 0, nbytes);
      return;
    }

  //-----------------------------------
  
  QByteArray Data;
  QString mesg = QString("depthslice : %1").arg(slc);
  Data.append(mesg);
  qint64 res = m_sendingSocket.writeDatagram(Data, QHostAddress::LocalHost, 7760);

  // block until we get reply
  while(!m_listeningSocket.waitForReadyRead())
    QThread::msleep(10); // sleep for a 10 msec

  while (m_listeningSocket.hasPendingDatagrams())
    QNetworkDatagram datagram = m_listeningSocket.receiveDatagram();
  
  m_sharedFile.open(QFile::ReadWrite);
  int len;
  m_sharedFile.read((char*)&len, 4);
  m_sharedFile.read((char*)slice, nbytes);
  m_sharedFile.close();
}


QVariant
ScriptsPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }


  QByteArray Data;
  QString mesg = QString("rawvalue : %1 , %2 , %3").arg(d).arg(w).arg(h);
  Data.append(mesg);
  qint64 res = m_sendingSocket.writeDatagram(Data, QHostAddress::LocalHost, 7760);

  // block until we get reply
  while(!m_listeningSocket.waitForReadyRead())
    QThread::msleep(10); // sleep for a 10 msec

  QNetworkDatagram datagram = m_listeningSocket.receiveDatagram();
  
  m_sharedFile.open(QFile::ReadWrite);
  m_sharedFile.seek(4); // skip first 4 bytes
  if (m_voxelType == _UChar)
    {
      unsigned char a;
      m_sharedFile.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Char)
    {
      char a;
      m_sharedFile.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _UShort)
    {
      unsigned short a;
      m_sharedFile.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Short)
    {
      short a;
      m_sharedFile.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Int)
    {
      int a;
      m_sharedFile.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((int)a);
    }
  else if (m_voxelType == _Float)
    {
      float a;
      m_sharedFile.read((char*)&a, m_bytesPerVoxel);
      v = QVariant((double)a);
    }
  m_sharedFile.close();
 
  return v;
}
