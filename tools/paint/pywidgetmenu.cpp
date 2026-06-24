#include "pywidgetmenu.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDir>

PyWidgetMenu::PyWidgetMenu(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);
  setStyleSheet("QWidget{background:gainsboro;}"); 
  ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui.tableWidget->setFont(QFont("MS Reference Sans Serif", 12));
  ui.scriptList->setFont(QFont("MS Reference Sans Serif", 12));
  ui.textEdit->setFont(QFont("MS Reference Sans Serif", 12));
  
  connect(ui.scriptList, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(on_scriptChanged(int)));

  m_scriptDir.clear();
  m_jsonFileList.clear();
  m_script.clear();

  // hide the tableWidget for the time being
  ui.tableWidget->hide();
  ui.addRow->hide();
  ui.removeRow->hide();
}

PyWidgetMenu::~PyWidgetMenu()
{
  ui.tableWidget->clear();
}

void
PyWidgetMenu::on_scriptChanged(int idx)
{
  m_script.clear();
	ui.textEdit->clear();

  // clear all rows beyond the third row
  for(int i=ui.tableWidget->rowCount()-1; i>=3; i--)
    {
      QTableWidgetItem *wi = ui.tableWidget->item(i, 1);
      ui.tableWidget->removeRow(i);
    }
  
  if (ui.tableWidget->rowCount() == 0 || idx == -1)
    return;

  
  QString jsonfile = m_jsonFileList[idx];
  QFile fl(jsonfile);
  if (fl.open(QIODevice::ReadOnly))
    {
      QByteArray bytes = fl.readAll();
      fl.close();
      
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
          
	          if (key == "arguments")
	    	    {
	    	      QJsonObject obj = value.toObject();
	    	      QStringList keys = obj.keys();
	    	      for (auto key : keys)
	    	        {
	    	          auto value = obj.take(key);
	    	          if (value.isString())
	    	    	addRow(key, value.toString());
	    	          else
	    	    	addRow(key, QString("%1").arg(value.toDouble()));
	    	        }
	    	    }
	          if (key == "script")
	    	      m_script = m_scriptDir + QDir::separator() +
	    	           ui.scriptList->currentText() + QDir::separator() +
	    	           value.toString();  
            if (key == "doc")
              //m_doc = value.toString();
              ui.textEdit->append(value.toString());
	        }	  
	    }
    }
}

void
PyWidgetMenu::loadScripts(QString scriptdir)
{
  QStringList scripts;

  m_scriptDir = scriptdir;
  
  QDir topDir(scriptdir);
  topDir.setFilter(QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot);
  topDir.setSorting(QDir::Name);

  QFileInfoList scriptD = topDir.entryInfoList();
  for (int i=0; i<scriptD.count(); i++)
    {
      QString jsonfile = scriptD[i].fileName();
      jsonfile += ".json";
      QDir pdir(scriptD[i].absoluteFilePath());
      if (pdir.exists(jsonfile))
	{
	  jsonfile = scriptD[i].absoluteFilePath() + QDir::separator() + jsonfile;
	  QFile fl(jsonfile);
	  if (fl.open(QIODevice::ReadOnly))
	    {
	      QByteArray bytes = fl.readAll();
	      fl.close();

	      QJsonParseError jsonError;
	      QJsonDocument document = QJsonDocument::fromJson( bytes, &jsonError );
	      if (jsonError.error != QJsonParseError::NoError )
		{
		  QMessageBox::information(0, "Error",
					   QString("fromJson failed: %1"). \
					   arg(jsonError.errorString()));
		}
	      else if (document.isObject() )
		{
		  QJsonObject jsonObj = document.object(); 
		  QStringList keys = jsonObj.keys();

		  QString skrpt;
		  for (auto key : keys)
		    {
		      QString value = jsonObj.take(key).toString();
		      if (!value.isEmpty())
			{
			  if (key == "script")
			    skrpt = scriptD[i].fileName();
			}
		    }
		  if (!skrpt.isEmpty())
		    {
		      scripts << skrpt;
		      m_jsonFileList << jsonfile;
		    }
		}
	    }
	}
    }

  if (scripts.count() == 0)
    QMessageBox::information(0, "Error", "No scripts found under "+scriptdir);
    
  ui.scriptList->addItems(scripts);
  ui.scriptList->setCurrentIndex(-1);  
}

void
PyWidgetMenu::addRow(QString key, QString value)
{
  QTableWidgetItem *wKey = new QTableWidgetItem("");
  QTableWidgetItem *wVal = new QTableWidgetItem("");

  wKey->setFlags(wKey->flags() & ~Qt::ItemIsUserCheckable);
  wVal->setFlags(wVal->flags() & ~Qt::ItemIsUserCheckable);

  wKey->setText(key);
  wVal->setText(value);
  
  int row = ui.tableWidget->rowCount();
  ui.tableWidget->insertRow(row);
  ui.tableWidget->setItem(row, 0, wKey);
  ui.tableWidget->setItem(row, 1, wVal);

  if (row == 0)
    {
      wKey->setBackground(QBrush(Qt::darkGray));
      wVal->setBackground(QBrush(Qt::darkGray));
    }
  
  ui.tableWidget->resizeColumnsToContents();
}

void
PyWidgetMenu::on_runScript_pressed()
{
  emit runCommand(m_script);
}

QStringList
PyWidgetMenu::getData()
{
  QStringList kv;
  for (int i=0; i<ui.tableWidget->rowCount(); i++)
    {
      kv << ui.tableWidget->item(i,0)->text();
      kv << ui.tableWidget->item(i,1)->text(); 
    }
  return kv;
}

void
PyWidgetMenu::on_addRow_pressed()
{
  addRow("", "");
}

void
PyWidgetMenu::on_removeRow_pressed()
{
  QList<int> selIdx;
  for(int i=ui.tableWidget->rowCount()-1; i>=3; i--)
    {
      QTableWidgetItem *wi = ui.tableWidget->item(i, 1);
      if (wi->isSelected())
	      ui.tableWidget->removeRow(i);
    }
}
