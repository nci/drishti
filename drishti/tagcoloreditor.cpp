#include "global.h"
#include "staticfunctions.h"
#include "tagcoloreditor.h"
#include "coloreditor.h"
#include "opacityeditor.h"

#include <QInputDialog>
#include <QItemEditorFactory>
#include <QItemEditorCreatorBase>
#include <QItemDelegate>
#include <QVBoxLayout>

TagColorEditor::TagColorEditor()
{
  createGUI();
}

void
TagColorEditor::setColors()
{
  uchar *colors = Global::tagColors();
  for (int i=0; i < 254; i++)
    {      
      int r,g,b;
      float a;

      r = colors[4*(i+1)+0];
      g = colors[4*(i+1)+1];
      b = colors[4*(i+1)+2];
      a = (float)colors[4*(i+1)+3]/255.0;

      //a = (int)a + (int)((a-(int)a)*10)*0.1;

      QTableWidgetItem *colorItem = new QTableWidgetItem;
      colorItem->setData(Qt::DisplayRole, QColor(r,g,b));
      colorItem->setData(Qt::DecorationRole, QColor(r,g,b));
      
      QTableWidgetItem *opacityItem = new QTableWidgetItem;
      opacityItem->setData(Qt::DisplayRole, a);

      table->setItem(i, 0, colorItem);
      table->setItem(i, 1, opacityItem);
    }
}

void
TagColorEditor::createGUI()
{
  QItemEditorFactory *colorFactory = new QItemEditorFactory;
  QItemEditorCreatorBase *colorCreator =
    new QStandardItemEditorCreator<ColorEditor>();
  colorFactory->registerEditor(QVariant::Color, colorCreator);
  
  QItemEditorFactory *opFactory = new QItemEditorFactory;
  QItemEditorCreatorBase *opacityCreator =
    new QStandardItemEditorCreator<OpacityEditor>();
  opFactory->registerEditor(QVariant::Double, opacityCreator);


  table = new QTableWidget(254, 2);
  table->setHorizontalHeaderLabels(QStringList() << tr("Color")
				                 << tr("Opacity"));
  table->resize(150, 200);
  
  QItemDelegate *colorDelegate = new QItemDelegate();
  colorDelegate->setItemEditorFactory(colorFactory);
  
  QItemDelegate *opDelegate = new QItemDelegate();
  opDelegate->setItemEditorFactory(opFactory);
  
  table->setItemDelegateForColumn(0, colorDelegate);
  table->setItemDelegateForColumn(1, opDelegate);
  
  table->setEditTriggers(QAbstractItemView::DoubleClicked);
  
  setColors();

  table->setGridStyle(Qt::NoPen);
  for (int i=0; i < table->rowCount(); i++)
    table->setRowHeight(i, 20);

  table->setColumnWidth(0, 100);
  table->setColumnWidth(1, 50);
    
  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(table);
  
  setLayout(layout);
  
  setWindowTitle(tr("Tag Color Editor"));
  
  connect(table, SIGNAL(itemChanged(QTableWidgetItem*)),
	  this, SLOT(itemChanged(QTableWidgetItem*)));

  connect(table, SIGNAL(cellClicked(int, int)),
	  this, SLOT(tagClicked(int, int)));
}

void
TagColorEditor::tagClicked(int row, int colm)
{
  if (colm == 0)
    emit tagClicked(row+1);
}

void
TagColorEditor::itemChanged(QTableWidgetItem *item)
{
  uchar *colors = Global::tagColors();

  int row = item->row();
  int colm = item->column();
  if (colm == 0)
    {
      QColor clr;
      clr.setNamedColor(item->data(Qt::DisplayRole).toString());
      item->setData(Qt::DecorationRole, clr);
      colors[4*(row+1)+0] = clr.red();
      colors[4*(row+1)+1] = clr.green();
      colors[4*(row+1)+2] = clr.blue();
    }
  else
    {
      double opv = item->data(Qt::DisplayRole).toDouble();
      int op = opv*255;
      op = qMin(op, 255);
      colors[4*(row+1)+3] = op;
    }

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

  for(int i=0; i<256; i++)
    {
      float r,g,b;
      if (i > 0 && i < 255)
	{
	  r = (float)qrand()/(float)RAND_MAX;
	  g = (float)qrand()/(float)RAND_MAX;
	  b = (float)qrand()/(float)RAND_MAX;
	}
      else
	{
	  r = g = b = 0;
	}
      colors[4*i+0] = 255*r;
      colors[4*i+1] = 255*g;
      colors[4*i+2] = 255*b;
    }
  
  setColors();
}

void
TagColorEditor::setOpacity(float op)
{
  uchar *colors = Global::tagColors();

  for(int i=0; i<256; i++)
    {
      int a;
      if (i > 0 && i < 255)
	a = 255*op;
      else
	a = 0;
      a = qMin(a, 255);
      colors[4*i+3] = a;
    }
  
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
  
  setColors();
}

