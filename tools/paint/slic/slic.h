#ifndef SLIC_H
#define SLIC_H

#include <QtGlobal>
#include <QVector>

class SLIC
{
 public :
  SLIC();
  ~SLIC();

  void clear();

  void PerformSLICO_ForGivenK(ushort*,
			      int, int,
			      int);

  int getNumLabels() { return m_numlabels; }
  int* getLabels() { return m_klabels; }
  float* getMeanL() { return m_meanl; }

  void DrawContoursAroundSegmentsTwoColors(ushort*, int*,
					   int, int);

  void MergeSuperPixels(int*, QList<int>, QList<float>,
			float, int);

 private :
  int m_width, m_height, m_depth;				     
  ushort* m_data;
  float* m_lvec;

  float* m_kseedsl;
  float* m_kseedsx;
  float* m_kseedsy;
  float* m_edgemag;

  float* m_meanl;

  int *m_klabels;
  int m_numlabels;
  int m_numseeds;

  

  void DetectEdges();

  void GetSeeds_ForGivenK(int, bool);

  void PerturbSeeds();

  void PerformSuperpixelSegmentation_VariableSandM(int, int);

  void EnforceLabelConnectivity(int*, int);
};

#endif
