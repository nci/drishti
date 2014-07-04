#include "global.h"
#include "staticfunctions.h"
#include "trisetobject.h"
#include "matrix.h"
#include "ply.h"
#include "matrix.h"
#include "volumeinformation.h"

#include <QFileDialog>

void
TrisetObject::gridSize(int &nx, int &ny, int &nz)
{
  nx = m_nX;
  ny = m_nY;
  nz = m_nZ;
}

void TrisetObject::setOpacity(float op) { m_opacity = op; }

TrisetObject::TrisetObject()
{
  m_scrV = 0;
  m_scrD = 0;
  clear();
}

TrisetObject::~TrisetObject() { clear(); }

void
TrisetObject::enclosingBox(Vec &boxMin,
			   Vec &boxMax)
{
  boxMin = m_enclosingBox[0];
  boxMax = m_enclosingBox[6];
}

void
TrisetObject::clear()
{
  m_fileName.clear();
  m_centroid = Vec(0,0,0);
  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);
  m_nX = m_nY = m_nZ = 0;
  m_color = Vec(0.3f,0.6f,0.8f);
  m_cropcolor = Vec(0.0f,0.0f,0.0f);
  m_opacity = 1.0f;
  m_specular = 1.0f;
  m_diffuse = 1.0f;
  m_ambient = 0.0f;
  m_pointMode = true;
  m_blendMode = false;
  m_shadows = false;
  m_flipNormals = false;
  m_screenDoor = false;
  m_pointSize = 5;
  m_pointStep = 1;
  m_vertices.clear();
  m_vcolor.clear();
  m_drawcolor.clear();
  m_normals.clear();
  m_triangles.clear();
  m_texValues.clear();
  m_tvertices.clear();
  m_tnormals.clear();

  if (m_scrV) delete [] m_scrV;
  if (m_scrD) delete [] m_scrD;
  m_scrV = 0;
  m_scrD = 0;
}

bool
TrisetObject::load(QString flnm)
{
  if (StaticFunctions::checkExtension(flnm, ".triset"))
    return loadTriset(flnm);
  else
    return loadPLY(flnm);

  return false;
}

bool
TrisetObject::loadPLY(QString flnm)
{
  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  typedef struct Vertex {
    float x,y,z;
    float r,g,b;
    float nx,ny,nz;
    void *other_props;       /* other properties */
  } Vertex;

  typedef struct Face {
    unsigned char nverts;    /* number of vertex indices in list */
    int *verts;              /* vertex index list */
    void *other_props;       /* other properties */
  } Face;

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {"x", Float32, Float32, offsetof(Vertex,x), 0, 0, 0, 0},
    {"y", Float32, Float32, offsetof(Vertex,y), 0, 0, 0, 0},
    {"z", Float32, Float32, offsetof(Vertex,z), 0, 0, 0, 0},
    {"red", Float32, Float32, offsetof(Vertex,r), 0, 0, 0, 0},
    {"green", Float32, Float32, offsetof(Vertex,g), 0, 0, 0, 0},
    {"blue", Float32, Float32, offsetof(Vertex,b), 0, 0, 0, 0},
    {"nx", Float32, Float32, offsetof(Vertex,nx), 0, 0, 0, 0},
    {"ny", Float32, Float32, offsetof(Vertex,ny), 0, 0, 0, 0},
    {"nz", Float32, Float32, offsetof(Vertex,nz), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {"vertex_indices", Int32, Int32, offsetof(Face,verts),
     1, Uint8, Uint8, offsetof(Face,nverts)},
  };


  /*** the PLY object ***/

  int nverts,nfaces;
  Vertex **vlist;
  Face **flist;

  PlyOtherProp *vert_other,*face_other;

  bool per_vertex_color = false;
  bool has_normals = false;

  int i,j;
  int elem_count;
  char *elem_name;
  PlyFile *in_ply;


  /*** Read in the original PLY object ***/
  FILE *fp = fopen(flnm.toLatin1().data(), "rb");

  in_ply  = read_ply (fp);

  for (i = 0; i < in_ply->num_elem_types; i++) {

    /* prepare to read the i'th list of elements */
    elem_name = setup_element_read_ply (in_ply, i, &elem_count);


    if (equal_strings ("vertex", elem_name)) {

      /* create a vertex list to hold all the vertices */
      vlist = (Vertex **) malloc (sizeof (Vertex *) * elem_count);
      nverts = elem_count;

      /* set up for getting vertex elements */

      setup_property_ply (in_ply, &vert_props[0]);
      setup_property_ply (in_ply, &vert_props[1]);
      setup_property_ply (in_ply, &vert_props[2]);

      for (j = 0; j < in_ply->elems[i]->nprops; j++) {
	PlyProperty *prop;
	prop = in_ply->elems[i]->props[j];
	if (equal_strings ("r", prop->name) ||
	    equal_strings ("red", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[3]);
	  per_vertex_color = true;
	}
	if (equal_strings ("g", prop->name) ||
	    equal_strings ("green", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[4]);
	  per_vertex_color = true;
	}
	if (equal_strings ("b", prop->name) ||
	    equal_strings ("blue", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[5]);
	  per_vertex_color = true;
	}
	if (equal_strings ("nx", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[6]);
	  has_normals = true;
	}
	if (equal_strings ("ny", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[7]);
	  has_normals = true;
	}
	if (equal_strings ("nz", prop->name)) {
	  setup_property_ply (in_ply, &vert_props[8]);
	  has_normals = true;
	}
      }

      vert_other = get_other_properties_ply (in_ply, 
					     offsetof(Vertex,other_props));

      /* grab all the vertex elements */
      for (j = 0; j < elem_count; j++) {
        vlist[j] = (Vertex *) malloc (sizeof (Vertex));
        get_element_ply (in_ply, (void *) vlist[j]);
      }
    }
    else if (equal_strings ("face", elem_name)) {

      /* create a list to hold all the face elements */
      flist = (Face **) malloc (sizeof (Face *) * elem_count);
      nfaces = elem_count;

      /* set up for getting face elements */

      setup_property_ply (in_ply, &face_props[0]);
      face_other = get_other_properties_ply (in_ply, 
					     offsetof(Face,other_props));

      /* grab all the face elements */
      for (j = 0; j < elem_count; j++) {
        flist[j] = (Face *) malloc (sizeof (Face));
        get_element_ply (in_ply, (void *) flist[j]);
      }
    }
    else
      get_other_element_ply (in_ply);
  }

  close_ply (in_ply);
  free_ply (in_ply);
  
  if (Global::volumeType() == Global::DummyVolume)
    {
      float minX, maxX;
      float minY, maxY;
      float minZ, maxZ;
      minX = maxX = vlist[0]->x;
      minY = maxY = vlist[0]->y;
      minZ = maxZ = vlist[0]->z;
      for(int i=0; i<nverts; i++)
	{
	  minX = qMin(minX, vlist[i]->x);
	  maxX = qMax(maxX, vlist[i]->x);
	  minY = qMin(minY, vlist[i]->y);
	  maxY = qMax(maxY, vlist[i]->y);
	  minZ = qMin(minZ, vlist[i]->z);
	  maxZ = qMax(maxZ, vlist[i]->z);
	}
      minX = floor(minX);
      minY = floor(minY);
      minZ = floor(minZ);
      maxX = ceil(maxX);
      maxY = ceil(maxY);
      maxZ = ceil(maxZ);
      int h = maxX-minX+1;
      int w = maxY-minY+1;
      int d = maxZ-minZ+1;

      m_nX = d;
      m_nY = w;
      m_nZ = h;
      m_position = Vec(-minX, -minY, -minZ);

//      bool ok;
//      QString text = QInputDialog::getText(0,
//					   "Please enter grid size",
//					   "Grid Size",
//					   QLineEdit::Normal,
//					   QString("%1 %2 %3").\
//					   arg(d).arg(w).arg(h),
//					   &ok);
//      if (!ok || text.isEmpty())
//	{
//	  QMessageBox::critical(0, "Cannot load PLY", "No grid");
//	  return false;
//	}
//      
//      int nx=0;
//      int ny=0;
//      int nz=0;
//      QStringList gs = text.split(" ", QString::SkipEmptyParts);
//      if (gs.count() > 0) nx = gs[0].toInt();
//      if (gs.count() > 1) ny = gs[1].toInt();
//      if (gs.count() > 2) nz = gs[2].toInt();
//      if (nx > 0 && ny > 0 && nz > 0)
//	{
//	  m_nX = nx;
//	  m_nY = ny;
//	  m_nZ = nz;
//	}
//      else
//	{
//	  QMessageBox::critical(0, "Cannot load triset", "No grid");
//	  return false;
//	}
//
//      if (d == m_nX && w == m_nY && h == m_nZ)
//	m_position = Vec(-minX, -minY, -minZ);
    }
  else
    {
      Vec dim = VolumeInformation::volumeInformation(0).dimensions;
      m_nZ = dim.x;
      m_nY = dim.y;
      m_nX = dim.z;
    }

  m_vertices.resize(nverts);
  for(int i=0; i<nverts; i++)
    m_vertices[i] = Vec(vlist[i]->x,
			vlist[i]->y,
			vlist[i]->z);


  m_normals.clear();
  if (has_normals)
    {
      m_normals.resize(nverts);
      for(int i=0; i<nverts; i++)
	m_normals[i] = Vec(vlist[i]->nx,
			   vlist[i]->ny,
			   vlist[i]->nz);
    }

  m_vcolor.clear();
  if (per_vertex_color)
    {
      m_vcolor.resize(nverts);
      for(int i=0; i<nverts; i++)
	m_vcolor[i] = Vec(vlist[i]->r/255.0f,
			  vlist[i]->g/255.0f,
			  vlist[i]->b/255.0f);
    }

  // only triangles considered
  int ntri=0;
  for (int i=0; i<nfaces; i++)
    {
      if (flist[i]->nverts >= 3)
	ntri++;
    }
  m_triangles.resize(3*ntri);

  int tri=0;
  for(int i=0; i<nfaces; i++)
    {
      if (flist[i]->nverts >= 3)
	{
	  m_triangles[3*tri+0] = flist[i]->verts[0];
	  m_triangles[3*tri+1] = flist[i]->verts[1];
	  m_triangles[3*tri+2] = flist[i]->verts[2];
	  tri++;
	}
    }



  m_tvertices.resize(nverts);
  m_tnormals.resize(nverts);
  m_texValues.resize(nverts);


  Vec bmin = m_vertices[0];
  Vec bmax = m_vertices[0];
  for(int i=0; i<nverts; i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertices[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertices[i]);
    }
  m_centroid = (bmin + bmax)/2;

  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);


  m_pointStep = qMax(1, nverts/50000);

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).	\
//			   arg(m_vertices.count()).		\
//			   arg(m_triangles.count()/3));

  m_fileName = flnm;

  return true;
}

bool
TrisetObject::loadTriset(QString flnm)
{
  QFile fd(flnm);
  fd.open(QFile::ReadOnly);

  uchar stype = 0;
  fd.read((char*)&stype, sizeof(uchar));
  if (stype != 0)
    {
      QMessageBox::critical(0, "Cannot load triset",
			    "Wrong input format : First byte not equal to 0");
      return false;
    }

  fd.read((char*)&m_nX, sizeof(int));
  fd.read((char*)&m_nY, sizeof(int));
  fd.read((char*)&m_nZ, sizeof(int));


  int nvert, ntri;
  fd.read((char*)&nvert, sizeof(int));
  fd.read((char*)&ntri, sizeof(int));
   

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).\
//			   arg(nvert).\
//			   arg(ntri));


  float *vert = new float[nvert*3];
  fd.read((char*)vert, sizeof(float)*3*nvert);

  float *vnorm = new float[nvert*3];
  fd.read((char*)vnorm, sizeof(float)*3*nvert);

  int *tri = new int[ntri*3];
  fd.read((char*)tri, sizeof(int)*3*ntri);

  fd.close();


  m_vertices.resize(nvert);
  for(int i=0; i<nvert; i++)
    m_vertices[i] = Vec(vert[3*i],
			vert[3*i+1],
			vert[3*i+2]);
  delete [] vert;

  m_normals.resize(nvert);
  for(int i=0; i<nvert; i++)
    m_normals[i] = Vec(vnorm[3*i],
		       vnorm[3*i+1],
		       vnorm[3*i+2]);
  delete [] vnorm;

  m_triangles.resize(3*ntri);
  for(int i=0; i<3*ntri; i++)
    m_triangles[i] = tri[i];
  delete [] tri;


  m_tvertices.resize(nvert);
  m_tnormals.resize(nvert);
  m_texValues.resize(nvert);


  Vec bmin = m_vertices[0];
  Vec bmax = m_vertices[0];
  for(int i=0; i<nvert; i++)
    {
      bmin = StaticFunctions::minVec(bmin, m_vertices[i]);
      bmax = StaticFunctions::maxVec(bmax, m_vertices[i]);
    }
  m_centroid = (bmin + bmax)/2;

  m_position = Vec(0,0,0);
  m_scale = Vec(1,1,1);

  m_enclosingBox[0] = Vec(bmin.x, bmin.y, bmin.z);
  m_enclosingBox[1] = Vec(bmax.x, bmin.y, bmin.z);
  m_enclosingBox[2] = Vec(bmax.x, bmax.y, bmin.z);
  m_enclosingBox[3] = Vec(bmin.x, bmax.y, bmin.z);
  m_enclosingBox[4] = Vec(bmin.x, bmin.y, bmax.z);
  m_enclosingBox[5] = Vec(bmax.x, bmin.y, bmax.z);
  m_enclosingBox[6] = Vec(bmax.x, bmax.y, bmax.z);
  m_enclosingBox[7] = Vec(bmin.x, bmax.y, bmax.z);


  m_pointStep = qMax(1, nvert/50000);

//  QMessageBox::information(0, "", QString("%1 %2 %3\n%4 %5").	\
//			   arg(m_nX).arg(m_nY).arg(m_nZ).\
//			   arg(m_vertices.count()).\
//			   arg(m_triangles.count()));

  m_fileName = flnm;

  return true;
}


void
TrisetObject::postdraw(QGLViewer *viewer,
		       int x, int y,
		       bool active, int idx)
{
  if (!active)
    return;

  viewer->startScreenCoordinatesSystem();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // blend on top

  QString str = QString("triset %1").arg(idx);
  QFont font = QFont();
  QFontMetrics metric(font);
  int ht = metric.height();
  int wd = metric.width(str);
  //x -= wd/2;
  x += 10;

  StaticFunctions::renderText(x+2, y, str, font, Qt::black, Qt::white);

  viewer->stopScreenCoordinatesSystem();
}

void
TrisetObject::draw(QGLViewer *viewer,
		   bool active,
		   Vec lightPosition,
		   float pnear, float pfar, Vec step)
{
  if (active)
    {
      Vec lineColor = Vec(0.7f, 0.3f, 0);
      StaticFunctions::drawEnclosingCube(m_tenclosingBox, lineColor);
    }
  
  if (m_opacity < 0.05)
    return;

  float pos[4];
  float amb[4];
  float diff[4];
  float spec[4];
  float shine = 128*m_specular;

  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LIGHT0);

  //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // assume infinite view point
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  for (int i=0; i<4; i++)
    spec[i] = m_specular;

  for (int i=0; i<4; i++)
    diff[i] = m_diffuse;

  for (int i=0; i<4; i++)
    amb[i] = m_ambient;


  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shine);

  // emissive when active
  if (active)
    {
      float emiss[] = { 0.5f, 0, 0, 1 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }
  else
    {
      float emiss[] = { 0, 0, 0, 1 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
    }


  glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
  if (spec > 0)
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  pos[0] = lightPosition.x;
  pos[1] = lightPosition.y;
  pos[2] = lightPosition.z;
  pos[3] = 0;
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  glColor4f(m_color.x*m_opacity,
	    m_color.y*m_opacity,
	    m_color.z*m_opacity,
	    m_opacity);

  if (m_blendMode)
    drawTriset(pnear, pfar, step);
  else
    drawTriset();

  { // reset emissivity
    float emiss[] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emiss);
  }

  glDisable(GL_LIGHTING);
}

void
TrisetObject::predraw(QGLViewer *viewer,
		      double *Xform,
		      Vec pn,
		      bool shadows, int shadowWidth, int shadowHeight)
{
  if (m_opacity < 0.05)
    return;

  double *sp0 = new double[16];
  double *sp1 = new double[16];
  double *scl = new double[16];
  Matrix::identity(sp0);
  sp0[3] = m_centroid.x;
  sp0[7] = m_centroid.y;
  sp0[11]= m_centroid.z;

  Matrix::identity(scl);
  scl[0] = m_scale.x;
  scl[5] = m_scale.y;
  scl[10] = m_scale.z;
  Matrix::matmult(sp0,scl,sp1);  // sp1 = sp0 * scl

  Matrix::identity(sp0);
  sp0[3] = -m_centroid.x;
  sp0[7] = -m_centroid.y;
  sp0[11]= -m_centroid.z;  
  Matrix::matmult(sp1, sp0, scl); // scl = sp1 * sp0

  double *localXform = new double[16];
  Matrix::matmult(scl, Xform, sp1);

  Matrix::identity(sp0);
  sp0[3] = m_position.x;
  sp0[7] = m_position.y;
  sp0[11]= m_position.z;
  Matrix::matmult(sp1, sp0, localXform);

  delete [] sp0;
  delete [] sp1;
  delete [] scl;


  m_tcentroid = Matrix::xformVec(localXform, m_centroid);

  for(int i=0; i<8; i++)
    m_tenclosingBox[i] = Matrix::xformVec(localXform, m_enclosingBox[i]);

  for(int i=0; i<m_vertices.count(); i++)
    m_tvertices[i] = Matrix::xformVec(localXform, m_vertices[i]);


  if (m_normals.count() > 0)
    {  int fn = 1;
      if (m_flipNormals)
	fn = -1;
      for(int i=0; i<m_normals.count(); i++)
	m_tnormals[i] = Matrix::rotateVec(localXform, (fn*m_normals[i]));
    }

  if (m_blendMode)
    {
      if (!shadows || !m_shadows)
	{
	  for(int i=0; i<m_vertices.count(); i++)
	    m_texValues[i] = Vec(pn*m_tvertices[i], 0, 0);
	}
      else
	{
	  for(int i=0; i<m_vertices.count(); i++)
	    {
	      Vec scr = viewer->camera()->projectedCoordinatesOf(m_tvertices[i]);
	      float tx = scr.x;
	      float ty = shadowHeight-scr.y;
	      
	      float d = pn * m_tvertices[i];
	      
	      m_texValues[i] = Vec(d, tx, ty);
	    }
	}
    }

  bool black = (m_color.x<0.1 && m_color.y<0.1 && m_color.z<0.1);
  if (black)
    {
      m_drawcolor.clear();
      m_drawcolor = m_vcolor;
      for(int i=0; i<m_drawcolor.count(); i++)
	m_drawcolor[i] *= m_opacity;
    }


  delete [] localXform;
}

void
TrisetObject::makeReadyForPainting(QGLViewer *viewer)
{
  if (m_scrV) delete [] m_scrV;
  if (m_scrD) delete [] m_scrD;

  int swd = viewer->camera()->screenWidth();
  int sht = viewer->camera()->screenHeight();
  m_scrV = new uint[swd*sht];
  m_scrD = new float[swd*sht];

  for(int i=0; i<swd*sht; i++)
    m_scrD[i] = -1;
  for(int i=0; i<swd*sht; i++)
    m_scrV[i] = 1000000000; // a very large number - we will not have billion vertices

  for(int i=0; i<m_tvertices.count(); i++)
    {
      Vec scr = viewer->camera()->projectedCoordinatesOf(m_tvertices[i]);
      int tx = scr.x;
      int ty = sht-scr.y;
      if (tx>0 && tx<swd && ty>0 && ty<sht)
	{
	  if (scr.z > 0.0 && scr.z > m_scrD[tx*sht+ty])
	    {
	      m_scrV[tx*sht + ty] = i;
	      m_scrD[tx*sht + ty] = scr.z;
	    }
	}
    }

  if (m_vcolor.count() == 0)
    { // create per vertex color and fill with white
      m_vcolor.resize(m_vertices.count());
      m_vcolor.fill(Vec(1,1,1));
    }

  bool black = (m_color.x<0.1 && m_color.y<0.1 && m_color.z<0.1);
  if (!black) // make it black so that actual vertex colors are displayed
    m_color = Vec(0,0,0);
}

void
TrisetObject::releaseFromPainting()
{
  if (m_scrV) delete [] m_scrV;
  if (m_scrD) delete [] m_scrD;
  m_scrV = 0;
  m_scrD = 0;
}

void
TrisetObject::paint(QGLViewer *viewer,
		    QBitArray doodleMask,
		    float *depthMap,
		    Vec tcolor, float tmix)
{
  int swd = viewer->camera()->screenWidth();
  int sht = viewer->camera()->screenHeight();

  for(int i=0; i<m_tvertices.count(); i++)
    {
      Vec scr = viewer->camera()->projectedCoordinatesOf(m_tvertices[i]);
      int tx = scr.x;
      int ty = sht-scr.y;
      float td = scr.z;
      if (tx>0 && tx<swd && ty>0 && ty<sht)
	{
	  int idx = ty*swd + tx;
	  if (doodleMask.testBit(idx))
	    {
	      float zd = depthMap[idx];
	      if (fabs(zd-td) < 0.0002)
		m_vcolor[i] = tmix*tcolor + (1.0-tmix)*m_vcolor[i];
	    }
	}
    }
}

void
TrisetObject::drawTriset(float pnear, float pfar, Vec step)
{
  glEnable(GL_DEPTH_TEST);

  bool black = (m_color.x<0.1 && m_color.y<0.1 && m_color.z<0.1);
  bool has_normals = (m_normals.count() > 0);
  bool per_vertex_color = (m_vcolor.count() > 0 && black);
  if (m_pointMode)
    {
      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);
      glPointSize(m_pointSize);
      glBegin(GL_POINTS);
      for(int i=0; i<m_triangles.count()/3; i+=m_pointStep)
	{
	  int v0 = m_triangles[3*i];
	  if ( m_texValues[v0].x >= pnear &&
	       m_texValues[v0].x <= pfar )
	    {
	      if (has_normals) glNormal3dv(m_tnormals[v0]);
	      if (per_vertex_color)
		glColor4f(m_drawcolor[v0].x, 
			  m_drawcolor[v0].y, 
			  m_drawcolor[v0].z,
			  m_opacity);

	      glVertex3dv(m_tvertices[v0]);
	    }
	}
      glEnd();
      glPointSize(1);
      glDisable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
    }
  else
    {
      glBegin(GL_TRIANGLES);
      for(int i=0; i<m_triangles.count()/3; i++)
	{
	  int v0 = m_triangles[3*i];
	  int v1 = m_triangles[3*i+1];
	  int v2 = m_triangles[3*i+2];
	  
	  if ( ! ((m_texValues[v0].x < pnear &&
		   m_texValues[v1].x < pnear &&
		   m_texValues[v2].x < pnear) ||
		  (m_texValues[v0].x > pfar  &&
		   m_texValues[v1].x > pfar  &&
		   m_texValues[v2].x > pfar)) )
	    {
	      glMultiTexCoord2d(GL_TEXTURE0,
				m_texValues[v0].y,
				m_texValues[v0].z);
	      if (has_normals) glNormal3dv(m_tnormals[v0]);
	      if (per_vertex_color)
		glColor4f(m_drawcolor[v0].x, 
			  m_drawcolor[v0].y, 
			  m_drawcolor[v0].z,
			  m_opacity);
	      glMultiTexCoord3dv(GL_TEXTURE2, m_tvertices[v0]);
	      glVertex3dv(m_tvertices[v0]+step);
	      
	      glMultiTexCoord2d(GL_TEXTURE0,
				m_texValues[v1].y,
				m_texValues[v1].z);
	      if (has_normals) glNormal3dv(m_tnormals[v1]);
	      if (per_vertex_color)
		glColor4f(m_drawcolor[v1].x, 
			  m_drawcolor[v1].y, 
			  m_drawcolor[v1].z,
			  m_opacity);
	      glMultiTexCoord3dv(GL_TEXTURE2, m_tvertices[v1]);
	      glVertex3dv(m_tvertices[v1]+step);
	      
	      glMultiTexCoord2d(GL_TEXTURE0,
				m_texValues[v2].y,
				m_texValues[v2].z);
	      if (has_normals) glNormal3dv(m_tnormals[v2]);
	      if (per_vertex_color)
		glColor4f(m_drawcolor[v2].x, 
			  m_drawcolor[v2].y, 
			  m_drawcolor[v2].z,
			  m_opacity);
	      glMultiTexCoord3dv(GL_TEXTURE2, m_tvertices[v2]);
	      glVertex3dv(m_tvertices[v2]+step);
	    }
	  
	}
      glEnd();
    }

  glDisable(GL_DEPTH_TEST);
}

void
TrisetObject::drawTriset()
{
  bool black = (m_color.x<0.1 && m_color.y<0.1 && m_color.z<0.1);
  bool has_normals = (m_normals.count() > 0);
  bool per_vertex_color = (m_vcolor.count() > 0 && black);
  if (m_pointMode)
    {
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, Global::spriteTexture());
      glTexEnvf( GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE );
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_POINT_SMOOTH);
      glPointSize(m_pointSize);
      glBegin(GL_POINTS);
      for(int i=0; i<m_triangles.count()/3; i+=m_pointStep)
	{
	  int v0 = m_triangles[3*i];
	  if (has_normals) glNormal3dv(m_tnormals[v0]);
	  if (per_vertex_color)
	    glColor4f(m_drawcolor[v0].x, 
		      m_drawcolor[v0].y, 
		      m_drawcolor[v0].z,
		      m_opacity);
	  glVertex3dv(m_tvertices[v0]);
	}
      glEnd();
      glPointSize(1);
      glDisable(GL_POINT_SPRITE);
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
    }
  else
    {
      glBegin(GL_TRIANGLES);
      for(int i=0; i<m_triangles.count()/3; i++)
	{
	  int v0 = m_triangles[3*i];
	  int v1 = m_triangles[3*i+1];
	  int v2 = m_triangles[3*i+2];
	  
	  if (has_normals) glNormal3fv(m_tnormals[v0]);
	  if (per_vertex_color)
	    glColor4f(m_drawcolor[v0].x, 
		      m_drawcolor[v0].y, 
		      m_drawcolor[v0].z,
		      m_opacity);
	  glVertex3fv(m_tvertices[v0]);
	      
	  if (has_normals) glNormal3fv(m_tnormals[v1]);
	  if (per_vertex_color)
	    glColor4f(m_drawcolor[v1].x, 
		      m_drawcolor[v1].y, 
		      m_drawcolor[v1].z,
		      m_opacity);
	  glVertex3fv(m_tvertices[v1]);
	      
	  if (has_normals) glNormal3fv(m_tnormals[v2]);
	  if (per_vertex_color)
	    glColor4f(m_drawcolor[v2].x, 
		      m_drawcolor[v2].y, 
		      m_drawcolor[v2].z,
		      m_opacity);
	  glVertex3fv(m_tvertices[v2]);	  
	}
      glEnd();
    }
}

QDomElement
TrisetObject::domElement(QDomDocument &doc)
{
  QDomElement de = doc.createElement("triset");
  
  {
    QDomElement de0 = doc.createElement("name");
    QDomText tn0 = doc.createTextNode(m_fileName);
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("position");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_position.x).arg(m_position.y).arg(m_position.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("scale");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_scale.x).arg(m_scale.y).arg(m_scale.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("opacity");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_opacity));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("color");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_color.x).arg(m_color.y).arg(m_color.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("cropcolor");
    QDomText tn0 = doc.createTextNode(QString("%1 %2 %3").\
				      arg(m_cropcolor.x).arg(m_cropcolor.y).arg(m_cropcolor.z));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("ambient");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_ambient));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("diffuse");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_diffuse));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("specular");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_specular));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("pointmode");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_pointMode));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("pointsize");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_pointSize));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("pointstep");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_pointStep));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("blendmode");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_blendMode));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("flipnormals");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_flipNormals));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("screendoor");
    QDomText tn0 = doc.createTextNode(QString("%1").arg(m_screenDoor));
    de0.appendChild(tn0);
    de.appendChild(de0);
  }

  return de;
}

bool
TrisetObject::fromDomElement(QDomElement de)
{
  clear();

  bool ok = false;

  QString name;
  QDomNodeList dlist = de.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      QDomElement dnode = dlist.at(i).toElement();
      QString str = dnode.toElement().text();
      if (dnode.tagName() == "name")
	ok = load(str);
      else if (dnode.tagName() == "position")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_position = Vec(x,y,z);
	}
      else if (dnode.tagName() == "scale")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_scale = Vec(x,y,z);
	}
      else if (dnode.tagName() == "opacity")
	m_opacity = str.toFloat();
      else if (dnode.tagName() == "color")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_color = Vec(x,y,z);
	}
      else if (dnode.tagName() == "cropcolor")
	{
	  QStringList xyz = str.split(" ");
	  float x = 0;
	  float y = 0;
	  float z = 0;
	  if (xyz.size() > 0) x = xyz[0].toFloat();
	  if (xyz.size() > 1) y  = xyz[1].toFloat();
	  if (xyz.size() > 2) z  = xyz[2].toFloat();
	  m_cropcolor = Vec(x,y,z);
	}
      else if (dnode.tagName() == "ambient")
	m_ambient = str.toFloat();
      else if (dnode.tagName() == "diffuse")
	m_diffuse = str.toFloat();
      else if (dnode.tagName() == "specular")
	m_specular = str.toFloat();
      else if (dnode.tagName() == "pointmode")
	{
	  if (str == "yes" || str == "1") m_pointMode = true;
	  else m_pointMode = false;
	}
      else if (dnode.tagName() == "pointsize")
	m_pointSize = str.toFloat();
      else if (dnode.tagName() == "pointstep")
	m_pointStep = str.toFloat();
      else if (dnode.tagName() == "blendmode")
	{
	  if (str == "yes" || str == "1") m_pointMode = true;
	  else m_blendMode = false;
	}
      else if (dnode.tagName() == "screendoor")
	{
	  if (str == "yes" || str == "1") m_screenDoor = true;
	  else m_screenDoor = false;
	}
      else if (dnode.tagName() == "flipnormals")
	{
	  if (str == "yes" || str == "1") m_flipNormals = true;
	  else m_flipNormals = false;
	}
    }

  return ok;
}

TrisetInformation
TrisetObject::get()
{
  TrisetInformation ti;
  ti.filename = m_fileName;
  ti.position = m_position;
  ti.scale = m_scale;
  ti.pointMode = m_pointMode;
  ti.blendMode = m_blendMode;
  ti.shadows = m_shadows;
  ti.pointStep = m_pointStep;
  ti.pointSize = m_pointSize;
  ti.color = m_color;
  ti.cropcolor = m_cropcolor;
  ti.opacity = m_opacity;
  ti.ambient = m_ambient;
  ti.diffuse = m_diffuse;
  ti.specular = m_specular;
  ti.flipNormals = m_flipNormals;
  ti.screenDoor = m_screenDoor;

  return ti;
}

bool
TrisetObject::set(TrisetInformation ti)
{
  bool ok = false;

  if (m_fileName != ti.filename)
    ok = load(ti.filename);
  else
    ok = true;

  m_position = ti.position;
  m_scale = ti.scale;
  m_opacity = ti.opacity;
  m_color = ti.color;
  m_cropcolor = ti.cropcolor;
  m_ambient = ti.ambient;
  m_diffuse = ti.diffuse;
  m_specular = ti.specular;
  m_pointMode = ti.pointMode;
  m_pointSize = ti.pointSize;
  m_pointStep = ti.pointStep;
  m_blendMode = ti.blendMode;
  m_shadows = ti.shadows;
  m_flipNormals = ti.flipNormals;
  m_screenDoor = ti.screenDoor;

  return ok;
}

void
TrisetObject::save()
{
  bool has_normals = (m_normals.count() > 0);
  bool per_vertex_color = (m_vcolor.count() > 0);

  QString flnm = QFileDialog::getSaveFileName(0,
					      "Export mesh to file",
					      Global::previousDirectory(),
					      "*.ply");
  if (flnm.size() == 0)
    return;

  typedef struct PlyFace
  {
    unsigned char nverts;    /* number of Vertex indices in list */
    int *verts;              /* Vertex index list */
  } PlyFace;

  typedef struct
  {
    float  x,  y,  z;  /**< Vertex coordinates */
    float nx, ny, nz;  /**< Vertex normal */
    uchar r, g, b;
  } myVertex ;


  PlyProperty vert_props[]  = { /* list of property information for a PlyVertex */
    {"x", Float32, Float32,  offsetof( myVertex,x ), 0, 0, 0, 0},
    {"y", Float32, Float32,  offsetof( myVertex,y ), 0, 0, 0, 0},
    {"z", Float32, Float32,  offsetof( myVertex,z ), 0, 0, 0, 0},
    {"nx", Float32, Float32, offsetof( myVertex,nx ), 0, 0, 0, 0},
    {"ny", Float32, Float32, offsetof( myVertex,ny ), 0, 0, 0, 0},
    {"nz", Float32, Float32, offsetof( myVertex,nz ), 0, 0, 0, 0},
    {"red", Uint8, Uint8,    offsetof( myVertex,r ), 0, 0, 0, 0},
    {"green", Uint8, Uint8,  offsetof( myVertex,g ), 0, 0, 0, 0},
    {"blue", Uint8, Uint8,   offsetof( myVertex,b ), 0, 0, 0, 0}
  };

  PlyProperty face_props[]  = { /* list of property information for a PlyFace */
    {"vertex_indices", Int32, Int32, offsetof( PlyFace,verts ),
      1, Uint8, Uint8, offsetof( PlyFace,nverts )},
  };


  PlyFile    *ply;
  FILE       *fp = fopen(flnm.toLatin1().data(),
			 bin ? "wb" : "w");

  PlyFace     face ;
  int         verts[3] ;
  char       *elem_names[]  = { "vertex", "face" };
  ply = write_ply (fp,
		   2,
		   elem_names,
		   bin? PLY_BINARY_LE : PLY_ASCII );

  int nvertices = m_vertices.count();
  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, "vertex", nvertices );
  describe_property_ply ( ply, &vert_props[0] );
  describe_property_ply ( ply, &vert_props[1] );
  describe_property_ply ( ply, &vert_props[2] );
  describe_property_ply ( ply, &vert_props[3] );
  describe_property_ply ( ply, &vert_props[4] );
  describe_property_ply ( ply, &vert_props[5] );
  describe_property_ply ( ply, &vert_props[6] );
  describe_property_ply ( ply, &vert_props[7] );
  describe_property_ply ( ply, &vert_props[8] );

  /* describe PlyFace properties (just list of PlyVertex indices) */
  int ntriangles = m_triangles.count()/3;
  describe_element_ply ( ply, "face", ntriangles );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );


  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, "vertex" );

  for(int i=0; i<m_vertices.count(); i++)
    {
      myVertex vertex;
      vertex.x = m_vertices[i].x*m_scale.x;
      vertex.y = m_vertices[i].y*m_scale.y;
      vertex.z = m_vertices[i].z*m_scale.z;
      if (has_normals)
	{
	  vertex.nx = m_normals[i].x;
	  vertex.ny = m_normals[i].y;
	  vertex.nz = m_normals[i].z;
	}
      if (per_vertex_color)
	{
	  vertex.r = 255*m_vcolor[i].x;
	  vertex.g = 255*m_vcolor[i].y;
	  vertex.b = 255*m_vcolor[i].z;
	}
      put_element_ply ( ply, ( void * ) &vertex );
    }

  put_element_setup_ply ( ply, "face" );
  face.nverts = 3 ;
  face.verts  = verts ;
  for(int i=0; i<m_triangles.count()/3; i++)
    {
      int v0 = m_triangles[3*i];
      int v1 = m_triangles[3*i+1];
      int v2 = m_triangles[3*i+2];

      face.verts[0] = v0;
      face.verts[1] = v1;
      face.verts[2] = v2;

      put_element_ply ( ply, ( void * ) &face );
    }

  close_ply ( ply );
  free_ply ( ply );
  fclose( fp ) ;

  QMessageBox::information(0, "Save Mesh", "done");
}
