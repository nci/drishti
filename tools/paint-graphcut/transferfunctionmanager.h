#ifndef TRANSFERFUNCTIONMANAGER_H
#define TRANSFERFUNCTIONMANAGER_H

#include <QtGui>

#include "transferfunctioncontainer.h"

class TransferFunctionManager : public QFrame
{
  Q_OBJECT

 public :
  TransferFunctionManager(QWidget *parent=0);

  void registerContainer(TransferFunctionContainer*);
  void refreshManager();
  void load(const char *);
  void append(const char *);
  void load(QList<SplineInformation>);
  void save(const char *);

 public slots :
  void clearManager();
  void addNewTransferFunction();

 signals :
  void changeTransferFunctionDisplay(int, QList<bool>);
  void checkStateChanged(int, int, bool);

 protected slots :
  void headerClicked(int);
  void cellClicked(int, int);
  void cellChanged(int, int);
  void refreshTransferFunction();
  void removeTransferFunction();  

 private :
  TransferFunctionContainer *m_tfContainer;
  QGroupBox *m_buttonGroup;
  QTableWidget *m_tableWidget;

  void modifyTableWidget();
};

#endif
