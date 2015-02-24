#include "global.h"
#include "tagcoloreditor.h"
#include "coloreditor.h"
#include "dcolordialog.h"
#include "staticfunctions.h"
#include <QTime>

#include <QInputDialog>
#include <QItemEditorFactory>
#include <QItemEditorCreatorBase>
#include <QItemDelegate>
#include <QVBoxLayout>
#include <QHeaderView>

TagColorEditor::TagColorEditor()
{
  createGUI();
}

void
TagColorEditor::setColors()
{
  uchar *colors = Global::tagColors();
  for (int i=0; i < 256; i++)
    {      
      int r,g,b;
      float a;

      r = colors[4*i+0];
      g = colors[4*i+1];
      b = colors[4*i+2];

      QTableWidgetItem *colorItem = new QTableWidgetItem;
      colorItem->setData(Qt::DisplayRole, QString("%1").arg(i));
      colorItem->setBackground(QColor(r,g,b));
      
      int row = i/8;
      int col = i%8;
      table->setItem(row, col, colorItem);
    }
}

void
TagColorEditor::createGUI()
{
  table = new QTableWidget(32, 8);
  table->resize(300, 300);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers); // disable number editing
  table->verticalHeader()->hide();
  table->horizontalHeader()->hide();
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setStyleSheet("QTableWidget{gridline-color:white;}");

  setColors();

  for (int i=0; i < table->rowCount(); i++)
    table->setRowHeight(i, 20);

  for (int i=0; i < table->columnCount(); i++)
    table->setColumnWidth(i, 40);

  QPushButton *newTags = new QPushButton("New Tag Colors");

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(newTags);
  layout->addWidget(table);
  
  setLayout(layout);
  
  setWindowTitle(tr("Tag Color Editor"));

  connect(newTags, SIGNAL(clicked()),
	  this, SLOT(newTagsClicked()));

  connect(table, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(table, SIGNAL(cellDoubleClicked(int, int)),
	  this, SLOT(cellDoubleClicked(int, int)));
}

void
TagColorEditor::newTagsClicked()
{
  QStringList items;
  items << "Random";
  items << "Library";

  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      "Colors",
			      "Colors",
			      items,
			      0,
			      false, // text is not editable
			      &ok);
  
  if (!ok)
    return;

  if (str == "Library")    
    newColorSet(1);
  else
    newColorSet(QTime::currentTime().msec());

  emit tagColorChanged();
}

void
TagColorEditor::cellClicked(int row, int col)
{
  int index = row*8 + col;
  emit tagSelected(index);
}

void
TagColorEditor::cellDoubleClicked(int row, int col)
{
  QTableWidgetItem *item = table->item(row, col);
  uchar *colors = Global::tagColors();

  int index = row*8 + col;
  QColor clr = QColor(colors[4*index+0],
		      colors[4*index+1],
		      colors[4*index+2]);
  clr = DColorDialog::getColor(clr);
  if (!clr.isValid())
    return;

  colors[4*index+0] = clr.red();
  colors[4*index+1] = clr.green();
  colors[4*index+2] = clr.blue();
  item->setData(Qt::DisplayRole, QString("%1").arg(index));
  item->setBackground(clr);

  emit tagColorChanged();
}

void
TagColorEditor::newColorSet(int h)
{
  if (h == 1)
    {
      askGradientChoice();
      return;
    }

  qsrand(h);

  uchar *colors = Global::tagColors();

  for(int i=1; i<255; i++)
    {
      float r,g,b;
      r = (float)qrand()/(float)RAND_MAX;
      g = (float)qrand()/(float)RAND_MAX;
      b = (float)qrand()/(float)RAND_MAX;
      colors[4*i+0] = 255*r;
      colors[4*i+1] = 255*g;
      colors[4*i+2] = 255*b;
    }
  colors[0] = 0;
  colors[1] = 0;
  colors[2] = 0;
  colors[3] = 0;
  
  setColors();
}

void
TagColorEditor::copyGradientFile(QString stopsflnm)
{
  QString sflnm = ":/images/gradientstops.xml";
  QFileInfo fi(sflnm);
  if (! fi.exists())
    {
      QMessageBox::information(0, "Gradient Stops",
			       QString("Gradient Stops file does not exit at %1").arg(sflnm));
      
      return;
    }

  // copy information from gradientstops.xml to HOME/.drishtigradients.xml
  QDomDocument document;
  QFile f(sflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }
  
  QFile fout(stopsflnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      document.save(out, 2);
      fout.close();
    }
}

void
TagColorEditor::askGradientChoice()
{
  QString homePath = QDir::homePath();
  QFileInfo sfi(homePath, ".drishtigradients.xml");
  QString stopsflnm = sfi.absoluteFilePath();
  if (!sfi.exists())
    copyGradientFile(stopsflnm);

  QDomDocument document;
  QFile f(stopsflnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QStringList glist;

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.nodeName() == "name")
		glist << dnode.text();
	    }
	}
    }

  bool ok;
  QString gstr = QInputDialog::getItem(0,
				       "Color Gradient",
				       "Color Gradient",
				       glist, 0, false,
				       &ok);
  if (!ok)
    return;

  int cno = -1;
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gradient")
	{
	  QDomNodeList cnode = dlist.at(i).childNodes();
	  for(int j=0; j<cnode.count(); j++)
	    {
	      QDomElement dnode = cnode.at(j).toElement();
	      if (dnode.tagName() == "name" && dnode.text() == gstr)
		{
		  cno = i;
		  break;
		}
	    }
	}
    }
	
  if (cno < 0)
    return;

  QGradientStops stops;
  QDomNodeList cnode = dlist.at(cno).childNodes();
  for(int j=0; j<cnode.count(); j++)
    {
      QDomElement de = cnode.at(j).toElement();
      if (de.tagName() == "gradientstops")
	{
	  QString str = de.text();
	  QStringList strlist = str.split(" ", QString::SkipEmptyParts);
	  for(int j=0; j<strlist.count()/5; j++)
	    {
	      float pos, r,g,b,a;
	      pos = strlist[5*j].toFloat();
	      r = strlist[5*j+1].toInt();
	      g = strlist[5*j+2].toInt();
	      b = strlist[5*j+3].toInt();
	      a = strlist[5*j+4].toInt();
	      stops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
    }

  int mapSize = QInputDialog::getInt(0,
				     "Number of Colors",
				     "Number of Colors",
				     50, 2, 255, 1, &ok);
  if (!ok)
    mapSize = 50;

  QGradientStops gstops;
  gstops = StaticFunctions::resampleGradientStops(stops, mapSize);

  uchar *colors = Global::tagColors();  
  for(int i=0; i<gstops.size(); i++)
    {
      float pos = gstops[i].first;
      QColor color = gstops[i].second;
      int r = color.red();
      int g = color.green();
      int b = color.blue();
      colors[4*i+0] = r;
      colors[4*i+1] = g;
      colors[4*i+2] = b;
    }
  colors[0] = 0;
  colors[1] = 0;
  colors[2] = 0;
  colors[3] = 0;
  
  setColors();
}
