#include "lightingwidget.h"
#include "propertyeditor.h"
#include "global.h"

LightingWidget::LightingWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

//  m_lightPosition = new DirectionVectorWidget(ui.lightposition);  
//  m_lightPosition->setRange(-1.0, 3.0, 0.0);
//
//  connect(m_lightPosition, SIGNAL(directionChanged(float, Vec)),
//	  this, SLOT(lightDirectionChanged(float, Vec)));

  setFlat();
}

void
LightingWidget::setFlat()
{
  ui.lightposition->hide();

  if (ui.applylighting->isChecked())
    {
      ui.lightingbox->show();
      ui.applylighting->setFlat(false);
    }
  else
    {
      ui.lightingbox->hide();
      ui.applylighting->setFlat(true);
    }

  ui.applycoloredshadow->hide();

  ui.applybackplane->hide();


  ui.applyshadow->hide();

  if (ui.peel->isChecked())
    {
      ui.peelbox->show();
      ui.peel->setFlat(false);
    }
  else
    {
      ui.peelbox->hide();
      ui.peel->setFlat(true);
    }

  ui.lightposition->adjustSize();
  ui.applylighting->adjustSize();
  ui.applycoloredshadow->adjustSize();
  ui.applybackplane->adjustSize();
  ui.applyshadow->adjustSize();
  ui.peel->adjustSize();


  update();
}

void
LightingWidget::setLightInfo(LightingInformation lightInfo)
{
  ui.applyemissive->setChecked(lightInfo.applyEmissive);
  ui.applylighting->setChecked(lightInfo.applyLighting);
  ui.ambient->setValue(lightInfo.highlights.ambient*10);
  ui.diffuse->setValue(lightInfo.highlights.diffuse*10);
  ui.specular->setValue(lightInfo.highlights.specular*10);
  ui.specularcoeff->setValue(ui.specularcoeff->maximum()-
      		    lightInfo.highlights.specularCoefficient);

  ui.peel->setChecked(lightInfo.peel);
  ui.peelmin->setValue(lightInfo.peelMin*100);
  ui.peelmax->setValue(lightInfo.peelMax*100);
  ui.peelmix->setValue(lightInfo.peelMix*100);
  ui.peeltype->setCurrentIndex(lightInfo.peelType);

  ui.applyshadow->setChecked(lightInfo.applyShadows);
  ui.shadowblur->setValue(lightInfo.shadowBlur);
  ui.shadowscale->setValue((lightInfo.shadowScale-0.2)*10);
  ui.shadowcontrast->setValue(lightInfo.shadowIntensity*10);
  ui.shadowfov->setValue(lightInfo.shadowFovOffset*25+5);

  ui.applycoloredshadow->setChecked(lightInfo.applyColoredShadows);
  ui.red->setValue(lightInfo.colorAttenuation.x*50);
  ui.green->setValue(lightInfo.colorAttenuation.y*50);
  ui.blue->setValue(lightInfo.colorAttenuation.z*50);

  ui.applybackplane->setChecked(lightInfo.applyBackplane);
  ui.backplaneshadowscale->setValue(lightInfo.backplaneShadowScale*5);
  ui.backplanecontrast->setValue(lightInfo.backplaneIntensity*10);

//  m_lightPosition->setVector(lightInfo.userLightVector);
//  m_lightPosition->setDistance(lightInfo.lightDistanceOffset);

  ui.gamma->setValue(qBound(0,(int)((1.5-Global::gamma())*100), 100));

  setFlat();
}

void LightingWidget::lightDirectionChanged(float len, Vec v)
{
  emit directionChanged(v);
  emit lightDistanceOffset(len);
}

void LightingWidget::on_lightposition_clicked(bool flag) { setFlat(); }


void LightingWidget::on_peel_clicked(bool flag)
{
  emit peel(flag);
  setFlat();
}
void LightingWidget::on_peelmin_sliderReleased() { peelSliderReleased(); }
void LightingWidget::on_peelmax_sliderReleased() { peelSliderReleased(); }
void LightingWidget::on_peelmix_sliderReleased() { peelSliderReleased(); }
void LightingWidget::on_peeltype_currentIndexChanged(int idx) { peelSliderReleased(); }
void LightingWidget::peelSliderReleased()
{
  int pidx = ui.peeltype->currentIndex();
  float emin = (float)ui.peelmin->value()*0.01;
  float emax = (float)ui.peelmax->value()*0.01;
  float emix = (float)ui.peelmix->value()*0.01;
  emit peelInfo(pidx, emin, emax, emix);
}

void LightingWidget::on_applylighting_clicked(bool flag)
{
  emit applyLighting(flag);
  setFlat();
}
void LightingWidget::on_ambient_valueChanged(int v) { highlightsChanged(); }
void LightingWidget::on_diffuse_valueChanged(int v) { highlightsChanged(); }
void LightingWidget::on_specular_valueChanged(int v) { highlightsChanged(); }
void LightingWidget::on_specularcoeff_valueChanged(int v) { highlightsChanged(); }
void LightingWidget::highlightsChanged()
{
  Highlights hl;

  hl.ambient = (float)ui.ambient->value()*0.1;
  hl.diffuse = (float)ui.diffuse->value()*0.1;
  hl.specular = (float)ui.specular->value()*0.1;
  hl.specularCoefficient = ui.specularcoeff->maximum()-
                               ui.specularcoeff->value();

  emit highlights(hl);
}

void
LightingWidget::on_gamma_valueChanged(int g)
{
  Global::setGamma(0.5 + fabs(1.0-ui.gamma->value()*0.01));
  emit updateGL();
}


void LightingWidget::on_shadowblur_valueChanged(int g)
{
  float v = (float)ui.shadowblur->value();
  emit shadowBlur(v);
}
void LightingWidget::on_shadowcontrast_valueChanged(int g)
{
  float v = (float)ui.shadowcontrast->value()*0.1;
  emit shadowIntensity(v);
}


void LightingWidget::on_applyemissive_clicked(bool flag)
{
  emit applyEmissive(flag);
}

void
LightingWidget::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_H &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() & Qt::MetaModifier) )
    showHelp();
}

void
LightingWidget::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/shader.help");
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
  
  propertyEditor.set("Shader Help", plist, keys);
  propertyEditor.exec();
}
