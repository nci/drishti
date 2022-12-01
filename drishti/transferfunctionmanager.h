#ifndef TRANSFERFUNCTIONMANAGER_H
#define TRANSFERFUNCTIONMANAGER_H

#include <QWidget>
#include <QGroupBox>
#include <QTableWidget>
#include <QCheckBox>

#include <QDoubleSpinBox>

#include "transferfunctioncontainer.h"
#include "imglistdialog.h"

class TransferFunctionManager : public QFrame
{
  Q_OBJECT

 public :
  TransferFunctionManager(QWidget *parent=0);

  void registerContainer(TransferFunctionContainer*);
  void load(const char *);
  void append(const char *);
  void load(QList<SplineInformation>);
  void save(const char *);
  void loadDefaultTF();

 public slots :
  void clearManager();
  void updateMorph(bool);
  void updateReplace(bool);
  void applyUndo(bool);
  void transferFunctionUpdated();
  void keyPressEvent(QKeyEvent*);
  void changeMaterial();
  void matMixChanged(double);
  
 signals :
  void changeTransferFunctionDisplay(int, QList<bool>);
  void checkStateChanged(int, int, bool);
  void updateGL();
					
 protected slots :
  void headerClicked(int);
  void cellClicked(int, int);
  void cellChanged(int, int);
  void addNewTransferFunction();
  void refreshTransferFunction();
  void removeTransferFunction();  
  void replaceChanged(int);
  void morphChanged(int);
  bool eventFilter(QObject*, QEvent*);

 private :
  TransferFunctionContainer *m_tfContainer;
  QGroupBox *m_buttonGroup;
  QTableWidget *m_tableWidget;
  QCheckBox *m_replaceTF;
  QCheckBox *m_morphTF;

  ImgListDialog *m_matcapDialog;
  QPushButton *m_material;
  QDoubleSpinBox *m_matMix;
  
  void refreshManager(int tfno=0);
  void modifyTableWidget();
  void showHelp();
};

#endif
