#include "global.h"
#include "transferfunctioncontainer.h"

int TransferFunctionContainer::count() { return m_splineTF.count(); }
int TransferFunctionContainer::maxSets() { return m_maxSets; }

void
TransferFunctionContainer::switch1D()
{
  for(int si=0; si<m_splineTF.count(); si++)
    m_splineTF[si]->switch1D();
}

TransferFunctionContainer::TransferFunctionContainer(QObject * parent) : QObject(parent)
{
  m_splineTF.clear();
  m_maxSets = Global::lutSize();
}

TransferFunctionContainer::~TransferFunctionContainer()
{
  for(int i=0; i<m_splineTF.count(); i++)
    delete m_splineTF[i];
  m_splineTF.clear();
  m_maxSets = 0;
}

void
TransferFunctionContainer::clearContainer()
{
  for(int i=0; i<m_splineTF.count(); i++)
    delete m_splineTF[i];
  m_splineTF.clear();
  m_maxSets = Global::lutSize();
}

SplineTransferFunction*
TransferFunctionContainer::transferFunctionPtr(int row)
{
  if (row < m_splineTF.count())
    return m_splineTF[row];
  else
    return NULL;
}

bool
TransferFunctionContainer::getCheckState(int row, int col)
{
  if (row < m_splineTF.count() && col < m_maxSets)
    return m_splineTF[row]->on(col);
  else
    return false;
}

void
TransferFunctionContainer::setCheckState(int row, int col, bool flag)
{
  if (row < m_splineTF.count() && col < m_maxSets)
    m_splineTF[row]->setOn(col, flag);
}

void
TransferFunctionContainer::fromDomElement(QDomElement de)
{
  SplineTransferFunction *splineTF = new SplineTransferFunction();
  splineTF->fromDomElement(de);
  m_splineTF.push_back(splineTF);
}

void
TransferFunctionContainer::fromSplineInformation(SplineInformation si)
{
  SplineTransferFunction *splineTF = new SplineTransferFunction();
  splineTF->setSpline(si);
  m_splineTF.push_back(splineTF);
}
void
TransferFunctionContainer::addSplineTF()
{
  SplineTransferFunction *splineTF = new SplineTransferFunction();
  splineTF->setOn(0, true);
  m_splineTF.push_back(splineTF);
}

void
TransferFunctionContainer::removeSplineTF(int row)
{
  if (row>=0 && row<m_splineTF.count())
    {
      delete m_splineTF[row];
      m_splineTF.removeAt(row);
    }
}

QImage
TransferFunctionContainer::colorMapImage(int row)
{
  if (row < m_splineTF.count())
    return m_splineTF[row]->colorMapImage();
  else
    return QImage();
}


QImage
TransferFunctionContainer::composite(int col)
{
  if (col >= m_maxSets)
    return QImage();

  float *acclut = new float[256*256*4];
  memset(acclut, 0, 256*256*4*sizeof(float));

  float *maxa = new float[256*256];
  for(int l=0; l<256*256; l++)
    maxa[l] = -1.0f;

  uchar* blut = new uchar[256*256*4];
  memset(blut, 0, 256*256*4);

  bool atleastOne = false;
  for(int si=0; si<m_splineTF.count(); si++)
    {
      if (m_splineTF[si]->on(col))
        {
          atleastOne = true;
          memcpy(blut, m_splineTF[si]->colorMapImage().bits(), 256*256*4);
          for(int l=0; l<256*256; l++)
            {
              float op = ((float)blut[4*l + 3])/255.0f;
              acclut[4*l + 0] += op*((float)blut[4*l + 0])/255.0f;
              acclut[4*l + 1] += op*((float)blut[4*l + 1])/255.0f;
              acclut[4*l + 2] += op*((float)blut[4*l + 2])/255.0f;
              acclut[4*l + 3] += op;
              maxa[l] = qMax(maxa[l], op);
            }
        }
    }
  if (atleastOne)
    {
      for(int l=0; l<256*256; l++)
        {
          float op = acclut[4*l + 3];
          if (op > 0)
            {
              acclut[4*l + 0] /= op;
              acclut[4*l + 1] /= op;
              acclut[4*l + 2] /= op;
            }
        }

      for(int l=0; l<256*256; l++)
        {
          float r = acclut[4*l + 0];
          float g = acclut[4*l + 1];
          float b = acclut[4*l + 2];
          
          uchar ur, ug, ub, ua;
          ur = 255*r;
          ug = 255*g;
          ub = 255*b;
          ua = 255*maxa[l];
          
          blut[4*l+0] = ur;
          blut[4*l+1] = ug;
          blut[4*l+2] = ub;
          blut[4*l+3] = ua;
        }
    }

  QImage colorMapImage = QImage(256, 256, QImage::Format_ARGB32);
  memcpy(colorMapImage.bits(), blut, 256*256*4);

  delete [] acclut;
  delete [] maxa;
  delete [] blut;

  return colorMapImage;
}

void
TransferFunctionContainer::transferFunctionUpdated(int row)
{  
  m_splineTF[row]->append();
}

void
TransferFunctionContainer::applyUndo(int row, bool flag)
{  
  m_splineTF[row]->applyUndo(flag);
}

void
TransferFunctionContainer::duplicate(int row)
{
  if (row < m_splineTF.count())
    fromSplineInformation(m_splineTF[row]->getSpline());      
}
