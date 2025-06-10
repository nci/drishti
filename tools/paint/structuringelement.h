#ifndef STRUCTURINGELEMENT_H
#define STRUCTURINGELEMENT_H

#include <QString>
#include <QMap>

class StructuringElement
{
 public :
  static void init();
  static bool* getElement(QString);

 private :
  static QMap<QString, bool*> m_se;  
};

#endif
