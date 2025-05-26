#include "graph.h"
#include "point.h"

#include <math.h>
#include <vector>

using namespace std;

typedef Graph<double,double,double> GraphType;

#define uchar unsigned char

class MaxFlowMinCut
{
 public :
  MaxFlowMinCut();
  ~MaxFlowMinCut();

  int run(int, int,
	  int, float, bool,
	  uchar*, ushort*, int, ushort*);

 private :
  void compute_sigmas(uchar*, int, int, double*, int, float);

  void draw_edges_image_data(GraphType&,
			     uchar*, int, int,
			     int, int, int, int,
			     double*, double);

};
