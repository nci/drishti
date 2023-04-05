#include "volumeinformationwidget.h"
#include "global.h"
#include "staticfunctions.h"
#include "propertyeditor.h"
#include "xmlheaderfunctions.h"

VolumeInformationWidget::VolumeInformationWidget(QWidget *parent) :
  QWidget(parent)
{
  ui.setupUi(this);

  QRegExp rx("(\\-?\\d*\\.?\\d*\\s?){3,3}");
  QRegExpValidator *validator = new QRegExpValidator(rx,0);
  ui.m_voxelSize->setValidator(validator);

  ui.m_boxSize->setValidator(validator);

  QRegExp rx1("(\\d+)");
  QRegExpValidator *validator1 = new QRegExpValidator(rx1,0);
  ui.m_volnum->setValidator(validator1);
  ui.m_volnum_2->setValidator(validator1);
  ui.m_volnum_3->setValidator(validator1);
  ui.m_volnum_4->setValidator(validator1);

  ui.m_pvlFile->setReadOnly(true);
  ui.m_rawFile->setReadOnly(true);
  ui.m_dimensions->setReadOnly(true);
  ui.m_voxelType->setReadOnly(true);  
  ui.m_description->setDisabled(true);
  ui.m_voxelUnit->setDisabled(true);
  ui.m_voxelSize->setDisabled(true);
  ui.m_boxSize->setDisabled(true);


  ui.m_pvlFile_2->setReadOnly(true);
  ui.m_rawFile_2->setReadOnly(true);
  ui.m_dimensions_2->setReadOnly(true);
  ui.m_voxelType_2->setReadOnly(true);  
  ui.m_description_2->setDisabled(true);
  ui.m_voxelUnit_2->setDisabled(true);
  ui.m_voxelSize_2->setDisabled(true);
  ui.m_boxSize_2->setDisabled(true);

  ui.m_pvlFile_3->setReadOnly(true);
  ui.m_rawFile_3->setReadOnly(true);
  ui.m_dimensions_3->setReadOnly(true);
  ui.m_voxelType_3->setReadOnly(true);  
  ui.m_description_3->setDisabled(true);
  ui.m_voxelUnit_3->setDisabled(true);
  ui.m_voxelSize_3->setDisabled(true);
  ui.m_boxSize_3->setDisabled(true);

  ui.m_pvlFile_4->setReadOnly(true);
  ui.m_rawFile_4->setReadOnly(true);
  ui.m_dimensions_4->setReadOnly(true);
  ui.m_voxelType_4->setReadOnly(true);  
  ui.m_description_4->setDisabled(true);
  ui.m_voxelUnit_4->setDisabled(true);
  ui.m_voxelSize_4->setDisabled(true);
  ui.m_boxSize_4->setDisabled(true);


  ui.tabWidget->setTabText(0, "Volume 1");
  m_currVol = 0;

  if (ui.tabWidget->count() > 3)
    {
      m_widget4 = ui.tabWidget->widget(3);
      ui.tabWidget->removeTab(3);
    }
  if (ui.tabWidget->count() > 2)
    {
      m_widget3 = ui.tabWidget->widget(2);
      ui.tabWidget->removeTab(2);
    }
  if (ui.tabWidget->count() > 1)
    {
      m_widget2 = ui.tabWidget->widget(1);
      ui.tabWidget->removeTab(1);
    }

  m_replaceInHeader = true;
}

QList<bool>
VolumeInformationWidget::repeatType()
{
  QList<bool> rt;
  rt << ui.m_repeatType->isChecked();
  rt << ui.m_repeatType_2->isChecked();
  rt << ui.m_repeatType_3->isChecked();
  rt << ui.m_repeatType_4->isChecked();
  return rt;
}

void
VolumeInformationWidget::setRepeatType(QList<bool> rt)
{
  if (rt.count() > 0)
    ui.m_repeatType->setChecked(rt[0]);
  if (rt.count() > 1)
    ui.m_repeatType_2->setChecked(rt[1]);
  if (rt.count() > 2)
    ui.m_repeatType_3->setChecked(rt[2]);
  if (rt.count() > 3)
    ui.m_repeatType_4->setChecked(rt[3]);
}


void
VolumeInformationWidget::setVolumes(QList<int> vsizes)
{
  switch (vsizes.count())
    {
    case 1 :
      setVolumes(vsizes[0]);
      break;
      
    case 2 :
      setVolumes(vsizes[0],
		 vsizes[1]);
      break;
      
    case 3 :
      setVolumes(vsizes[0],
		 vsizes[1],
		 vsizes[2]);
      break;

    case 4 :
      setVolumes(vsizes[0],
		 vsizes[1],
		 vsizes[2],
		 vsizes[3]);
      break;
    }
}

void
VolumeInformationWidget::setVolumes(int vmax)
{
  ui.m_maxVolumes->setText(QString("  No. of volumes : %1").arg(vmax));
  ui.m_volnum->setText("0");  
  ui.m_description->setEnabled(true);
  ui.m_voxelUnit->setEnabled(true);
  ui.m_voxelSize->setEnabled(true);
  ui.m_boxSize->setEnabled(true);

  if (ui.tabWidget->count() > 3)
    {
      m_widget4 = ui.tabWidget->widget(3);
      ui.tabWidget->removeTab(3);
    }
  if (ui.tabWidget->count() > 2)
    {
      m_widget3 = ui.tabWidget->widget(2);
      ui.tabWidget->removeTab(2);
    }
  if (ui.tabWidget->count() > 1)
    {
      m_widget2 = ui.tabWidget->widget(1);
      ui.tabWidget->removeTab(1);
    }
}

void
VolumeInformationWidget::setVolumes(int vmax1, int vmax2)
{
  ui.m_maxVolumes->setText(QString("  No. of volumes : %1").arg(vmax1));
  ui.m_volnum->setText("0");  
  ui.m_description->setEnabled(true);
  ui.m_voxelUnit->setEnabled(true);
  ui.m_voxelSize->setEnabled(true);
  ui.m_boxSize->setEnabled(true);


  ui.m_maxVolumes_2->setText(QString("  No. of volumes : %1").arg(vmax2));
  ui.m_volnum_2->setText("0");  
  ui.m_description_2->setEnabled(true);
  ui.m_voxelUnit_2->setEnabled(true);
  ui.m_voxelSize_2->setEnabled(true);
  ui.m_boxSize_2->setEnabled(true);

  if (ui.tabWidget->count() > 3)
    {
      m_widget4 = ui.tabWidget->widget(3);
      ui.tabWidget->removeTab(3);
    }
  if (ui.tabWidget->count() > 2)
    {
      m_widget3 = ui.tabWidget->widget(2);
      ui.tabWidget->removeTab(2);
    }

  if (ui.tabWidget->count() == 1)
    ui.tabWidget->addTab(m_widget2, "Volume 2");
}

void
VolumeInformationWidget::setVolumes(int vmax1, int vmax2, int vmax3)
{
  ui.m_maxVolumes->setText(QString("  No. of volumes : %1").arg(vmax1));
  ui.m_volnum->setText("0");  
  ui.m_description->setEnabled(true);
  ui.m_voxelUnit->setEnabled(true);
  ui.m_voxelSize->setEnabled(true);
  ui.m_boxSize->setEnabled(true);

  ui.m_maxVolumes_2->setText(QString("  No. of volumes : %1").arg(vmax2));
  ui.m_volnum_2->setText("0");  
  ui.m_description_2->setEnabled(true);
  ui.m_voxelUnit_2->setEnabled(true);
  ui.m_voxelSize_2->setEnabled(true);
  ui.m_boxSize_2->setEnabled(true);

  ui.m_maxVolumes_3->setText(QString("  No. of volumes : %1").arg(vmax3));
  ui.m_volnum_3->setText("0");  
  ui.m_description_3->setEnabled(true);
  ui.m_voxelUnit_3->setEnabled(true);
  ui.m_voxelSize_3->setEnabled(true);
  ui.m_boxSize_3->setEnabled(true);

  if (ui.tabWidget->count() > 3)
    {
      m_widget4 = ui.tabWidget->widget(3);
      ui.tabWidget->removeTab(3);
    }
  if (ui.tabWidget->count() == 1)
    {
      ui.tabWidget->addTab(m_widget2, "Volume 2");
      ui.tabWidget->addTab(m_widget3, "Volume 3");
    }
  if (ui.tabWidget->count() == 2)
    ui.tabWidget->addTab(m_widget3, "Volume 3");
}

void
VolumeInformationWidget::setVolumes(int vmax1, int vmax2,
				    int vmax3, int vmax4)
{
  ui.m_maxVolumes->setText(QString("  No. of volumes : %1").arg(vmax1));
  ui.m_volnum->setText("0");  
  ui.m_description->setEnabled(true);
  ui.m_voxelUnit->setEnabled(true);
  ui.m_voxelSize->setEnabled(true);
  ui.m_boxSize->setEnabled(true);

  ui.m_maxVolumes_2->setText(QString("  No. of volumes : %1").arg(vmax2));
  ui.m_volnum_2->setText("0");  
  ui.m_description_2->setEnabled(true);
  ui.m_voxelUnit_2->setEnabled(true);
  ui.m_voxelSize_2->setEnabled(true);
  ui.m_boxSize_2->setEnabled(true);

  ui.m_maxVolumes_3->setText(QString("  No. of volumes : %1").arg(vmax3));
  ui.m_volnum_3->setText("0");  
  ui.m_description_3->setEnabled(true);
  ui.m_voxelUnit_3->setEnabled(true);
  ui.m_voxelSize_3->setEnabled(true);
  ui.m_boxSize_3->setEnabled(true);

  ui.m_maxVolumes_4->setText(QString("  No. of volumes : %1").arg(vmax4));
  ui.m_volnum_4->setText("0");  
  ui.m_description_4->setEnabled(true);
  ui.m_voxelUnit_4->setEnabled(true);
  ui.m_voxelSize_4->setEnabled(true);
  ui.m_boxSize_4->setEnabled(true);

  if (ui.tabWidget->count() == 1)
    {
      ui.tabWidget->addTab(m_widget2, "Volume 2");
      ui.tabWidget->addTab(m_widget3, "Volume 3");
      ui.tabWidget->addTab(m_widget4, "Volume 4");
    }
  if (ui.tabWidget->count() == 2)
    {
      ui.tabWidget->addTab(m_widget3, "Volume 3");
      ui.tabWidget->addTab(m_widget4, "Volume 4");
    }
  if (ui.tabWidget->count() == 3)
    {
      ui.tabWidget->addTab(m_widget4, "Volume 4");
    }
}

void
VolumeInformationWidget::refreshVolInfo(int vnum,
					VolumeInformation volInfo)
{
  m_replaceInHeader = false;

  m_volInfo[0] = volInfo;
  ui.m_volnum->setText(QString("%1").arg(vnum));  
  ui.m_pvlFile->setText(volInfo.pvlFile);
  ui.m_rawFile->setText(volInfo.rawFile);
  ui.m_description->setText(volInfo.description);
  ui.m_dimensions->setText(QString("%1 %2 %3").\
			   arg(volInfo.dimensions.z).\
			   arg(volInfo.dimensions.y).\
			   arg(volInfo.dimensions.x));
  ui.m_voxelType->setText(volInfo.voxelTypeString());
  ui.m_voxelUnit->setCurrentIndex(volInfo.voxelUnit);
  ui.m_voxelSize->setText(QString("%1 %2 %3").\
			  arg(volInfo.voxelSize.x).\
			  arg(volInfo.voxelSize.y).\
			  arg(volInfo.voxelSize.z));

  Vec boxSize;
  boxSize.x = volInfo.voxelSize.x*volInfo.dimensions.z;
  boxSize.y = volInfo.voxelSize.y*volInfo.dimensions.y;
  boxSize.z = volInfo.voxelSize.z*volInfo.dimensions.x;
  ui.m_boxSize->setText(QString("%1 %2 %3").\
			arg(boxSize.x).	\
			arg(boxSize.y).	\
			arg(boxSize.z));

  // ---- using voxelsize for current volume to calculate relative voxelscaling ----
  float minval = qMin(volInfo.voxelSize.x,
		      qMin(volInfo.voxelSize.y,
			   volInfo.voxelSize.z));
  Vec vscl = Vec(1,1,1);
  if (minval > 0.00000001)
    vscl = volInfo.voxelSize/minval;
  Global::setRelativeVoxelScaling(vscl);
  //----------------------------------------------------------


  ui.m_mapping->clear();
  for(int i=0; i<volInfo.mapping.count(); i++)
    {
      ui.m_mapping->addItem(QString("%1 <--> %2").\
			    arg(volInfo.mapping[i].x()).\
			    arg(volInfo.mapping[i].y()));
    }

  update();

  m_replaceInHeader = true;
}

void
VolumeInformationWidget::refreshVolInfo2(int vnum,
					 VolumeInformation volInfo)
{
  m_replaceInHeader = false;

  m_volInfo[1] = volInfo;
  ui.m_volnum_2->setText(QString("%1").arg(vnum));  
  ui.m_pvlFile_2->setText(volInfo.pvlFile);
  ui.m_rawFile_2->setText(volInfo.rawFile);
  ui.m_description_2->setText(volInfo.description);
  ui.m_dimensions_2->setText(QString("%1 %2 %3").\
			     arg(volInfo.dimensions.z).	\
			     arg(volInfo.dimensions.y).	\
			     arg(volInfo.dimensions.x));
  ui.m_voxelType_2->setText(volInfo.voxelTypeString());
  ui.m_voxelUnit_2->setCurrentIndex(volInfo.voxelUnit);
  ui.m_voxelSize_2->setText(QString("%1 %2 %3").\
			    arg(volInfo.voxelSize.x).	\
			    arg(volInfo.voxelSize.y).	\
			    arg(volInfo.voxelSize.z));

  Vec boxSize;
  boxSize.x = volInfo.voxelSize.x*volInfo.dimensions.z;
  boxSize.y = volInfo.voxelSize.y*volInfo.dimensions.y;
  boxSize.z = volInfo.voxelSize.z*volInfo.dimensions.x;
  ui.m_boxSize_2->setText(QString("%1 %2 %3").\
			  arg(boxSize.x).     \
			  arg(boxSize.y).     \
			  arg(boxSize.z));

  
  // ---- using voxelsize for current volume to calculate relative voxelscaling ----
  float minval = qMin(volInfo.voxelSize.x,
		      qMin(volInfo.voxelSize.y,
			   volInfo.voxelSize.z));
  Vec vscl = Vec(1,1,1);
  if (minval > 0.00000001)
    vscl = volInfo.voxelSize/minval;
  Global::setRelativeVoxelScaling(vscl);
  //----------------------------------------------------------

  ui.m_mapping_2->clear();
  for(int i=0; i<volInfo.mapping.count(); i++)
    {
      ui.m_mapping_2->addItem(QString("%1 <--> %2").		\
			      arg(volInfo.mapping[i].x()).	\
			      arg(volInfo.mapping[i].y()));
    }  

  m_replaceInHeader = true;
}

void
VolumeInformationWidget::refreshVolInfo3(int vnum,
					 VolumeInformation volInfo)
{
  m_replaceInHeader = false;

  m_volInfo[2] = volInfo;
  ui.m_volnum_3->setText(QString("%1").arg(vnum));  
  ui.m_pvlFile_3->setText(volInfo.pvlFile);
  ui.m_rawFile_3->setText(volInfo.rawFile);
  ui.m_description_3->setText(volInfo.description);
  ui.m_dimensions_3->setText(QString("%1 %2 %3").\
			     arg(volInfo.dimensions.z).	\
			     arg(volInfo.dimensions.y).	\
			     arg(volInfo.dimensions.x));
  ui.m_voxelType_3->setText(volInfo.voxelTypeString());
  ui.m_voxelUnit_3->setCurrentIndex(volInfo.voxelUnit);
  ui.m_voxelSize_3->setText(QString("%1 %2 %3").\
			    arg(volInfo.voxelSize.x).	\
			    arg(volInfo.voxelSize.y).	\
			    arg(volInfo.voxelSize.z));

  Vec boxSize;
  boxSize.x = volInfo.voxelSize.x*volInfo.dimensions.z;
  boxSize.y = volInfo.voxelSize.y*volInfo.dimensions.y;
  boxSize.z = volInfo.voxelSize.z*volInfo.dimensions.x;
  ui.m_boxSize_3->setText(QString("%1 %2 %3").\
			  arg(boxSize.x).     \
			  arg(boxSize.y).     \
			  arg(boxSize.z));
  
  // ---- using voxelsize for current volume to calculate relative voxelscaling ----
  float minval = qMin(volInfo.voxelSize.x,
		      qMin(volInfo.voxelSize.y,
			   volInfo.voxelSize.z));
  Vec vscl = Vec(1,1,1);
  if (minval > 0.00000001)
    vscl = volInfo.voxelSize/minval;
  Global::setRelativeVoxelScaling(vscl);
  //----------------------------------------------------------

  ui.m_mapping_3->clear();
  for(int i=0; i<volInfo.mapping.count(); i++)
    {
      ui.m_mapping_3->addItem(QString("%1 <--> %2").		\
			      arg(volInfo.mapping[i].x()).	\
			      arg(volInfo.mapping[i].y()));
    }  

  m_replaceInHeader = true;
}

void
VolumeInformationWidget::refreshVolInfo4(int vnum,
					 VolumeInformation volInfo)
{
  m_replaceInHeader = false;

  m_volInfo[3] = volInfo;
  ui.m_volnum_4->setText(QString("%1").arg(vnum));  
  ui.m_pvlFile_4->setText(volInfo.pvlFile);
  ui.m_rawFile_4->setText(volInfo.rawFile);
  ui.m_description_4->setText(volInfo.description);
  ui.m_dimensions_4->setText(QString("%1 %2 %3").\
			     arg(volInfo.dimensions.z).	\
			     arg(volInfo.dimensions.y).	\
			     arg(volInfo.dimensions.x));
  ui.m_voxelType_4->setText(volInfo.voxelTypeString());
  ui.m_voxelUnit_4->setCurrentIndex(volInfo.voxelUnit);
  ui.m_voxelSize_4->setText(QString("%1 %2 %3").\
			    arg(volInfo.voxelSize.x).	\
			    arg(volInfo.voxelSize.y).	\
			    arg(volInfo.voxelSize.z));
  
  Vec boxSize;
  boxSize.x = volInfo.voxelSize.x*volInfo.dimensions.z;
  boxSize.y = volInfo.voxelSize.y*volInfo.dimensions.y;
  boxSize.z = volInfo.voxelSize.z*volInfo.dimensions.x;
  ui.m_boxSize_4->setText(QString("%1 %2 %3").\
			  arg(boxSize.x).     \
			  arg(boxSize.y).     \
			  arg(boxSize.z));

  // ---- using voxelsize for current volume to calculate relative voxelscaling ----
  float minval = qMin(volInfo.voxelSize.x,
		      qMin(volInfo.voxelSize.y,
			   volInfo.voxelSize.z));
  Vec vscl = Vec(1,1,1);
  if (minval > 0.00000001)
    vscl = volInfo.voxelSize/minval;
  Global::setRelativeVoxelScaling(vscl);
  //----------------------------------------------------------

  ui.m_mapping_4->clear();
  for(int i=0; i<volInfo.mapping.count(); i++)
    {
      ui.m_mapping_4->addItem(QString("%1 <--> %2").		\
			      arg(volInfo.mapping[i].x()).	\
			      arg(volInfo.mapping[i].y()));
    }  

  m_replaceInHeader = true;
}

void
VolumeInformationWidget::refreshVolInfo(int tab, int vnum,
					VolumeInformation volInfo)
{
  if (tab == 0)
    refreshVolInfo(vnum, volInfo);
  else if (tab == 1)
    refreshVolInfo2(vnum, volInfo);
  else if (tab == 2)
    refreshVolInfo3(vnum, volInfo);
  else if (tab == 3)
    refreshVolInfo4(vnum, volInfo);

  update();
}

void VolumeInformationWidget::on_tabWidget_currentChanged(int vol) { m_currVol = vol; }

void VolumeInformationWidget::on_m_repeatType_clicked(bool c) { emit repeatType(0, c); }
void VolumeInformationWidget::on_m_repeatType_2_clicked(bool c) { emit repeatType(1, c); }
void VolumeInformationWidget::on_m_repeatType_3_clicked(bool c) { emit repeatType(2, c); }
void VolumeInformationWidget::on_m_repeatType_4_clicked(bool c) { emit repeatType(3, c); }

void VolumeInformationWidget::on_m_volnum_editingFinished()
{
  int vnum = ui.m_volnum->text().toInt();
  emit volumeNumber(vnum);
}
void VolumeInformationWidget::on_m_volnum_2_editingFinished()
{
  int vnum = ui.m_volnum_2->text().toInt();
  emit volumeNumber(1, vnum);
}
void VolumeInformationWidget::on_m_volnum_3_editingFinished()
{
  int vnum = ui.m_volnum_3->text().toInt();
  emit volumeNumber(2, vnum); 
}
void VolumeInformationWidget::on_m_volnum_4_editingFinished()
{
  int vnum = ui.m_volnum_4->text().toInt();
  emit volumeNumber(3, vnum);
}



void
VolumeInformationWidget::newVoxelUnit(int idx)
{
 m_volInfo[m_currVol].voxelUnit = idx;
 VolumeInformation::setVolumeInformation(m_volInfo[m_currVol], m_currVol);
 
  if (m_replaceInHeader)
    {
      QString vstr = m_volInfo[m_currVol].voxelUnitString().toLower();
      XmlHeaderFunctions::replaceInHeader(m_volInfo[m_currVol].pvlFile,
					  "voxelunit",
					  vstr);
    }
}

void VolumeInformationWidget::on_m_voxelUnit_currentIndexChanged(int idx) { newVoxelUnit(idx); }
void VolumeInformationWidget::on_m_voxelUnit_2_currentIndexChanged(int idx) { newVoxelUnit(idx); }
void VolumeInformationWidget::on_m_voxelUnit_3_currentIndexChanged(int idx) { newVoxelUnit(idx); }
void VolumeInformationWidget::on_m_voxelUnit_4_currentIndexChanged(int idx) { newVoxelUnit(idx); }




void
VolumeInformationWidget::newDescription()
{
  if (m_currVol == 0) m_volInfo[m_currVol].description = ui.m_description->text();
  if (m_currVol == 1) m_volInfo[m_currVol].description = ui.m_description_2->text();
  if (m_currVol == 2) m_volInfo[m_currVol].description = ui.m_description_3->text();
  if (m_currVol == 3) m_volInfo[m_currVol].description = ui.m_description_4->text();
  XmlHeaderFunctions::replaceInHeader(m_volInfo[m_currVol].pvlFile,
				      "description",
				      m_volInfo[m_currVol].description);

  qApp->processEvents();
}
void VolumeInformationWidget::on_m_description_editingFinished() { newDescription(); }
void VolumeInformationWidget::on_m_description_2_editingFinished() { newDescription(); }
void VolumeInformationWidget::on_m_description_3_editingFinished() { newDescription(); }
void VolumeInformationWidget::on_m_description_4_editingFinished() { newDescription(); }



void
VolumeInformationWidget::newVoxelSize()
{
  Vec vsize;
  if (m_currVol == 0) vsize = StaticFunctions::getVec(ui.m_voxelSize->text());
  if (m_currVol == 1) vsize = StaticFunctions::getVec(ui.m_voxelSize_2->text());
  if (m_currVol == 2) vsize = StaticFunctions::getVec(ui.m_voxelSize_3->text());
  if (m_currVol == 3) vsize = StaticFunctions::getVec(ui.m_voxelSize_4->text());

  if (vsize.x <= 0 || vsize.x <= 0 || vsize.x <= 0)
    {
      QMessageBox::critical(0, "Voxel Size Error",
			    QString("Voxel size <= 0 not allowed\nDefaulting to 1 1 1"),
			    QString("%1 %2 %3").arg(vsize.x).arg(vsize.y).arg(vsize.z));
      vsize.x = vsize.y = vsize.z = 1;
    }

  m_volInfo[m_currVol].voxelSize = vsize;

  QString vstr = QString("%1 %2 %3").arg(vsize.x).arg(vsize.y).arg(vsize.z);

  XmlHeaderFunctions::replaceInHeader(m_volInfo[m_currVol].pvlFile,
				      "voxelsize",
				      vstr);

  // ---- using voxelsize calculate relative voxelscaling ----
  float minval = qMin(vsize.x, qMin(vsize.y, vsize.z));
  Vec vscl = Vec(1,1,1);
  if (minval > 0.00000001)
    vscl = vsize/minval;
  Global::setRelativeVoxelScaling(vscl);

  m_volInfo[m_currVol].relativeVoxelScaling = vscl;
  VolumeInformation::setVolumeInformation(m_volInfo[m_currVol], m_currVol);
  //----------------------------------------------------------

  QString vtxt = QString("%1 %2 %3"). 
                   arg(vsize.x).arg(vsize.y).arg(vsize.z);
  if (m_currVol == 0) ui.m_voxelSize->setText(vtxt);
  if (m_currVol == 1) ui.m_voxelSize_2->setText(vtxt);
  if (m_currVol == 2) ui.m_voxelSize_3->setText(vtxt);
  if (m_currVol == 3) ui.m_voxelSize_4->setText(vtxt);

  Vec dim;
  dim = m_volInfo[m_currVol].dimensions;

  Vec boxSize;
  boxSize.x = vsize.x*dim.z;
  boxSize.y = vsize.y*dim.y;
  boxSize.z = vsize.z*dim.x;
  vtxt = QString("%1 %2 %3").arg(boxSize.x).\
                             arg(boxSize.y).\
			     arg(boxSize.z);
  if (m_currVol == 0) ui.m_boxSize->setText(vtxt);
  if (m_currVol == 1) ui.m_boxSize_2->setText(vtxt);
  if (m_currVol == 2) ui.m_boxSize_3->setText(vtxt);
  if (m_currVol == 3) ui.m_boxSize_4->setText(vtxt);

  update();
  emit updateScaling();
  emit updateGL();

  qApp->processEvents();
}
void VolumeInformationWidget::on_m_voxelSize_editingFinished() { newVoxelSize(); }
void VolumeInformationWidget::on_m_voxelSize_2_editingFinished() { newVoxelSize(); }
void VolumeInformationWidget::on_m_voxelSize_3_editingFinished() { newVoxelSize(); }
void VolumeInformationWidget::on_m_voxelSize_4_editingFinished() { newVoxelSize(); }




void
VolumeInformationWidget::newBoxSize()
{
  Vec bsize;
  if (m_currVol == 0) bsize = StaticFunctions::getVec(ui.m_boxSize->text());
  if (m_currVol == 1) bsize = StaticFunctions::getVec(ui.m_boxSize_2->text());
  if (m_currVol == 2) bsize = StaticFunctions::getVec(ui.m_boxSize_3->text());
  if (m_currVol == 3) bsize = StaticFunctions::getVec(ui.m_boxSize_4->text());

  Vec vsize;
  vsize.x = bsize.x/m_volInfo[m_currVol].dimensions.z;
  vsize.y = bsize.y/m_volInfo[m_currVol].dimensions.y;
  vsize.z = bsize.z/m_volInfo[m_currVol].dimensions.x;
  m_volInfo[m_currVol].voxelSize = vsize;

  QString vstr = QString("%1 %2 %3").arg(vsize.x).arg(vsize.y).arg(vsize.z);
  XmlHeaderFunctions::replaceInHeader(m_volInfo[m_currVol].pvlFile,
				      "voxelsize",
				      vstr);

  // ---- using voxelsize calculate relative voxelscaling ----
  float minval = qMin(vsize.x, qMin(vsize.y, vsize.z));
  Vec vscl = Vec(1,1,1);
  if (minval > 0.00000001)
    vscl = m_volInfo[m_currVol].voxelSize/minval;
  Global::setRelativeVoxelScaling(vscl);

  m_volInfo[m_currVol].relativeVoxelScaling = vscl;

  VolumeInformation::setVolumeInformation(m_volInfo[m_currVol], m_currVol);
  //----------------------------------------------------------

  QString vtxt = QString("%1 %2 %3"). 
                   arg(vsize.x).arg(vsize.y).arg(vsize.z);
  if (m_currVol == 0) ui.m_voxelSize->setText(vtxt);
  if (m_currVol == 1) ui.m_voxelSize_2->setText(vtxt);
  if (m_currVol == 2) ui.m_voxelSize_3->setText(vtxt);
  if (m_currVol == 3) ui.m_voxelSize_4->setText(vtxt);

  Vec dim = m_volInfo[m_currVol].dimensions;

  Vec boxSize;
  boxSize.x = vsize.x*dim.z;
  boxSize.y = vsize.y*dim.y;
  boxSize.z = vsize.z*dim.x;
  vtxt = QString("%1 %2 %3").arg(boxSize.x).\
                             arg(boxSize.y).\
			     arg(boxSize.z);
  if (m_currVol == 0) ui.m_boxSize->setText(vtxt);
  if (m_currVol == 1) ui.m_boxSize_2->setText(vtxt);
  if (m_currVol == 2) ui.m_boxSize_3->setText(vtxt);
  if (m_currVol == 3) ui.m_boxSize_4->setText(vtxt);

  update();
  emit updateScaling();
  emit updateGL();
  qApp->processEvents();
}
void VolumeInformationWidget::on_m_boxSize_editingFinished() { newBoxSize(); }
void VolumeInformationWidget::on_m_boxSize_2_editingFinished() { newBoxSize(); }
void VolumeInformationWidget::on_m_boxSize_3_editingFinished() { newBoxSize(); }
void VolumeInformationWidget::on_m_boxSize_4_editingFinished() { newBoxSize(); }

void
VolumeInformationWidget::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_H &&
      (event->modifiers() & Qt::ControlModifier ||
       event->modifiers() == Qt::MetaModifier) )
    showHelp();
}

void
VolumeInformationWidget::showHelp()
{
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/volinfo.help");
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
  
  propertyEditor.set("Volume Information Help", plist, keys);
  propertyEditor.exec();
}
