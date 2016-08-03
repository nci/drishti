#include "propertyeditor.h"
#include "dcolordialog.h"
#include "gradienteditorwidget.h"

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QScrollArea>


PropertyEditor::PropertyEditor(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);
  resize(700, 400);

  m_signalMapper = new QSignalMapper(this);


  m_hotkeymouse = new QCheckBox("HotKey + Mouse Help");
  connect(m_hotkeymouse, SIGNAL(clicked(bool)),
	  this, SLOT(hotkeymouseClicked(bool)));
  
  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(m_hotkeymouse);

  QSplitter *splitter = new QSplitter(Qt::Vertical, ui.commandHelp);
  m_helpList = new QListWidget;
  m_helpLabel = new QTextEdit;
  m_helpLabel->setReadOnly(true);

  splitter->addWidget(m_helpList);
  splitter->addWidget(m_helpLabel);
  splitter->setChildrenCollapsible(false);
  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 2);

  vbox->addWidget(splitter);
  ui.commandHelp->setLayout(vbox);


  connect(m_helpList, SIGNAL(currentRowChanged(int)),
	  this, SLOT(helpItemSelected(int)));

  splitter->setStyleSheet("QSplitter::handle {image: url(:/images/sizegrip.png);}");
  ui.splitter->setStyleSheet("QSplitter::handle {image: url(:/images/sizegrip.png);}");
  ui.splitter_2->setStyleSheet("QSplitter::handle {image: url(:/images/sizegrip.png);}");
}

PropertyEditor::~PropertyEditor() { }

void
PropertyEditor::set(QString title,
		    QMap<QString, QVariantList> plist,
		    QStringList keys,
		    bool addReset)
{
  setWindowTitle(title);

  m_plist = plist;
  m_keys = keys;
  m_widgets.clear();

  m_helpText.clear();
  m_helpCommandString.clear();

  ui.propertyBox->hide();
  ui.messageLabel->hide();
  ui.commandString->hide();
  ui.commandHelp->hide();

  m_hotkeymouse->hide();
  m_helpLabel->hide();
  m_helpList->hide();

  QGridLayout *gridLayout = new QGridLayout;
  //gridLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  gridLayout->setSizeConstraint(QLayout::SetMinimumSize);

  QWidget *viewport = new QWidget;
  viewport->setLayout(gridLayout);
  viewport->setMinimumSize(100, 100);
  viewport->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

  QScrollArea *scrollArea = new QScrollArea;
  scrollArea->setWidget(viewport);
  scrollArea->setWidgetResizable(true);

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(scrollArea);
  ui.propertyBox->setLayout(vbox);

  resize(700, qBound(1, keys.count(), 25)*30);


  QStringList pkeys = plist.keys();
  for(int i=0; i<keys.count(); i++)
    {
      QVariantList vlist;

      if (pkeys.contains(keys[i]))
	vlist = plist.value(keys[i]);
      else
	vlist.clear();
      
      if (keys[i] == "command")
	{
	  ui.commandString->show();
	  if (vlist.count() > 0)
	    ui.lineEdit->setText(vlist[0].toString());
	}
      else if (vlist.count() == 0) // enable command string
	{
	  if (keys[i] == "gap")
	    {
	      QSpacerItem *gap = new QSpacerItem(10,10);
	      gridLayout->addItem(gap,i,0,1,2);
	    }
	}
      else if (keys[i] == "message")
	{
	  QString mesgText = vlist[0].toString();
 	  ui.messageLabel->show();
	  ui.messageLabel->setPlainText(mesgText);
	}
      else if (keys[i] == "commandhelp") // enable command help string
	{
	  ui.commandHelp->show();
	  m_helpLabel->show();

	  if (vlist.count() == 1)
	    {
	      m_helpLabel->setText(vlist[0].toString());
	    }
	  else
	    {
	      m_helpList->show();

	      m_helpText.clear();
	      m_helpCommandString.clear();
	      m_orighelpCS.clear();

	      QStringList hcsList;
	      QStringList keyText;
	      bool startKeyboardHelp = false;
	      for(int iv=0; iv<vlist.count()/2; iv++)
		{
		  QString hcs = vlist[2*iv].toString();
		  if (hcs == "keyboard" && !startKeyboardHelp)
		    startKeyboardHelp = true;
		  else
		    {
		      if (!startKeyboardHelp)
			hcsList << hcs;
		      else
			keyText << hcs;

		      m_orighelpCS << hcs;
		      m_helpText << vlist[2*iv+1].toString();
		    }
		}
	      m_helpCommandString << hcsList;
	      m_helpCommandString.sort();
	      m_helpList->addItems(m_helpCommandString);

	      if (keyText.count() > 0)
		{
		  m_hotkeymouse->show();
		  m_hotkeymouse->setChecked(false);
		  m_hotkeymouseString << keyText;
		  m_hotkeymouseString.sort();
		}
	    }
	}
      else
	{
	  ui.propertyBox->show();
	  QList<int> ss;
	  ss << 1;
	  ss << 1;
	  ui.splitter_2->setSizes(ss);

	  QLabel *lbl = new QLabel(keys[i]);
	  gridLayout->addWidget(lbl, i, 0);
	  
	  if (vlist[0] == "int")
	    {
	      QSpinBox *sbox = new QSpinBox();
	      if (vlist.count() > 3)
		sbox->setRange(vlist[2].toInt(),
			       vlist[3].toInt());
	      if (vlist.count() > 1)
		sbox->setValue(vlist[1].toInt());
	      
	      gridLayout->addWidget(sbox, i, 1);
	      
	      m_widgets[keys[i]] = sbox;
	    }
	  else if (vlist[0] == "double" ||
		   vlist[0] == "float")
	    {
	      QDoubleSpinBox *sbox = new QDoubleSpinBox();
	      if (vlist.count() > 3)
		sbox->setRange(vlist[2].toDouble(), 
			       vlist[3].toDouble());
	      if (vlist.count() > 1)
		sbox->setValue(vlist[1].toDouble());
	      if (vlist.count() > 4)
		sbox->setSingleStep(vlist[4].toDouble());
	      if (vlist.count() > 5)
		sbox->setDecimals(vlist[5].toInt());
	      
	      gridLayout->addWidget(sbox, i, 1);
	      m_widgets[keys[i]] = sbox;
	    }
	  else if (vlist[0] == "string")
	    {
	      QLineEdit *ledit = new QLineEdit();
	      if (vlist.count() > 1)
		ledit->setText(vlist[1].toString());
	      
	      gridLayout->addWidget(ledit, i, 1);
	      m_widgets[keys[i]] = ledit;
	    }
	  else if (vlist[0] == "color")
	    {
	      QString clbl = QString("<a href=\"") +
	                     QString("%1 ").arg(i) +
	                     vlist[1].value<QColor>().name() +
	                     QString("\">") +
	                     vlist[1].value<QColor>().name() +
	                     QString("</a>");

	      QLabel *txt = new QLabel(clbl);
	      txt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	      txt->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
	      gridLayout->addWidget(txt, i, 1);
	      
	      connect(txt, SIGNAL(linkActivated(const QString&)),
		      this, SLOT(changeColor(const QString&)));
	      
	      m_widgets[keys[i]] = txt;
	    }
	  else if (vlist[0] == "combobox")
	    {
	      QComboBox *combo = new QComboBox();
	      QStringList slist;
	      for(int j=0; j<vlist.count()-2; j++)
		slist << vlist[j+2].toString();
	      combo->addItems(slist);
	      combo->setCurrentIndex(vlist[1].toInt());
	      
	      gridLayout->addWidget(combo, i, 1);
	      m_widgets[keys[i]] = combo;
	    }
	  else if (vlist[0] == "checkbox")
	    {
	      QCheckBox *check = new QCheckBox();
	      if (vlist.count() > 1)
		{
		  if (vlist[1].toBool())
		    check->setCheckState(Qt::Checked);
		  else
		    check->setCheckState(Qt::Unchecked);
		}
	      
	      gridLayout->addWidget(check, i, 1);
	      m_widgets[keys[i]] = check;

	      if (keys[i] == "SELECT ALL")
		connect(check, SIGNAL(stateChanged(int)),
			this, SLOT(selectAll(int)));
	    }
	  else if (vlist[0] == "colorgradient")
	    {
	      GradientEditorWidget *gew = new GradientEditorWidget();
	      gew->setMinimumSize(200, 50);
	      QGradientStops stops;
	      for(int j=0; j<(vlist.count()-1)/5; j++)
		{
		  float pos = vlist[1+5*j+0].toDouble();
		  float r = vlist[1+5*j+1].toInt();
		  float g = vlist[1+5*j+2].toInt();
		  float b = vlist[1+5*j+3].toInt();
		  float a = vlist[1+5*j+4].toInt();
		  stops << QGradientStop(pos, QColor(r,g,b,a));
		}
	      gew->setColorGradient(stops);
	      gridLayout->addWidget(gew, i, 1);
	      m_widgets[keys[i]] = gew;
	    }
	  else
	    {
	      QMessageBox::information(0, "What ??", vlist[0].toString());
	    }
	}
    }

  if (ui.propertyBox->isHidden() &&
      ui.commandString->isHidden() &&
      ui.messageLabel->isHidden())
    {
      QList<int> sz;
      sz << 0;
      sz << 1;
      ui.splitter_2->setSizes(sz);
    }
}

QString PropertyEditor::getCommandString() { return ui.lineEdit->text(); }

QGradientStops
PropertyEditor::getGradientStops(QString kstr)
{
  QStringList keys = m_plist.keys();
  for(int i=0; i<keys.count(); i++)
    {
      if (keys[i] == kstr)
	{
	  GradientEditorWidget *gew = (GradientEditorWidget*)m_widgets[keys[i]];
	  return gew->colorGradient();
	}
    }
  return QGradientStops();
}

QMap<QString, QPair<QVariant, bool> > 
PropertyEditor::get()
{
  QMap<QString, QPair<QVariant, bool> > vmap;

  QStringList keys = m_plist.keys();
  for(int i=0; i<keys.count(); i++)
    {
      QVariantList vlist = m_plist.value(keys[i]);

      if (vlist.count() > 0)
	{
	  if (vlist[0] == "int")
	    {
	      int val = ((QSpinBox*)m_widgets[keys[i]])->value();
	      if (val != vlist[1].toInt())
		vmap[keys[i]] = qMakePair(QVariant(val), true);
	      else
		vmap[keys[i]] = qMakePair(QVariant(val), false);
	    }
	  else if (vlist[0] == "double" || vlist[0] == "float")
	    {
	      double val = ((QDoubleSpinBox*)m_widgets[keys[i]])->value();
	      if (val != vlist[1].toDouble())
		vmap[keys[i]] = qMakePair(QVariant(val), true);
	      else
		vmap[keys[i]] = qMakePair(QVariant(val), false);
	    }
	  else if (vlist[0] == "string")
	    {
	      QString val = ((QLineEdit*)m_widgets[keys[i]])->text();
	      if (val != vlist[1].toString())
		vmap[keys[i]] = qMakePair(QVariant(val), true);
	      else
		vmap[keys[i]] = qMakePair(QVariant(val), false);
	    }
	  else if (vlist[0] == "color")
	    {
	      QString cstr = ((QLabel*)m_widgets[keys[i]])->text();
	      QStringList slist = cstr.split(">");
	      QString cname = slist[1];
	      cname.resize(7);
	      QColor val = QColor(cname);
	      QVariant vcol = val;
	      if (cname != vlist[1].value<QColor>().name())
		vmap[keys[i]] = qMakePair(vcol, true);
	      else
		vmap[keys[i]] = qMakePair(vcol, false);
	    }
	  else if (vlist[0] == "combobox")
	    {
	      int val = ((QComboBox*)m_widgets[keys[i]])->currentIndex();
	      if (val != vlist[1].toInt())
		vmap[keys[i]] = qMakePair(QVariant(val), true);
	      else
		vmap[keys[i]] = qMakePair(QVariant(val), false);
	    }	
	  else if (vlist[0] == "checkbox")
	    {
	      bool val = false;
	      if (((QCheckBox*)m_widgets[keys[i]])->checkState() == Qt::Checked)
		val = true;
	      if (val != vlist[1].toBool())
		vmap[keys[i]] = qMakePair(QVariant(val), true);
	      else
		vmap[keys[i]] = qMakePair(QVariant(val), false);
	    }
	  else if (vlist[0] == "colorgradient")
	    {
	      vmap[keys[i]] = qMakePair(QVariant(1), true);
	    }
	}
    }

  return vmap;
}

void
PropertyEditor::changeColor(const QString& rgb)
{
  QStringList slist = rgb.split(" ");
  int objIdx = slist[0].toInt();
  QColor pcolor(slist[1]);
  QColor color = DColorDialog::getColor(pcolor);
  if (color.isValid())
    {
      QString clbl = QString("<a href=\"") +
	             QString("%1 ").arg(objIdx) +
	             color.name() +
	             QString("\">") +
	             color.name() +
	             QString("</a>");

      QStringList keys = m_plist.keys();
      QLabel *lbl = (QLabel *)m_widgets[m_keys[objIdx]];
      lbl->setText(clbl);
    }
}

void
PropertyEditor::resetProperty(int i)
{
  if (i >= m_plist.count())
    return;

  QString key = m_plist.keys()[i];
  QVariantList vlist = m_plist.value(key);
  
  if (vlist[0] == "int")
    ((QSpinBox*)m_widgets[key])->setValue(vlist[1].toInt());
  else if (vlist[0] == "double" ||
	   vlist[0] == "float")
    ((QDoubleSpinBox*)m_widgets[key])->setValue(vlist[1].toDouble());
  else if (vlist[0] == "string")
    ((QLineEdit*)m_widgets[key])->setText(vlist[1].toString());
  else if (vlist[0] == "color")
    {
      QString clbl = QString("<a href=\"") +
		     QString("%1 ").arg(i) +
		     vlist[1].value<QColor>().name() +
		     QString("\">") +
		     vlist[1].value<QColor>().name() +
		     QString("</a>");
      ((QLabel*)m_widgets[key])->setText(clbl);
    }
  else if (vlist[0] == "combobox")
    ((QComboBox*)m_widgets[key])->setCurrentIndex(vlist[1].toInt());
  else if (vlist[0] == "checkbox")
    {
      if (vlist.count() > 1)
	{
	  if (vlist[1].toBool())
	    ((QCheckBox*)m_widgets[key])->setCheckState(Qt::Checked);
	  else
	    ((QCheckBox*)m_widgets[key])->setCheckState(Qt::Unchecked);
	}
      else
	((QCheckBox*)m_widgets[key])->setCheckState(Qt::Unchecked);
    }
}

void
PropertyEditor::helpItemSelected(int row)
{
  if (row >= 0)
    {
      QString cmd;
      if (m_hotkeymouse->isChecked())
	cmd = m_hotkeymouseString[row];
      else
	cmd = m_helpCommandString[row];

      int r = m_orighelpCS.indexOf(cmd);
      m_helpLabel->setText(m_helpText[r]);

      if (! m_hotkeymouse->isChecked())
	ui.lineEdit->setText(cmd);
    }
}

void
PropertyEditor::hotkeymouseClicked(bool checked)
{
  if (checked)
    {
      m_helpList->clear();
      m_helpList->addItems(m_hotkeymouseString);
      m_helpList->setCurrentRow(0);
      helpItemSelected(0);
    }
  else
    {
      m_helpList->clear();
      m_helpList->addItems(m_helpCommandString);
    }
}

void
PropertyEditor::selectAll(int s)
{   
  bool st = false;
  st = (s == Qt::Checked);
  QList<QWidget*> widgets = m_widgets.values();
  for(int i=0; i<m_widgets.count(); i++)
    {
      if (widgets[i]->inherits("QCheckBox"))
	{
	  ((QCheckBox*)widgets[i])->setChecked(st);
	}
    }
}
