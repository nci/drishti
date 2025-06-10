#include "structuringelement.h"

QMap<QString, bool*> StructuringElement::m_se;

void
StructuringElement::init()
{
  if (m_se.count() != 0)
    return;
  
  bool *e;

  e = new bool[27];

  memset(e, 0, 27);  
  e[4] = true; 
  e[10] = true;
  e[12] = true;
  e[13] = true; 
  e[14] = true;
  e[16] = true;
  e[22] = true;    
  m_se["cross3"] = e;

  
  for(int i=0; i<27; i++)
    e[i] = true;
  m_se["box3"] = e;
}  

bool*
StructuringElement::getElement(QString name)
{
  return m_se[name];
}
