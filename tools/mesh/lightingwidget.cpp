#include "lightingwidget.h"
#include "propertyeditor.h"
#include "global.h"

LightingWidget::LightingWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  m_direction = Vec(0.1,0.1,1);
  m_direction.normalize();

  m_lightdisc = new LightDisc(ui.disc);
  m_lightdisc->setDirection(false, QPointF(m_direction.x, m_direction.y));
  
  connect(m_lightdisc, SIGNAL(directionChanged(QPointF)),
	  this, SLOT(updateDirection(QPointF)));

  // hiding the light direction disc
  ui.disc->hide();
}

void
LightingWidget::updateDirection(QPointF pt)
{
  float x = pt.x();
  float y = pt.y();
  m_direction = Vec(x, y, sqrt(qAbs(1.0f-sqrt(x*x+y*y))));

  if (m_direction.norm() > 0)
    m_direction.normalize();

  emit lightDirectionChanged(m_direction);
}


void
LightingWidget::setLightInfo(LightingInformation lightInfo)
{
  ui.applylighting->setChecked(lightInfo.applyLighting);
  ui.ambient->setValue(lightInfo.highlights.ambient*10);
  ui.diffuse->setValue(lightInfo.highlights.diffuse*10);
  ui.specular->setValue(lightInfo.highlights.specular*10);
  ui.specularcoeff->setValue(lightInfo.highlights.specularCoefficient);

  ui.softness->setValue(lightInfo.softness);
  ui.edges->setValue(lightInfo.edges*10);
  ui.gamma->setValue(qBound(0,(int)((1.5-Global::gamma())*100), 100));

  ui.shadowIntensity->setValue(lightInfo.shadowIntensity*100);
  ui.valleyIntensity->setValue(lightInfo.valleyIntensity*100);
  ui.peakIntensity->setValue(lightInfo.peakIntensity*100);

  m_lightdisc->setDirection(false, QPointF(lightInfo.userLightVector.x,
					   lightInfo.userLightVector.y));
				   
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
  hl.specularCoefficient = ui.specularcoeff->value();

  emit highlights(hl);
}

void
LightingWidget::on_gamma_valueChanged(int g)
{
  Global::setGamma(0.5 + fabs(1.0-ui.gamma->value()*0.01));
  emit updateGL();
}


void LightingWidget::on_softness_valueChanged(int g)
{
  float v = (float)ui.softness->value();
  emit softness(v);
}
void LightingWidget::on_edges_valueChanged(int g)
{
  float v = (float)ui.edges->value()*0.1;
  emit edges(v);
}
void LightingWidget::on_shadowIntensity_valueChanged(int g)
{
  float v = (float)ui.shadowIntensity->value()*0.01;
  emit shadowIntensity(v);
}
void LightingWidget::on_valleyIntensity_valueChanged(int g)
{
  float v = (float)ui.valleyIntensity->value()*0.01;
  emit valleyIntensity(v);
}
void LightingWidget::on_peakIntensity_valueChanged(int g)
{
  float v = (float)ui.peakIntensity->value()*0.01;
  emit peakIntensity(v);
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
  QFile helpFile(":/shader.help"+Global::helpLanguage());
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      in.setCodec("Utf-8");
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
