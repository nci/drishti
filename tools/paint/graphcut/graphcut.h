#ifndef GRAPHCUT_H
#define GRAPHCUT_H

#include "getmemorysize.h"
#include "graph.h"
#include "point.h"

#include <QString>
#include <QtGlobal>

#include <cstddef>
#include <atomic>

typedef Graph<double,double,double> GraphType;

class MaxFlowMinCut
{
 public :
  MaxFlowMinCut();
  ~MaxFlowMinCut();

	bool run(int, int,
	   int, float, bool,
		   const uchar*, const ushort*, int, ushort*,
		   int&, QString&,
		   std::atomic_bool *cancelRequested = nullptr);

  static bool estimateMemoryBytes(int, int, quint64&, QString&);
  static bool estimateInvocationMemoryBytes(int, int, int, int,
				    quint64&, QString&);
  static bool admitMemoryBytes(quint64,
			       QString&,
			       PaintAlgorithmMemoryAdmission * = nullptr,
			       PaintMemoryStatusProvider = nullptr,
			       void * = nullptr);
  static quint64 memoryLimitBytes();

 private :
  void compute_sigmas(const uchar*, int, int, double*, int, float);

  void draw_edges_image_data(GraphType&,
			     const uchar*, int, int,
			     int, int, int, int,
			     const double*, double);

};

#endif
