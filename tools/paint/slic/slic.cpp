#include "slic.h"

#include <QMessageBox>
#include <QMap>
#include <QQueue>
#include <QBitArray>
#include <QtMath>

SLIC::SLIC()
{
  m_klabels = 0;
  m_lvec = 0;
  m_width = m_height = 0;
  m_numlabels = 0;
  m_numseeds = 0;

  m_kseedsl = 0;
  m_kseedsx = 0;
  m_kseedsy = 0;
  m_edgemag = 0;

  m_meanl = 0;
}

SLIC::~SLIC()
{
  clear();
}

void
SLIC::clear()
{
  if (m_klabels) delete [] m_klabels;
  if (m_lvec) delete [] m_lvec;
  if (m_kseedsl) delete [] m_kseedsl;
  if (m_kseedsx) delete [] m_kseedsx;
  if (m_kseedsy) delete [] m_kseedsy;
  if (m_edgemag) delete [] m_edgemag;
  if (m_meanl) delete [] m_meanl;

  m_klabels = 0;
  m_lvec = 0;
  m_kseedsl = 0;
  m_kseedsx = 0;
  m_kseedsy = 0;
  m_edgemag = 0;
  m_meanl = 0;

  m_numseeds = 0;
  m_numlabels = 0;
  m_width = m_height = 0;
}

// Zero parameter SLIC algorithm for a given number K of superpixels.
void
SLIC::PerformSLICO_ForGivenK(ushort *ubuff,
			     int wd, int ht,
			     int K) //required number of superpixels
{
  bool perturbseeds = true;

  m_width = wd;
  m_height = ht;

  int sz = m_width*m_height;

  m_edgemag = new float[sz];
  memset(m_edgemag, 0, sz*sizeof(float));

  if (m_klabels) delete [] m_klabels;
  m_klabels = new int[sz];
  for(int s=0; s<sz; s++)
    m_klabels[s] = -1;

  if (m_lvec) delete [] m_lvec;
  m_lvec = new float[sz];
  for(int s=0; s<sz; s++)
    m_lvec[s] = ubuff[s];

  if (perturbseeds)
    DetectEdges();

  GetSeeds_ForGivenK(K, perturbseeds);
  delete [] m_edgemag;

  int STEP = sqrt(float(sz)/float(K)) + 2.0;//adding a small value in the even the STEP size is too small.
  PerformSuperpixelSegmentation_VariableSandM(STEP,10);

  int* nlabels = new int[sz];
  EnforceLabelConnectivity(nlabels, K);

  for(int i = 0; i < sz; i++ )
    m_klabels[i] = nlabels[i];

  delete [] nlabels;

  //---------------------
  // store meanl at every pixel
  if (m_meanl) delete [] m_meanl;
  m_meanl = new float[sz];
  for( int i = 0; i < sz; i++ )
    m_meanl[i] = m_kseedsl[m_klabels[i]];
  //---------------------

}


void
SLIC::DetectEdges()
{
  for( int j = 1; j < m_height-1; j++ )
    {
      int k = 0;
      int i = j*m_width+k;

      float dx = (m_lvec[i]-m_lvec[i+1])*
	         (m_lvec[i]-m_lvec[i+1]);
      
      float dy = (m_lvec[i-m_width]-m_lvec[i+m_width])*
	         (m_lvec[i-m_width]-m_lvec[i+m_width]);
      
      m_edgemag[i] = (dx + dy);
    }

  for( int j = 1; j < m_height-1; j++ )
    {
      int k = m_width-1;
      int i = j*m_width+k;

      float dx = (m_lvec[i-1]-m_lvec[i])*
	         (m_lvec[i-1]-m_lvec[i]);
      
      float dy = (m_lvec[i-m_width]-m_lvec[i+m_width])*
	         (m_lvec[i-m_width]-m_lvec[i+m_width]);
      
      m_edgemag[i] = (dx + dy);
    }

  for( int k = 1; k < m_width-1; k++ )
    {
      int j = 0;
      int i = j*m_width+k;
      float dx = (m_lvec[i-1]-m_lvec[i+1])*
	         (m_lvec[i-1]-m_lvec[i+1]);
      
      float dy = (m_lvec[i]-m_lvec[i+m_width])*
	         (m_lvec[i]-m_lvec[i+m_width]);
      
      m_edgemag[i] = (dx + dy);
    }
  for( int k = 1; k < m_width-1; k++ )
    {
      int j = m_height-1;
      int i = j*m_width+k;
      float dx = (m_lvec[i-1]-m_lvec[i+1])*
	         (m_lvec[i-1]-m_lvec[i+1]);
      
      float dy = (m_lvec[i-m_width]-m_lvec[i])*
	         (m_lvec[i-m_width]-m_lvec[i]);
      
      m_edgemag[i] = (dx + dy);
    }


  for( int j = 1; j < m_height-1; j++ )
    for( int k = 1; k < m_width-1; k++ )
      {
	int i = j*m_width+k;
	
	float dx = (m_lvec[i-1]-m_lvec[i+1])*
	           (m_lvec[i-1]-m_lvec[i+1]);
	
	float dy = (m_lvec[i-m_width]-m_lvec[i+m_width])*
	           (m_lvec[i-m_width]-m_lvec[i+m_width]);
	  
	m_edgemag[i] = (dx + dy);
      }
}

void
SLIC::GetSeeds_ForGivenK(int K, bool perturbseeds)			 
{
  int sz = m_width*m_height;
  float step = sqrt(float(sz)/float(K));
  int xoff = step/2;
  int yoff = step/2;
  
  int n=0;
  int r=0;

  QVector<float> ksl;
  QVector<float> ksx;
  QVector<float> ksy;

  for( int y = 0; y < m_height; y++ )
    {
      int Y = y*step + yoff;
      if( Y > m_height-1 ) break;
      
      for( int x = 0; x < m_width; x++ )
	{
	  //int X = x*step + xoff;//square grid
	  int X = x*step + (xoff<<(r&0x1));//hex grid
	  if(X > m_width-1) break;
	  
	  int i = Y*m_width + X;
	  
	  ksl << m_lvec[i];
	  ksx << X;
	  ksy << Y;

	  n++;
	}
      r++;
    }

  m_numseeds = ksl.count();
  
  if (m_kseedsl) delete [] m_kseedsl;
  if (m_kseedsx) delete [] m_kseedsx;
  if (m_kseedsy) delete [] m_kseedsy;

  m_kseedsl = new float[m_numseeds];
  m_kseedsx = new float[m_numseeds];
  m_kseedsy = new float[m_numseeds];

  for(int i=0; i<m_numseeds; i++) m_kseedsl[i] = ksl[i];
  for(int i=0; i<m_numseeds; i++) m_kseedsx[i] = ksx[i];
  for(int i=0; i<m_numseeds; i++) m_kseedsy[i] = ksy[i];
  
  ksl.clear();
  ksx.clear();
  ksy.clear();

  if(perturbseeds)
    PerturbSeeds();
}

void
SLIC::PerturbSeeds()
				
{
  const int dx8[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
  const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1};
	
  for( int n = 0; n < m_numseeds; n++ )
    {
      int ox = m_kseedsx[n];//original x
      int oy = m_kseedsy[n];//original y
      int oind = oy*m_width + ox;

      int storeind = oind;
      for( int i = 0; i < 8; i++ )
	{
	  int nx = ox+dx8[i];//new x
	  int ny = oy+dy8[i];//new y

	  if( nx >= 0 && nx < m_width && ny >= 0 && ny < m_height)
	    {
	      int nind = ny*m_width + nx;
	      if( m_edgemag[nind] < m_edgemag[storeind])
		storeind = nind;
	    }
	}
      if(storeind != oind)
	{
	  m_kseedsx[n] = storeind%m_width;
	  m_kseedsy[n] = storeind/m_width;
	  m_kseedsl[n] = m_lvec[storeind];
	}
    }
}

//===========================================================================
///	PerformSuperpixelSegmentation_VariableSandM
///
///	Magic SLIC - no parameters
///
///	Performs k mean segmentation. It is fast because it looks locally, not
/// over the entire image.
/// This function picks the maximum value of color distance as compact factor
/// M and maximum pixel distance as grid step size S from each cluster (13 April 2011).
/// So no need to input a constant value of M and S. There are two clear
/// advantages:
///
/// [1] The algorithm now better handles both textured and non-textured regions
/// [2] There is not need to set any parameters!!!
///
/// SLICO (or SLIC Zero) dynamically varies only the compactness factor S,
/// not the step size S.
//===========================================================================
void
SLIC::PerformSuperpixelSegmentation_VariableSandM(int STEP,
						  int NUMITR)
{
  int sz = m_width*m_height;

  int numk = m_numseeds;

  int numitr = 0;

  //----------------
  int offset = STEP;
  if(STEP < 10) offset = STEP*1.5;
  //----------------

  QVector<float> sigmal(numk, 0);
  QVector<float> sigmax(numk, 0);
  QVector<float> sigmay(numk, 0);
  QVector<int> clustersize(numk, 0);
  QVector<float> inv(numk, 0);//to store 1/clustersize[k] values
  QVector<float> distxy(sz, FLT_MAX);
  QVector<float> distlab(sz, FLT_MAX);
  QVector<float> distvec(sz, FLT_MAX);
  QVector<float> maxlab(numk, 10*10);//THIS IS THE VARIABLE VALUE OF M, just start with 10
  QVector<float> maxxy(numk, STEP*STEP);//THIS IS THE VARIABLE VALUE OF M, just start with 10
	
  float invxywt = 1.0/(STEP*STEP);//NOTE: this is different from how usual SLIC/LKM works

  while( numitr < NUMITR )
    {
      numitr++;
      
      distvec.fill(FLT_MAX);

      for( int n = 0; n < numk; n++ )
	{
	  int y1 = qMax(0, (int)(m_kseedsy[n]-offset));
	  int y2 = qMin(m_height, (int)(m_kseedsy[n]+offset));
	  int x1 = qMax(0, (int)(m_kseedsx[n]-offset));
	  int x2 = qMin(m_width, (int)(m_kseedsx[n]+offset));

	  for( int y = y1; y < y2; y++ )
	    {
	      for( int x = x1; x < x2; x++ )
		{
		  int i = y*m_width + x;
		  //_ASSERT( y < m_height && x < m_width && y >= 0 && x >= 0 );
		  
		  float l = m_lvec[i];
		  
//		  distlab[i] = qSqrt((l - m_kseedsl[n])*(l - m_kseedsl[n]));

//		  distxy[i] = qSqrt((x - m_kseedsx[n])*(x - m_kseedsx[n]) +
//				    (y - m_kseedsy[n])*(y - m_kseedsy[n]));
		  

		  distlab[i] = ((l - m_kseedsl[n])*(l - m_kseedsl[n]));

		  distxy[i] = ((x - m_kseedsx[n])*(x - m_kseedsx[n]) +
			       (y - m_kseedsy[n])*(y - m_kseedsy[n]));

		  //------------------------------------------------------------------------
		  float dist = distlab[i]/maxlab[n] + distxy[i]*invxywt; //only varying m, prettier superpixels
		  //float dist = distlab[i]/maxlab[n] + distxy[i]/maxxy[n];//varying both m and S
		  //------------------------------------------------------------------------
		  
		  if(dist < distvec[i])
		    {
		      distvec[i] = dist;
		      m_klabels[i] = n;
		    }
		}
	    }
	}


      //-----------------------------------------------------------------
      // Assign the max color distance for a cluster
      //-----------------------------------------------------------------
      if(0 == numitr)
	{
	  maxlab.fill(1, numk);
	  maxxy.fill(1, numk);
	}

      for( int i = 0; i < sz; i++ )
	{
	  //if (m_klabels[i] >= 0)
	    {
	      if (maxlab[m_klabels[i]] < distlab[i])
		maxlab[m_klabels[i]] = distlab[i];
	      
	      if (maxxy[m_klabels[i]] < distxy[i])
		maxxy[m_klabels[i]] = distxy[i];
	    }
	}
	
	//-----------------------------------------------------------------
	// Recalculate the centroid and store in the seed values
	//-----------------------------------------------------------------
	sigmal.fill(0, numk);
	sigmax.fill(0, numk);
	sigmay.fill(0, numk);
	clustersize.fill(0, numk);

	for( int j = 0; j < sz; j++ )
	  {
	    //if (m_klabels[j] >= 0)
	      {
		sigmal[m_klabels[j]] += m_lvec[j];
		sigmax[m_klabels[j]] += (j%m_width);
		sigmay[m_klabels[j]] += (j/m_width);
		
		clustersize[m_klabels[j]]++;
	      }
	  }

	for( int k = 0; k < numk; k++ )
	  {
	    //_ASSERT(clustersize[k] > 0);
	    if( clustersize[k] <= 0 )
	      clustersize[k] = 1;
	    inv[k] = 1.0/float(clustersize[k]);//computing inverse now to multiply, than divide later
	  }
		
	for( int k = 0; k < numk; k++ )
	  {
	    m_kseedsl[k] = sigmal[k]*inv[k];
	    m_kseedsx[k] = sigmax[k]*inv[k];
	    m_kseedsy[k] = sigmay[k]*inv[k];
	  }
    }
}

//===========================================================================
///	EnforceLabelConnectivity
///
///		1. finding an adjacent label for each new component at the start
///		2. if a certain component is too small, assigning the previously found
///		    adjacent label to this component, and not incrementing the label.
//===========================================================================
void
SLIC::EnforceLabelConnectivity(int* nlabels,//new labels
			       int K) //the number of superpixels desired by the user
{
  //	const int dx8[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
  //	const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1};
  
  const int dx4[4] = {-1,  0,  1,  0};
  const int dy4[4] = { 0, -1,  0,  1};
  
  const int sz = m_width*m_height;
  const int SUPSZ = sz/K;

  for( int i = 0; i < sz; i++ )
    nlabels[i] = -1;

  int label = 0;

  int* xvec = new int[sz];
  int* yvec = new int[sz];

  int oindex =0;
  int adjlabel =0;//adjacent label
  for(int j=0; j<m_height; j++)
    {
      for(int k=0; k<m_width; k++)
	{
	  if(nlabels[oindex] < 0)
	    {
	      nlabels[oindex] = label;
	      //--------------------
	      // Start a new segment
	      //--------------------
	      xvec[0] = k;
	      yvec[0] = j;

	      //-------------------------------------------------------
	      // Quickly find an adjacent label for use later if needed
	      //-------------------------------------------------------
	      for( int n = 0; n < 4; n++ )
		{
		  int x = xvec[0] + dx4[n];
		  int y = yvec[0] + dy4[n];
		  if( (x >= 0 && x < m_width) && (y >= 0 && y < m_height) )
		    {
		      int nindex = y*m_width + x;
		      if(nlabels[nindex] >= 0) adjlabel = nlabels[nindex];
		    }
		}
	      
	      int count = 1;
	      for( int c = 0; c < count; c++ )
		{
		  for( int n = 0; n < 4; n++ )
		    {
		      int x = xvec[c] + dx4[n];
		      int y = yvec[c] + dy4[n];
		      
		      if( (x >= 0 && x < m_width) && (y >= 0 && y < m_height) )
			{
			  int nindex = y*m_width + x;
			  
			  if(nlabels[nindex] < 0 && m_klabels[oindex] == m_klabels[nindex] )
			    {
			      xvec[count] = x;
			      yvec[count] = y;
			      nlabels[nindex] = label;
			      count++;
			    }
			}
		      
		    }
		}
	      
		//-------------------------------------------------------
	      // If segment size is less then a limit, assign an
	      // adjacent label found before, and decrement label count.
	      //-------------------------------------------------------
	      if(count <= SUPSZ >> 2)
		{
		  for( int c = 0; c < count; c++ )
		    {
		      int ind = yvec[c]*m_width+xvec[c];
		      nlabels[ind] = adjlabel;
		    }
		  label--;
		}
	      label++;
	    }
	  oindex++;
	}
    }

  m_numlabels = label;
  
  delete [] xvec;
  delete [] yvec;
}

void
SLIC::DrawContoursAroundSegmentsTwoColors(ushort* img,
					  int* labels,
					  int width,
					  int height)
{
  const int dx[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
  const int dy[8] = { 0, -1, -1, -1, 0, 1, 1,  1};
  
  int sz = width*height;
  
  memset(img, 0, sz*2);

  QVector<bool> istaken(sz, false);

  QVector<int> contourx(sz, 0);
  QVector<int> contoury(sz, 0);

  int mainindex = 0;
  int cind = 0;

  for( int j = 0; j < height; j++ )
    {
      for( int k = 0; k < width; k++ )
	{
	  int np = 0;
	  for( int i = 0; i < 8; i++ )
	    {
	      int x = k + dx[i];
	      int y = j + dy[i];
	      
	      if( (x >= 0 && x < width) && (y >= 0 && y < height) )
		{
		  int index = y*width + x;
		  
		  if( false == istaken[index] )//comment this to obtain internal contours
		  {
		    if( labels[mainindex] != labels[index] ) np++;
		  }
		}
	    }
	  if( np > 1 )
	    {
	      contourx[cind] = k;
	      contoury[cind] = j;
	      istaken[mainindex] = true;
	      //img[mainindex] = color;
	      cind++;
	    }
	  mainindex++;
	}
    }
  
  int numboundpix = cind; //int(contourx.size());
  
  for( int j = 0; j < numboundpix; j++ )
    {
      int ii = contoury[j]*width + contourx[j];
      img[ii] = 255;
//      //----------------------------------
//      // Uncomment this for thicker lines
//      //----------------------------------
//      for( int n = 0; n < 8; n++ )
//	{
//	  int x = contourx[j] + dx[n];
//	  int y = contoury[j] + dy[n];
//	  if( (x >= 0 && x < width) && (y >= 0 && y < height) )
//	    {
//	      int ind = y*width + x;
//	      if(!istaken[ind]) img[ind] = 0;
//	    }
//	}
    }
}

void
SLIC::MergeSuperPixels(int* nlabels,
		       QList<int> mkeys,
		       QList<float> lmeans,
		       float meanLbound,
		       int maxlabel)
{
  const int sz = m_width*m_height;

  QBitArray ignore;
  ignore.resize(sz);
  ignore.fill(true);

  QList<int> mels;
  for( int i = 0; i < sz; i++ )
    {
      if (mkeys.contains(nlabels[i]))	  
	ignore.setBit(i,false);
    }

  for( int i = 0; i < sz; i++ )
    {
      if (ignore.testBit(i))
	{
	  float lme = m_meanl[i];
	  for(int a=0; a<lmeans.count(); a++)
	    {
	      if (lme > 0 && qAbs(lme-lmeans[a]) < meanLbound)
		{
		  ignore.setBit(i, false);
		  break;
		}
	    }
	}
    }

  QMap<int, int> all_labels;
  for( int i = 0; i < sz; i++ )
    {
      if (!ignore.testBit(i))
	all_labels[nlabels[i]] = 1;
    }
  
  QList<int> labelkeys = all_labels.uniqueKeys();
  //QMessageBox::information(0, "", QString("labels : %1").arg(labelkeys.count()));
  
  int *newlabels = new int[sz]; 
  for( int i = 0; i < sz; i++ )
    newlabels[i] = -1;

  //------------------------------------------------
  //------------------------------------------------

  //int maxlabel = labelkeys.count() + 10;

  QQueue<QPoint> que;
  QBitArray bitmask;
  bitmask.resize(sz);
  bitmask.fill(false);

  for( int j = 0; j < m_height; j++ )
    for( int k = 0; k < m_width; k++ )
      {
	int idx = j*m_width + k;
	if (mkeys.contains(nlabels[idx]))
	  {
	    bitmask.setBit(idx, true);
	    newlabels[idx] = maxlabel;
	    que << QPoint(j,k);
	  }
      }

  const int dx4[4] = {-1,  0,  1,  0};
  const int dy4[4] = { 0, -1,  0,  1};
  while (!que.isEmpty())
    {
      QPoint jk = que.dequeue();
      int j = jk.x();
      int k = jk.y();
      for( int n = 0; n < 4; n++ )
	{
	  int x = k + dx4[n];
	  int y = j + dy4[n];
	  int idx = y*m_width + x;
	  if( (x >= 0 && x < m_width) &&
	      (y >= 0 && y < m_height) &&
	      !ignore.testBit(idx) &&
	      !bitmask.testBit(idx))
	    {
	      bitmask.setBit(idx, true);
	      newlabels[idx] = maxlabel;
	      que << QPoint(y,x);
	    }	  
	}
    }

  for( int i = 0; i < sz; i++ )
    {
      if (newlabels[i] != maxlabel)
	newlabels[i] = 0;
    }

  all_labels.clear();
  for( int i = 0; i < sz; i++ )
    all_labels[newlabels[i]] = 1;


  labelkeys = all_labels.uniqueKeys();
  //QMessageBox::information(0, "", QString("merged labels : %1").arg(labelkeys.count()));

  //------------------------------------------------
  //------------------------------------------------

  //m_numlabels = labelkeys.count();
  
  memcpy(nlabels, newlabels, sz*sizeof(int));

  delete newlabels;
}
