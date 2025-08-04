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
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QCursor>

#include <algorithm>

TagColorEditor::TagColorEditor()
{
  createGUI();
}

QStringList
TagColorEditor::tagNames()
{
  QStringList tn;
  for (int i=0; i < 256; i++)
    {
      tn << table->item(i, 0)->text();
    }

  return tn;
}

void
TagColorEditor::setTagNames(QStringList tn)
{
  if (tn.count() != 256)
    {
      for (int i=0; i < 256; i++)
	{
	  QString nm = "DEFAULT";
	  if (i > 0 && i < 255) nm = QString("Label %1").arg(i);
	  if (i == 255) nm = "BOUNDARY";
	  table->item(i, 0)->setText(nm);
	}
      return;
    }
  
  for (int i=0; i < 256; i++)
    {
      table->item(i, 0)->setText(tn[i]);
    }
}

void
TagColorEditor::setColors()
{
  QFont fnt("Helvetica", 12);

  uchar *colors = Global::tagColors();
  for (int i=0; i < 256; i++)
    {      
      //int row = i/2;
      //int col = i%2; 
      int row = i;
      int col = 1; 

      int r,g,b;
      float a;

      r = colors[4*i+0];
      g = colors[4*i+1];
      b = colors[4*i+2];

      //--------------------------
      // Label Name
      QTableWidgetItem *tagName = table->item(row, 0);
      if (!tagName)
	{
	  QString tn = "DEFAULT";
	  if (i > 0 && i < 255) tn = QString("Label %1").arg(i);
	  if (i == 255) tn = "BOUNDARY";
	  tagName = new QTableWidgetItem(tn);
	  table->setItem(row, 0, tagName);
	}
      if (colors[4*i+3] > 250)
	tagName->setCheckState(Qt::Checked);
      else
	tagName->setCheckState(Qt::Unchecked);
      //--------------------------

      
      //--------------------------
      // Label Color
      QTableWidgetItem *colorItem = table->item(row, 1);
      if (!colorItem)
	{
	  colorItem = new QTableWidgetItem;
	  colorItem->setFlags(colorItem->flags() ^ Qt::ItemIsEditable);
	  table->setItem(row, 1, colorItem);
	}

      colorItem->setFont(fnt);
      colorItem->setData(Qt::DisplayRole, QString("%1").arg(i));
      colorItem->setBackground(QColor(r,g,b));
    }
}

bool
TagColorEditor::getLowHighRange(int &low, int &high)
{
  // use QMessageBox to display the low and high range
  QMessageBox *box = new QMessageBox(QMessageBox::NoIcon,
				     "Label Range Selection",
				     "Select Low and High",
				     QMessageBox::Ok | QMessageBox::Cancel);
  QLayout *layout = box->layout();
  QWidget *widget = new QWidget();
  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(&m_low);
  hbox->addWidget(&m_high);
  widget->setLayout(hbox);
  ((QGridLayout*)layout)->addWidget(widget, 1, 0, 1, 2);

  m_low.setMaximum(65534);
  m_high.setMaximum(65534);
  
  m_low.setValue(0);
  m_high.setValue(65534);
  
  box->move(QCursor::pos());
  box->setMinimumWidth(300);
  int ret = box->exec();

  if (ret != QMessageBox::Cancel)
    {
      low = m_low.value();
      high = m_high.value();
      return true;
    }
  
  return false;
}

void
TagColorEditor::showTagsClicked()
{
//  uchar *colors = Global::tagColors();
//  int nColors = 256;
//  if (Global::bytesPerMask() == 2)
//    nColors = 65536;
//  
//  for(int i=0; i<nColors; i++)
//    colors[4*i+3] = 255;
//
//  setColors();

  int low, high;
  if (getLowHighRange(low, high))
    {
      uchar *colors = Global::tagColors();      

      for(size_t i=low; i<=high; i++)
	colors[4*i+3] = 255;
      
      setColors();
    }
}

void
TagColorEditor::hideTagsClicked()
{
//  uchar *colors = Global::tagColors();
//  int nColors = 256;
//  if (Global::bytesPerMask() == 2)
//    nColors = 65536;
//  
//  for(int i=0; i<nColors; i++)
//      colors[4*i+3] = 0;
//
//  setColors();

  int low, high;
  if (getLowHighRange(low, high))
    {
      uchar *colors = Global::tagColors();      

      for(size_t i=low; i<=high; i++)
	colors[4*i+3] = 0;
      
      setColors();
    }
}

void
TagColorEditor::createGUI()
{
  table = new QTableWidget(256, 2);
  table->resize(300, 300);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers); // disable number editing
  table->verticalHeader()->hide();
  table->horizontalHeader()->hide();
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setStyleSheet("QTableWidget{gridline-color:white;}");

  setColors();

  for (int i=0; i < table->rowCount(); i++)
    table->setRowHeight(i, 30);

  for (int i=0; i < table->columnCount(); i++)
    table->setColumnWidth(i, 100);

  QPushButton *newTags = new QPushButton("New Label Colors");
  QPushButton *showTags = new QPushButton("Show Labels");
  QPushButton *hideTags = new QPushButton("Hide Labels");

  QHBoxLayout *hlayout = new QHBoxLayout;
  hlayout->addWidget(newTags);
  hlayout->addWidget(showTags);
  hlayout->addWidget(hideTags);
  QVBoxLayout *layout = new QVBoxLayout;
  layout->addLayout(hlayout);
  layout->addWidget(table);
  
  setLayout(layout);
  
  setWindowTitle(tr("Label Color Editor"));

  connect(newTags, SIGNAL(clicked()),
	  this, SLOT(newTagsClicked()));

  connect(showTags, SIGNAL(clicked()),
	  this, SLOT(showTagsClicked()));

  connect(hideTags, SIGNAL(clicked()),
	  this, SLOT(hideTagsClicked()));

  connect(table, SIGNAL(cellClicked(int, int)),
	  this, SLOT(cellClicked(int, int)));

  connect(table, SIGNAL(cellDoubleClicked(int, int)),
	  this, SLOT(cellDoubleClicked(int, int)));

  connect(table, SIGNAL(cellChanged(int, int)),
	  this, SLOT(cellChanged(int, int)));
}

void
TagColorEditor::newTagsClicked()
{
  QStringList items;
  items << "Random";
  items << "Qualitative";
  items << "Sequential";
  items << "Sequential_r";
  items << "Local Thickness";
  items << "Local Thickness_r";

  QString text(items.value(0));  
  QInputDialog dialog(this, 0);
  dialog.move(QCursor::pos());
  dialog.setWindowTitle("Colors");
  dialog.setLabelText("Colors");
  dialog.setComboBoxItems(items);
  dialog.setTextValue(text);
  dialog.setComboBoxEditable(false);
  dialog.setInputMethodHints(Qt::ImhNone);
  int ret = dialog.exec();
  if (!ret)
    return;

  QString str = dialog.textValue();


  if (str == "Qualitative")    
    newColorSet(1);
  else if (str == "Sequential")    
    newColorSet(2);
  else if (str == "Sequential_r")    
    newColorSet(3);
  else if (str == "Local Thickness")    
    newColorSet(4);
  else if (str == "Local Thickness_r")    
    newColorSet(5);
  else
    newColorSet(QTime::currentTime().msec());

  emit tagColorChanged();
}

void
TagColorEditor::cellClicked(int row, int col)
{
  int index = row;

  uchar *colors = Global::tagColors();

  QTableWidgetItem *item = table->item(row, 0);
  bool checkBoxClicked = false;
  if (item->checkState() == Qt::Checked)
    {
      checkBoxClicked = colors[4*index+3] < 200; // previously unchecked      
      colors[4*index+3] = 255;
    }
  else
    {
      checkBoxClicked = colors[4*index+3] > 200; // previously checked
      colors[4*index+3] = 0;
    }

  emit tagSelected(index, checkBoxClicked);
}

void
TagColorEditor::cellChanged(int row, int col)
{
  if (col == 0)
    {
      emit tagNamesChanged();
    }
}

void
TagColorEditor::cellDoubleClicked(int row, int col)
{
  if (col == 0)
    {
      if (row == 0 || row == 255)
	return;
      
      QTableWidgetItem *item = table->item(row, 0);
      table->editItem(item);

      return;
    }
  
  QTableWidgetItem *item = table->item(row, 1);
  uchar *colors = Global::tagColors();

  int index = row;

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
  if (h == 1) // qualitative
    {
      askGradientChoice(-2, -1, false);
      return;
    }
  else if (h == 2) // sequential
    {
      askGradientChoice(-1, -1, false);
      return;
    }
  if (h == 3) // sequential reversed
    {
      askGradientChoice(-1, -1, true);
      return;
    }
  else if (h == 4) // local thickness
    {
      askGradientChoice(64000, 1000, false);
      return;
    }
  else if (h == 5) // local thickness reversed
    {
      askGradientChoice(64000, 1000, true);
      return;
    }

  qsrand(h);

  uchar *colors = Global::tagColors();
  int nColors = 255;
  if (Global::bytesPerMask() == 2)
    //nColors = 65536;
    nColors = 64000;  // beyond 64000 is for other purposes such as local thickness etc.
  
  for(int i=1; i<nColors; i++)
    {
      float r,g,b;
      r = (float)qrand()/(float)RAND_MAX;
      g = (float)qrand()/(float)RAND_MAX;
      b = (float)qrand()/(float)RAND_MAX;
      float mm = qMax(r,qMax(g,b));
      if (mm < 0.8) // don't want too dark
	{
	  if (mm < 0.1)
	    {
	      r = g = b = 1.0;
	    }
	  else if (mm < 0.3)
	    {
	      r = 1 - r;
	      g = 1 - g;
	      b = 1 - b;
	    }
	  else
	    {
	      r *= 0.8/mm;
	      g *= 0.8/mm;
	      b *= 0.8/mm;
	    }
	}
      colors[4*i+0] = 255*r;
      colors[4*i+1] = 255*g;
      colors[4*i+2] = 255*b;
    }
  colors[0] = 255;
  colors[1] = 255;
  colors[2] = 255;
  //colors[3] = 255;
  
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
TagColorEditor::askGradientChoice(int startIndex, int mapSize, bool reversed)
{
  bool ok;

  QList<QColor> cmap;
  if (startIndex == -2)
    cmap = m_colorMaps.getColorMap(0); // qualitative color map
  else
    cmap = m_colorMaps.getColorMap(1); // sequential color map

  if (cmap.count() == 0)
    return;

  
  if (reversed)
    std::reverse(cmap.begin(), cmap.end());


  //---------
  int nColors = 255;
  if (Global::bytesPerMask() == 2)
    nColors = 65536;

  if (mapSize < 0)
    {
      mapSize = QInputDialog::getInt(0,
				     "Number of Colors",
				     "Number of Colors",
				     50, 2, nColors, 1, &ok);
      if (!ok)
	mapSize = 50;
      //---------


      //---------
      startIndex = QInputDialog::getInt(0,
					"Starting Label Index",
					QString("Starting Label Index between 1 and %1").arg(65534-mapSize),
					1, 1, 65534-mapSize, 1, &ok);
      if (!ok)
	startIndex = 1;
    }  
  //---------

  setColorGradient(cmap, startIndex, mapSize);
}

void
TagColorEditor::setColorGradient(QList<QColor> cmap,
				 int startIndex,
				 int mapSize)
{
  QGradientStops stops;
  int csz = cmap.count();
  for(int i=0; i<csz; i++)
    {
      float pos = (float)i/(float)(csz-1);
      stops << QGradientStop(pos, cmap[i]);
    }

  
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

      int cidx = startIndex + i;
      colors[4*cidx+0] = r;
      colors[4*cidx+1] = g;
      colors[4*cidx+2] = b;
    }
  
  colors[0] = 255;
  colors[1] = 255;
  colors[2] = 255;
  
  colors[4*65535 + 0] = 255;
  colors[4*65535 + 1] = 0;
  colors[4*65535 + 2] = 0;
  
  setColors();
}
