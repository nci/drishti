#include <QtGui>
#include "graphcut.h"

#include <iostream>
#include <fstream>

#define INF 100000000
#define diff(img, x1, y1, x2, y2) pow((double)(img[y1*w+x1] - img[y2*w+x2]),2)

MaxFlowMinCut::MaxFlowMinCut() {}
MaxFlowMinCut::~MaxFlowMinCut() {}
  
void
MaxFlowMinCut::compute_sigmas(uchar *image, int w, int h,
			      double *sigmas,
			      int boxSize, float lambda)
{
  //calculate average gradient magnitude in a boxSized region

  ushort *imgtmp = new ushort[w*h];
  memset(imgtmp, 0, 2*w*h);
  for (int i=1; i<w-1; ++i)
    for (int j=1; j<h-1; ++j)
      {
//	// forward difference
//	int tmp = image[j*w+i];
//	int a = image[j*w+(i+1)];
//	int b = image[(j+1)*w+i];
//	imgtmp[j*w+i] = abs(tmp-a)+abs(tmp-b);

	// central difference
	int a = image[j*w+(i+1)];
	int b = image[j*w+(i-1)];
	int c = image[(j+1)*w+i];
	int d = image[(j-1)*w+i];
	imgtmp[j*w+i] = abs(a-b)+abs(c-d);

//	// Sobel
//	int a = image[(j+1)*w+(i+1)]-image[(j+1)*w+(i-1)];
//	int b = image[j*w+(i+1)] - image[j*w+(i-1)];
//	int c = image[(j-1)*w+(i+1)] - image[(j-1)*w+(i-1)];
//
//	int d = image[(j+1)*w+(i-1)] - image[(j-1)*w+(i-1)];
//	int e = image[(j+1)*w+i] - image[(j-1)*w+i];
//	int f = image[(j+1)*w+(i+1)] - image[(j-1)*w+(i+1)];
//
//	imgtmp[j*w+i] = abs(a)+2*abs(b)+abs(c) + abs(d)+2*abs(e)+abs(f);
      }

  for (int i=0; i<w; ++i)
    for (int j=0; j<h; ++j)
      {
	int xmin = max(0,i-boxSize);
	int xmax = min(w-1, i+boxSize);

	int ymin = max(0,j-boxSize);
	int ymax = min(h-1, j+boxSize);
	
	double sum=0;
	for (int x=xmin; x<=xmax; ++x)
	  for (int y=ymin; y<=ymax; ++y)
	    sum += imgtmp[y*w+x];
	
	sigmas[j*w+i] = sum / (2*(xmax-xmin+1)*(ymax-ymin+1));
	//sigmas[j*w+i] = sum / (8*(xmax-xmin+1)*(ymax-ymin+1));

	// lambda increases/decreases the average gradient magnitude
	sigmas[j*w+i] *= lambda;
      }

  delete [] imgtmp;
}

void
MaxFlowMinCut::draw_edges_image_data(GraphType &G,
				     uchar *image, int w, int h,
				     int x1, int y1, int x2, int y2,
				     double *sigmas,
				     double dist)
{
  if (x2 >= 0 && x2 < w &&
      y2 >= 0 && y2 < h)
    {
      double sigma = sigmas[y1*w+x1];

      double ene = 1.0;
      if (fabs(sigma) > 0.0)
	ene = exp(-diff(image, x1, y1, x2, y2)/(2*sigma*sigma));
      double weight = ene/dist;

//      double ene = 1.0;
//      if (fabs(sigma) > 0.0)
//	ene = exp(-diff(image, x1, y1, x2, y2)/(2*sigma*sigma));
//
//      double pi4 = 3.1415926535/4;
//      double num = dist*dist*pi4*ene;
//      
//      double imgx, imgy;
//      imgx = imgy = 0;
//      if (x1 > 0 && x1 < w-1)
//	imgx = (image[y1*w+(x1+1)]-image[y1*w+(x1-1)]);
//      if (y1 > 0 && y1 < h-1)
//	imgy = (image[(y1+1)*w+x1]-image[(y1-1)*w+x1]);
//      double imgl = sqrt(imgx*imgx+imgy*imgy);
//      if (fabs(imgl) > 0.0)
//	{
//	  imgx /= imgl;
//	  imgy /= imgl;
//	}
//      double dd = (y2-y1)*imgy + (x2-x1)*imgx;
//      double den = 2*qPow((ene*dist*dist+(1.0-ene)*dd*dd), 3.0/2.0);
//      double weight = num/den;
      
      
      G.add_edge(y1*w+x1, y2*w+x2, weight, weight);
    }
}

int
MaxFlowMinCut::run(int w, int h,
		   int boxSize, float lambda,
		   bool tagSimilar,
		   uchar *image,
		   uchar *mask,
		   int tag, uchar *tags)
{
  //cout << "boxSize=" << boxSize << endl;
  //QMessageBox::information(0, "", "run");

  double *sigmas = new double[w*h];
  compute_sigmas(image, w, h, sigmas, boxSize, lambda);
        
  GraphType G(w*h, 6*w*h);
  G.reset();
  G.add_node(w*h);
  
  //Data term
  for (int i=0; i<w; i++)
    for (int j=0; j<h; j++)
      {
	G.add_tweights(j*w+i, 0, 0);
	
	draw_edges_image_data(G,
			      image, w, h,
			      i, j, i, j+1,
			      sigmas, 1);	//Down

	draw_edges_image_data(G,
			      image, w, h,
			      i, j, i+1, j,
			      sigmas, 1);	//Right

	draw_edges_image_data(G,
			      image, w, h,
			      i, j, i+1, j+1,
			      sigmas, sqrt(2.)); //Down right

	draw_edges_image_data(G,
			      image, w, h,
			      i, j, i+1, j-1,
			      sigmas, sqrt(2.)); //Top right
      }
  delete [] sigmas;
  //---------------------------
  
  //Seed object and background points
  for (int i=0; i<w; ++i)
    for (int j=0; j<h; ++j)
      {
	if (mask[j*w+i] == 255) // background
	  G.add_tweights(j*w+i, INF, 0);
	else if (mask[j*w+i] == tag) // object
	  G.add_tweights(j*w+i, 0, INF);
      }
  //---------------------------

  //auto-background points
  for (int i=0;i<w;i++)
    for (int j=0;j<h;j++)
      {
	if (image[j*w+i] == 0)
	  G.add_tweights(j*w+i, INF, 0);
      }
  //---------------------------

  if (tagSimilar)
    {
      // calculate object and background histograms
      float *obj = new float[256];
      float *bg = new float[256];
      memset(obj, 0, sizeof(float)*256);
      memset(bg, 0, sizeof(float)*256);
      for (int i=0;i<w*h;i++)
	{
	  uchar v = image[i];
	  if (mask[i] == 255) bg[v]++;
	  if (mask[i] == tag) obj[v]++;
	}
      //---------------------------
      
      // normalize object histogram
      float totobj = 0;
      for(int i=0; i<256; i++)
	totobj += obj[i];
      if (totobj > 0)
	{
	  for(int i=0; i<256; i++)
	    obj[i] /= totobj;
	}
      //---------------------------
      
      // normalize background histogram
      float totbg = 0;
      for(int i=0; i<256; i++)
	totbg += bg[i];
      if (totbg > 0)
	{
	  for(int i=0; i<256; i++)
	    bg[i] /= totbg;
	}
      //---------------------------
      
      // additional t-links 
      for (int i=0;i<w;i++)
	for (int j=0;j<h;j++)
	  {
	    if (image[j*w+i] != 0 &&
		mask[j*w+i] != 255 &&
		mask[j*w+i] != tag)
	      {
		uchar v = image[j*w+i];
		float objP = obj[v]; // object probability
		float bgP = bg[v]; // background probability
		if (objP > 0)
		  objP = -log(1.0-objP);
		else
		  objP = INF;
		if (bgP > 0)
		  bgP = -log(1.0-bgP);
		else
		  bgP = INF;
		G.add_tweights(j*w+i, objP, bgP);
	      }
	  }

      delete [] obj;
      delete [] bg;
    }


  double f = G.maxflow();
  //cout << "flow " << f << endl;

  int nt=0;
  for (int i=0;i<w;i++)
    for (int j=0;j<h;j++)
      {
	if (G.what_segment(j*w+i)==GraphType::SINK)
	  {
	    tags[j*w+i] = tag;
	    nt ++;
	  }
      }

  //QMessageBox::information(0, "", "done");
  //cout << nt << " points tagged" << endl << endl;
    
  return nt;
}
