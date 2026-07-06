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
  //ui.tableWidget->hide();
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
              m_arguments.clear();

	    	      QJsonObject obj = value.toObject();
	    	      QStringList keys = obj.keys();
	    	      for (auto key : keys)
	    	        {
                  QString keystring = key.trimmed();
                  if (keystring.isEmpty())
                    continue;

                  QStringList words = keystring.split(" ");
                  if (words.count() != 2)
                    continue;
                  words[0] = words[0].trimmed();
                  words[1] = words[1].trimmed().toLower();

	    	          auto value = obj.take(key);

	    	          if (words[1] == "int")
                  {
	    	    	      addRow(words[0], "INT", QString("%1").arg(value.toInt()));
                    m_arguments.insert(words[0], value.toInt());
                  }
                  else if (words[1] == "float" || words[1] == "double")
                  {
	    	    	      addRow(words[0], "FLOAT", QString("%1").arg(value.toDouble()));
                    m_arguments.insert(words[0], value.toDouble());
                  }
                  else if (words[1] == "bool")
                  {
      	    	      addRow(words[0], "BOOL", QString("%1").arg(value.toBool()));
                    m_arguments.insert(words[0], value.toBool());
                  }
                  else
                  {
                    addRow(words[0], "STRING", value.toString());
                    m_arguments.insert(words[0], value.toString());
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
PyWidgetMenu::addRow(QString key, QString type, QString value)
{
  QTableWidgetItem *wKey = new QTableWidgetItem("");
  QTableWidgetItem *wType = new QTableWidgetItem("");
  QTableWidgetItem *wVal = new QTableWidgetItem("");

  wKey->setFlags(wKey->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);
  wType->setFlags(wType->flags() & ~Qt::ItemIsUserCheckable & ~Qt::ItemIsEditable);
  wVal->setFlags(wVal->flags() & ~Qt::ItemIsUserCheckable);

  wKey->setText(key);
  wType->setText(type);
  wVal->setText(value);
  
  int row = ui.tableWidget->rowCount();
  ui.tableWidget->insertRow(row);
  ui.tableWidget->setItem(row, 0, wKey);
  ui.tableWidget->setItem(row, 1, wType);
  ui.tableWidget->setItem(row, 2, wVal);

  if (row%2 == 0)
    {
      wKey->setBackground(QBrush(Qt::lightGray));
      wType->setBackground(QBrush(Qt::lightGray));
      wVal->setBackground(QBrush(Qt::lightGray));
    }
  
  ui.tableWidget->resizeColumnsToContents();
}

void
PyWidgetMenu::on_addRow_pressed()
{
  addRow("", "", "");
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

