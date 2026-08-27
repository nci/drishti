#include "pywidgetmenu.h"

#include "global.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

  ui.tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);

  connect(ui.scriptList, SIGNAL(currentIndexChanged(int)),
	  this, SLOT(on_scriptChanged(int)));

  m_scriptDir.clear();
  m_jsonFileList.clear();
  m_script.clear();

  //ui.tableWidget->hide();
  // hide add and remove row for the time being
  ui.addRow->hide();
  ui.removeRow->hide();
}

PyWidgetMenu::~PyWidgetMenu()
{
  ui.tableWidget->clear();
}

QVariant
PyWidgetMenu::getArg(QJsonValue val, QString &type, QString &desc)
{
  type = "";
  desc = "";

  if (val.isArray())
    {
      QJsonArray arr = val.toArray();
      if (arr.size() == 3)
	{
	  type = arr[0].toString().toLower();
	  desc = arr[1].toString();
	  if (type == "str")
	    return arr[2];
	  else
	    return arr[2].toVariant();
	}
  }

  return val.toVariant();
}

void
PyWidgetMenu::on_scriptChanged(int idx)
{
  m_script.clear();
  ui.textEdit->clear();

  // clear all rows
  for(int i=ui.tableWidget->rowCount()-1; i>=0; i--)
    {
      QTableWidgetItem *wi = ui.tableWidget->item(i, 1);
      ui.tableWidget->removeRow(i);
    }

  if (idx == -1)
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
	      QMessageBox::information(Global::mainWindow(), "Error",
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
		      m_arguments.clear();
		      m_desc.clear();
			
	    	      QJsonObject obj = value.toObject();
	    	      QStringList keys = obj.keys();
	    	      for (auto key : keys)
	    	        {
			  QString keystring = key.trimmed();
			  if (keystring.isEmpty())
			    continue;
			  
			  auto value = obj.take(key);

			  QString type, desc;
			  QVariant v = getArg(value, type, desc);

			  if (type.isEmpty())
			    continue;
			  
			  if (type == "int")
			    {
			      addRow(keystring, "INT", QString("%1").arg(v.toInt()), desc);
			      m_arguments.insert(keystring, v.toInt());
			    }
			  else if (type == "float" || type == "double")
			    {
			      addRow(keystring, "FLOAT", QString("%1").arg(v.toDouble()), desc);
			      m_arguments.insert(keystring, v.toDouble());
			    }
			  else if (type == "bool")
			    {
			      addRow(keystring, "BOOL", QString("%1").arg(v.toBool()), desc);
			      m_arguments.insert(keystring, v.toBool());
			    }
			  else
			    {
			      addRow(keystring, "STRING", v.toString(), desc);
			      m_arguments.insert(keystring, v.toString());
			    }
	    	        }
	    	    }
	          if (key == "script")
		    m_script = m_scriptDir + QDir::separator() +
		      ui.scriptList->currentText() + QDir::separator() +
		      value.toString();  
		  if (key == "doc")
		    ui.textEdit->append(value.toString());
	        }	  
	    }
    }

  ui.tableWidget->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
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
		  QMessageBox::information(Global::mainWindow(), "Error",
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
    QMessageBox::information(Global::mainWindow(), "Error", "No scripts found under "+scriptdir);
    
  ui.scriptList->addItems(scripts);
  ui.scriptList->setCurrentIndex(-1);  
}

void
PyWidgetMenu::addRow(QString key, QString type, QString value, QString desc)
{
  QTableWidgetItem *wKey = new QTableWidgetItem("");
  QTableWidgetItem *wType = new QTableWidgetItem("");
  QTableWidgetItem *wVal = new QTableWidgetItem("");
  QTableWidgetItem *wDesc = new QTableWidgetItem("");

  wKey->setFlags(wKey->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);
  wType->setFlags(wType->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);
  wVal->setFlags(wVal->flags() & ~Qt::ItemIsUserCheckable);
  wDesc->setFlags(wType->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);

  wKey->setText(key);
  wType->setText(type);
  wVal->setText(value);
  wDesc->setText(desc);
  
  int row = ui.tableWidget->rowCount();
  ui.tableWidget->insertRow(row);
  ui.tableWidget->setItem(row, 0, wKey);
  ui.tableWidget->setItem(row, 1, wType);
  ui.tableWidget->setItem(row, 2, wVal);
  ui.tableWidget->setItem(row, 3, wDesc);

  if (row%2 == 0)
    {
      wKey->setBackground(QBrush(Qt::lightGray));
      wType->setBackground(QBrush(Qt::lightGray));
      wVal->setBackground(QBrush(Qt::lightGray));
      wDesc->setBackground(QBrush(Qt::lightGray));
    }
  
  ui.tableWidget->resizeColumnsToContents();
}

void
PyWidgetMenu::on_addRow_pressed()
{
  addRow("", "", "","");
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
void
PyWidgetMenu::genArgumentsFromTable()
{
  for (int i=0; i<ui.tableWidget->rowCount(); i++)
    {
      QTableWidgetItem *keyItem = ui.tableWidget->item(i, 0);
      QTableWidgetItem *typeItem = ui.tableWidget->item(i, 1);
      QTableWidgetItem *valItem = ui.tableWidget->item(i, 2);

      if (!keyItem || !typeItem || !valItem)
        continue;

      QString key = keyItem->text().trimmed();
      if (key.isEmpty())
        continue;

      QString type = typeItem->text().trimmed().toLower();
      QString value = valItem->text().trimmed();

      if (type == "int")
        m_arguments.insert(key, value.toInt());
      else if (type == "float" || type == "double")
        m_arguments.insert(key, value.toDouble());
      else if (type == "bool")
      {
        if (value.toLower() == "true" || value == "1")
          m_arguments.insert(key, true);
        else
          m_arguments.insert(key, false);
      }
      else
        m_arguments.insert(key, value);
    }
}

void
PyWidgetMenu::on_runScript_pressed()
{
  genArgumentsFromTable();
  emit runCommand(m_script, m_arguments);
}

