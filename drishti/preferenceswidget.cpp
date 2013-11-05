#include "preferenceswidget.h"
#include "tick.h"
#include "global.h"
#include "dcolordialog.h"
#include "propertyeditor.h"

#include <QFile>
#include <QTextStream>
#include <QDomDocument>

PreferencesWidget::PreferencesWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  ui.m_still->setRange(-5, 16);
  ui.m_drag->setRange(-5, 16);
  setImageQualitySliderValues(7, 6);

  ui.m_tickSize->setRange(0, 20);
  ui.m_tickStep->setRange(0, 2000);

  setTick(0, 10, "X", "Y", "Z");

  Tick::setTickSize(ui.m_tickSize->value());
  Tick::setTickStep(ui.m_tickStep->value());
  Tick::setLabelX(ui.m_labelX->text());
  Tick::setLabelY(ui.m_labelY->text());
  Tick::setLabelZ(ui.m_labelZ->text());

  ui.m_eyeSeparation->setText("0.062");

  ui.m_focus->setValue(50);

  QWidget *tab = ui.tabWidget->widget(3);
  m_tagColorEditor = new TagColorEditor();
  tab->layout()->addWidget(m_tagColorEditor);

  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  this, SIGNAL(tagColorChanged()));
  connect(m_tagColorEditor, SIGNAL(tagColorChanged()),
	  this, SIGNAL(updateGL()));

  updateTextureMemory();
}

void
PreferencesWidget::updateTagColors()
{
  m_tagColorEditor->setColors();
}


void
PreferencesWidget::on_m_newColorSet_clicked()
{
  m_tagColorEditor->newColorSet(ui.m_colorSeed->value());
}

void
PreferencesWidget::on_m_setOpacity_clicked()
{
  m_tagColorEditor->setOpacity(ui.m_opacity->value());
}

void
PreferencesWidget::updateTextureMemory()
{
  int tms = Global::textureMemorySize();
  ui.m_textureMemorySize->setValue(tms);
}

void
PreferencesWidget::on_m_textureMemorySize_valueChanged(int tms)
{
  Global::setTextureMemorySize(tms);
  Global::calculate3dTextureSize();
}

void
PreferencesWidget::updateStereoSettings(float dist, float es, float width)
{
  m_focusDistance = dist;
  m_eyeSeparation = es;
  m_minFocusDistance = dist/10; // more control over zoom in
  m_maxFocusDistance = 2*dist; // less control over zoom out
  ui.m_focus->setValue(50);

  ui.m_screenWidth->setValue(width);
  ui.m_eyeSeparation->setText(QString("%1").arg(es));
  ui.label_8->setText(QString("Focus Distance (%1)").arg(dist));

  update();
}

void
PreferencesWidget::updateFocusSetting(float dist)
{
  ui.label_8->setText(QString("Focus Distance (%1)").arg(dist));

  int val = 50;  
  if (dist > m_focusDistance)
    {
      float frc = (dist-m_focusDistance)/
	          (m_maxFocusDistance-m_focusDistance);
      frc = qBound(0.0f, frc, 1.0f);
      val = 50 + frc*50;
    }
  else
    {
      float frc = (m_focusDistance-dist)/
	          (m_focusDistance-m_minFocusDistance);
      frc = qBound(0.0f, frc, 1.0f);
      val = (1-frc)*50;
    }

  ui.m_focus->setValue(val);

  update();
}

void
PreferencesWidget::enableStereo(bool flag)
{
  ui.m_stereoSettings->setEnabled(flag);
  update();
}

void
PreferencesWidget::changeStill(int val)
{
  int sv = ui.m_still->value() + val;
  ui.m_still->setValue(sv);
  if (ui.m_drag->value() > sv)
    ui.m_drag->setValue(sv);

  setImageQualityStepsizes();
  update();
}
void
PreferencesWidget::changeDrag(int val)
{
  int sv = ui.m_drag->value() + val;
  ui.m_drag->setValue(sv);
  if (ui.m_still->value() < sv)
    ui.m_still->setValue(sv);

  setImageQualityStepsizes();
  update();
}

int
PreferencesWidget::renderQualityValue(float v)
{
  int iv;
  if (v <= 1.0)
    iv = (1.7-v)*10;
  else
    iv = 8 - v;

  return iv;
}

void
PreferencesWidget::setRenderQualityValues(float still, float drag)
{
  int sv = renderQualityValue(still);
  int dv = renderQualityValue(drag);
  ui.m_still->setValue(sv);
  ui.m_drag->setValue(dv);
  update();
}

void
PreferencesWidget::setImageQualitySliderValues(int still, int drag)
{
  ui.m_still->setValue(still);
  ui.m_drag->setValue(drag);
  update();
}

void
PreferencesWidget::getTick(int& sz, int& st,
			   QString& xl, QString& yl, QString& zl)
{
  sz = ui.m_tickSize->value();
  st = ui.m_tickStep->value();
  xl = ui.m_labelX->text();
  yl = ui.m_labelY->text();
  zl = ui.m_labelZ->text();
}

void
PreferencesWidget::setTick(int sz, int st,
			   QString xl, QString yl, QString zl)
{
  ui.m_tickSize->setValue(sz);
  ui.m_tickStep->setValue(st);
  ui.m_labelX->setText(xl);
  ui.m_labelY->setText(yl);
  ui.m_labelZ->setText(zl);
  update();
}

float
PreferencesWidget::imageQualityValue(int iv)
{
  float v = iv;
  if (v >= 7)
    v = 1.7 - 0.1*v;
  else
    v = 8 - v;

  return v;
}

void
PreferencesWidget::setImageQualityStepsizes()
{
  float v;

  v = imageQualityValue(ui.m_drag->value());
  Global::setStepsizeDrag(v);

  v = imageQualityValue(ui.m_still->value());
  Global::setStepsizeStill(v);

  emit updateLookupTable();
  emit updateGL();
}

void
PreferencesWidget::on_m_still_sliderReleased()
{
  int sv = ui.m_still->value();
  if (ui.m_drag->value() > sv)
    ui.m_drag->setValue(sv);

  setImageQualityStepsizes();
}

void
PreferencesWidget::on_m_drag_sliderReleased()
{  
  int sv = ui.m_drag->value();
  if (ui.m_still->value() < sv)
    ui.m_still->setValue(sv);

  setImageQualityStepsizes();
}

void
PreferencesWidget::on_m_tickSize_valueChanged(int v)
{
  Tick::setTickSize(v);
  emit updateGL();
}

void
PreferencesWidget::on_m_tickStep_valueChanged(int v)
{
  Tick::setTickStep(v);
  emit updateGL();
}

void
PreferencesWidget::on_m_labelX_editingFinished()
{
  Tick::setLabelX(ui.m_labelX->text());
  emit updateGL();
}

void
PreferencesWidget::on_m_labelY_editingFinished()
{
  Tick::setLabelY(ui.m_labelY->text());
  emit updateGL();
}

void
PreferencesWidget::on_m_labelZ_editingFinished()
{
  Tick::setLabelZ(ui.m_labelZ->text());
  emit updateGL();
}

void
PreferencesWidget::on_m_eyeSeparation_editingFinished()
{
  on_m_focus_sliderReleased();
}

void
PreferencesWidget::on_m_focus_sliderReleased()
{
  float dist;
  int val = ui.m_focus->value();

  if (val > 50)
    {
      float frc = val/50.0f - 1.0f;
      dist = m_focusDistance*(1-frc) + frc*m_maxFocusDistance;
    }
  else
    {
      float frc = 1.0f - val/50.0f;
      dist = m_focusDistance*(1-frc) + frc*m_minFocusDistance;
    }
  float width = ui.m_screenWidth->value();
  float es = ui.m_eyeSeparation->text().toFloat();
  emit stereoSettings(dist, es, width);

  ui.label_8->setText(QString("Focus Distance (%1)").arg(dist));
}

void
PreferencesWidget::on_m_screenWidth_valueChanged(double)
{
  float dist;
  int val = ui.m_focus->value();
  if (val > 50)
    {
      float frc = val/50.0f - 1.0f;
      dist = m_focusDistance*(1-frc) + frc*m_maxFocusDistance;
    }
  else
    {
      float frc = 1.0f - val/50.0f;
      dist = m_focusDistance*(1-frc) + frc*m_minFocusDistance;
    }
  float width = ui.m_screenWidth->value();
  float es = ui.m_eyeSeparation->text().toFloat();
  emit stereoSettings(dist, es, width);
}

void
PreferencesWidget::on_m_bgcolor_clicked()
{
  Vec bg = Global::backgroundColor();
  QColor bgc = QColor(255*bg.x,
		      255*bg.y,
		      255*bg.z);

  QColor color = DColorDialog::getColor(bgc);
  if (color.isValid())
    {      
      bg = Vec(color.redF(),
	       color.greenF(),
	       color.blueF());
      Global::setBackgroundColor(bg);
      emit updateGL();
    }
}

void
PreferencesWidget::save(const char* flnm)
{
  QDomDocument doc;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }
  QDomElement topElement = doc.documentElement();

  QDomElement de = doc.createElement("preferences");
  QString str;

  {
    QDomElement dec = doc.createElement("backgroundcolor");
    Vec bc = Global::backgroundColor();
    str = QString("%1 %2 %3").arg(bc.x).arg(bc.y).arg(bc.z);
    QDomText tnc = doc.createTextNode(str);
    dec.appendChild(tnc);
    de.appendChild(dec);
  }

  { 
    float imgq;
    int idrag, istill;

    imgq = Global::stepsizeStill();
    if (imgq >= 1)
      istill = 8 - imgq;
    else
      istill = 17 - 10*imgq;

    imgq = Global::stepsizeDrag();
    if (imgq >= 1)
      idrag = 8 - imgq;
    else
      idrag = 17 - 10*imgq;

    QDomElement de0 = doc.createElement("imagequality");
    str = QString("%1 %2").			\
                  arg(istill).arg(idrag);
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  { 
    QDomElement de0 = doc.createElement("eyeseparation");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_eyeSeparation));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  { 
    QDomElement de0 = doc.createElement("tick");
    str = QString("%1 %2").			\
                  arg(Tick::tickSize()).arg(Tick::tickStep());
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  { 
    QDomElement de0 = doc.createElement("labelx");
    QDomText tn0 = doc.createTextNode(Tick::labelX());
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  { 
    QDomElement de0 = doc.createElement("labely");
    QDomText tn0 = doc.createTextNode(Tick::labelY());
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  { 
    QDomElement de0 = doc.createElement("labelz");
    QDomText tn0 = doc.createTextNode(Tick::labelZ());
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("tagcolors");
    QString str;
    uchar *colors = Global::tagColors();
    for(int i=0; i<256; i++)
      {
	if (i > 0) str += "               ";
	str += QString(" %1 %2 %3 %4\n").\
	              arg(colors[4*i+0]).\
	              arg(colors[4*i+1]).\
	              arg(colors[4*i+2]).\
	              arg(colors[4*i+3]);
      }
    QDomText tn0 = doc.createTextNode(str);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  topElement.appendChild(de);


  QFile fout(flnm);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      doc.save(out, 2);
      fout.close();
    }
}

void 
PreferencesWidget::load(const char* flnm)
{
  QDomDocument document;
  QFile f(flnm);
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList tlist = main.childNodes();
  for(int i=0; i<tlist.count(); i++)
    {
      if (tlist.at(i).nodeName() == "preferences")
	{
	  QDomNodeList dlist = tlist.at(i).childNodes();
	  for(int i=0; i<dlist.count(); i++)
	    {
	      if (dlist.at(i).nodeName() == "backgroundcolor")
		{
		  QString str = dlist.at(i).toElement().text();
		  QStringList xyz = str.split(" ");
		  float x,y,z;
		  if (xyz.size() > 0) x = xyz[0].toFloat();
		  if (xyz.size() > 1) y  = xyz[1].toFloat();
		  if (xyz.size() > 2) z  = xyz[2].toFloat();
		  Global::setBackgroundColor(Vec(x,y,z));
		}
	      else if (dlist.at(i).nodeName() == "imagequality")
		{
		  if (!Global::batchMode())
		    {
		      QString str = dlist.at(i).toElement().text();
		      QStringList xy = str.split(" ");
		      int istill = 7, idrag = 6;
		      if (xy.size() > 0) istill = xy[0].toInt();
		      if (xy.size() > 1) idrag  = xy[1].toInt();
		      setImageQualitySliderValues(istill, idrag);
		      Global::setStepsizeStill(imageQualityValue(istill));
		      Global::setStepsizeDrag(imageQualityValue(idrag));
		    }
		}
	      else if (dlist.at(i).nodeName() == "eyeseparation")
		ui.m_eyeSeparation->setText(dlist.at(i).toElement().text());
	      else if (dlist.at(i).nodeName() == "tick")
		{
		  QString str = dlist.at(i).toElement().text();
		  QStringList xy = str.split(" ");
		  if (xy.size() > 0) ui.m_tickSize->setValue(xy[0].toInt());
		  if (xy.size() > 1) ui.m_tickStep->setValue(xy[1].toInt());
		}
	      else if (dlist.at(i).nodeName() == "labelx")
		ui.m_labelX->setText(dlist.at(i).toElement().text());
	      else if (dlist.at(i).nodeName() == "labely")
		ui.m_labelY->setText(dlist.at(i).toElement().text());
	      else if (dlist.at(i).nodeName() == "labelz")
		ui.m_labelZ->setText(dlist.at(i).toElement().text());
	      else if (dlist.at(i).nodeName() == "tagcolors")
		{
		  QString str = dlist.at(i).toElement().text();
		  QStringList col = str.split("\n",
					      QString::SkipEmptyParts);
		  uchar *colors = Global::tagColors();
		  for(int i=0; i<qMin(256, col.size()); i++)
		    {
		      QStringList clr = col[i].split(" ",
						     QString::SkipEmptyParts);
		      colors[4*i+0] = clr[0].toInt();
		      colors[4*i+1] = clr[1].toInt();
		      colors[4*i+2] = clr[2].toInt();
		      colors[4*i+3] = clr[3].toInt();
		    }
		  Global::setTagColors(colors);
		  m_tagColorEditor->setColors();
		}
	    }
	}
    }

  Tick::setTickSize(ui.m_tickSize->value());
  Tick::setTickStep(ui.m_tickStep->value());
  Tick::setLabelX(ui.m_labelX->text());
  Tick::setLabelY(ui.m_labelY->text());
  Tick::setLabelZ(ui.m_labelZ->text());

  if (!Global::batchMode())
    {
      Global::setStepsizeDrag(imageQualityValue(ui.m_drag->value()));
      Global::setStepsizeStill(imageQualityValue(ui.m_still->value()));
    }
}


void
PreferencesWidget::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_H &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() & Qt::MetaModifier) )
    showHelp();
}

void
PreferencesWidget::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/preferences.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }  
  plist["commandhelp"] = vlist;

  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Preferences Help", plist, keys);
  propertyEditor.exec();
}
