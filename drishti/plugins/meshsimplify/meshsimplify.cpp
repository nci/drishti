#include "staticfunctions.h"
#include "meshsimplify.h"
#include "simplify.h"

MeshSimplify::MeshSimplify()
{
  vcolor=0;

  QStringList ps;
  ps << "x";
  ps << "y";
  ps << "z";
  ps << "nx";
  ps << "ny";
  ps << "nz";
  ps << "red";
  ps << "green";
  ps << "blue";
  ps << "vertex_indices";
  ps << "vertex";
  ps << "face";

  for(int i=0; i<ps.count(); i++)
    {
      char *s;
      s = new char[ps[i].size()+1];
      strcpy(s, ps[i].toLatin1().data());
      plyStrings << s;
    }
}
MeshSimplify::~MeshSimplify() {if (vcolor) delete [] vcolor;}

bool
MeshSimplify::getValues(float &decimate, int &aggressive, int &meshsmooth, int &colorsmooth)
{
  decimate = 0.2;
  aggressive = 5;
  meshsmooth = 0;
  colorsmooth = 10;
  
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(decimate);
  vlist << QVariant(0.0001);
  vlist << QVariant(0.9);
  vlist << QVariant(0.05); // singlestep
  vlist << QVariant(4); // decimals
  plist["decimate"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(aggressive);
  vlist << QVariant(1);
  vlist << QVariant(10);
  plist["aggressive"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(meshsmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["mesh smoothing"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(colorsmooth);
  vlist << QVariant(1);
  vlist << QVariant(100);
  plist["color smoothing"] = vlist;

  vlist.clear();
  QFile helpFile(":/meshsimplify.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }	      
  plist["commandhelp"] = vlist;

  vlist.clear();
  QString mesg;
  mesg += "\n* You can keep on working while this process is running.\n";
  vlist << mesg;
  plist["message"] = vlist;


  QStringList keys;
  keys << "mesh smoothing";
  keys << "decimate";
  keys << "aggressive";
  keys << "color smoothing";
  keys << "commandhelp";
  keys << "message";

  propertyEditor.set("Mesh Simplification Parameters", plist, keys);
  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return false;
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);

      if (pair.second)
	{
	  if (keys[ik] == "decimate")
	    decimate = pair.first.toFloat();
	  else if (keys[ik] == "aggressive")
	    aggressive = pair.first.toInt();
	  else if (keys[ik] == "mesh smoothing")
	    meshsmooth = pair.first.toInt();
	  else if (keys[ik] == "color smoothing")
	    colorsmooth = pair.first.toInt();
	}
    }

  return true;
}


QString
MeshSimplify::start(QString prevDir)
{
  QString inflnm, outflnm;
  bool plyType = true;
  
  //---- import the mesh ---
  inflnm = QFileDialog::getOpenFileName(0,
					"Load mesh to simplify",
					prevDir,
					"*.ply | *.obj");
  if (inflnm.size() == 0) return "";
  
  QFileInfo f(inflnm);
  
  
  if (StaticFunctions::checkExtension(inflnm, ".ply"))
    {
      plyType = true;
      outflnm = QFileDialog::getSaveFileName(0,
					     "Save simplified PLY mesh",
					     f.absolutePath(),
					     "*.ply");
      if (!StaticFunctions::checkExtension(outflnm, ".ply"))
	outflnm += ".ply";
    }
  if (StaticFunctions::checkExtension(inflnm, ".obj"))
    {
      plyType = false;
      outflnm = QFileDialog::getSaveFileName(0,
					     "Save simplified OBJ mesh",
					     f.absolutePath(),
					     "*.obj");
      if (!StaticFunctions::checkExtension(outflnm, ".obj"))
	outflnm += ".obj";
    }
  if (outflnm.size() == 0) return "";


  float decimate;
  int aggressive;
  int meshsmooth;
  int colorsmooth;

  if (! getValues(decimate, aggressive, meshsmooth, colorsmooth))
    return "";

  m_meshLog = new QTextEdit;
  m_meshProgress = new QProgressBar;

  QVBoxLayout *meshLayout = new QVBoxLayout;
  meshLayout->addWidget(m_meshLog);
  meshLayout->addWidget(m_meshProgress);

  QWidget *meshWindow = new QWidget;
  meshWindow->setWindowTitle("Drishti - Mesh Simplification");
  meshWindow->setLayout(meshLayout);
  meshWindow->show();
  meshWindow->resize(700, 700);

  m_meshLog->insertPlainText("\n\n");
  m_meshLog->insertPlainText(QString("Decimation : %1\n").arg(decimate));
  m_meshLog->insertPlainText(QString("Aggressive : %1\n").arg(aggressive));
  qApp->processEvents();
  

  simplifyMesh(plyType,
	       inflnm, outflnm,
	       decimate, aggressive,
	       meshsmooth, colorsmooth);
      
  meshWindow->close();

  return outflnm;
}


void
MeshSimplify::simplifyMesh(bool plyType,
			   QString inflnm, QString outflnm,
			   float decimate, int aggressive,
			   int meshsmooth, int colorsmooth)
{
  QString mesg;

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText(" Loading mesh ...\n");
  qApp->processEvents();

  if (plyType)
    {
      if (!loadPLY(inflnm))
	{
	  QMessageBox::information(0, "Error", "Cannot load "+inflnm);
	  return;
	}

      if (meshsmooth > 0)
	smoothMesh(meshsmooth);
      else
	m_meshLog->insertPlainText(" No smoothing applied before simplification.\n");


      Simplify::load_ply(m_nverts, m_nfaces, m_vlist, m_flist);
      for (int j = 0; j < m_nverts; j++) delete [] m_vlist[j];
      for (int j = 0; j < m_nfaces; j++)
	{
	  free(m_flist[j]->verts);
	  delete [] m_flist[j];
	}
      delete [] m_vlist;
      delete [] m_flist;
      m_vlist = 0;
      m_flist = 0;
    }
  else
    Simplify::load_obj(inflnm.toLatin1().data());

  
  int target_count =  Simplify::triangles.size() * decimate;
  int startSize = Simplify::triangles.size();
  
  mesg = QString("Input : %1 vertices,  %2 triangles (target %3)\n").	\
    arg(Simplify::vertices.size()).\
    arg(Simplify::triangles.size()).\
    arg(target_count);
	
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText(mesg);
  qApp->processEvents();


  Simplify::simplify_mesh(m_meshLog,
			  target_count,
			  aggressive,
			  true);

  mesg = QString("Output : %1 vertices,  %2 triangles (reduction %3)\n").\
    arg(Simplify::vertices.size()).\
    arg(Simplify::triangles.size()).\
    arg((float)Simplify::triangles.size()/(float)startSize);
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText(mesg);
  qApp->processEvents();

  //-----
  if (plyType)
    {
      applyColorSmoothing(colorsmooth);
      generateNormals();

      m_nverts = Simplify::vertices.size();
      m_nfaces = Simplify::triangles.size();
      m_vlist = (PlyVertex **) new uchar[sizeof (PlyVertex *) * m_nverts];
      m_flist = (PlyFace **) new uchar[sizeof (PlyFace *) * m_nfaces];
  
      for(int i=0; i<m_nverts; i++)
	{
	  m_vlist[i] = (PlyVertex *) new uchar[sizeof (PlyVertex)];
	  m_vlist[i]->x = Simplify::vertices[i].p.x;
	  m_vlist[i]->y = Simplify::vertices[i].p.y;
	  m_vlist[i]->z = Simplify::vertices[i].p.z;
	  m_vlist[i]->nx = Simplify::vertices[i].n.x;
	  m_vlist[i]->ny = Simplify::vertices[i].n.y;
	  m_vlist[i]->nz = Simplify::vertices[i].n.z;
	  m_vlist[i]->r = Simplify::vertices[i].c.x;
	  m_vlist[i]->g = Simplify::vertices[i].c.y;
	  m_vlist[i]->b = Simplify::vertices[i].c.z;
	}
      for(int i=0; i<m_nfaces; i++)
	{
	  m_flist[i] = (PlyFace *) new uchar[sizeof (PlyFace)];
	  m_flist[i]->verts = new int[3];
	  m_flist[i]->nverts = 3;
	  m_flist[i]->verts[0] = Simplify::triangles[i].v[0];
	  m_flist[i]->verts[1] = Simplify::triangles[i].v[1];
	  m_flist[i]->verts[2] = Simplify::triangles[i].v[2];
	}
      savePLY(outflnm);
    }
  else
    Simplify::write_obj(outflnm.toLatin1().data());
  //-----

  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Mesh saved in "+outflnm);

  QMessageBox::information(0, "", QString("Mesh saved in "+outflnm));
}

void
MeshSimplify::savePLY(QString flnm)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText("Saving Mesh");

  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(PlyVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(PlyVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(PlyVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(PlyVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(PlyVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(PlyVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(PlyVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(PlyVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(PlyVertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };


  PlyFile *ply;
  FILE *fp = fopen(flnm.toLatin1().data(), "wb");

  PlyFace face ;
  int verts[3] ;
  char *elem_names[]  = {plyStrings[10], plyStrings[11]};
  ply = write_ply (fp,
		   2,
		   elem_names,
		   PLY_BINARY_LE );

  /* describe what properties go into the PlyVertex elements */
  describe_element_ply ( ply, plyStrings[10], m_nverts );
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
  describe_element_ply ( ply, plyStrings[11], m_nfaces );
  describe_property_ply ( ply, &face_props[0] );

  header_complete_ply ( ply );

  /* set up and write the PlyVertex elements */
  put_element_setup_ply ( ply, plyStrings[10] );
  for(int ni=0; ni<m_nverts; ni++)
    put_element_ply ( ply, ( void * ) m_vlist[ni] );

  /* set up and write the PlyFace elements */
  put_element_setup_ply ( ply, plyStrings[11] );
  for(int ni=0; ni<m_nfaces; ni++)
    put_element_ply ( ply, ( void * ) m_flist[ni] );

  close_ply ( ply );
  free_ply ( ply );

  m_meshProgress->setValue(100);
}

bool
MeshSimplify::loadPLY(QString flnm)
{
  PlyProperty vert_props[] = { /* list of property information for a vertex */
    {plyStrings[0], Float32, Float32, offsetof(PlyVertex,x), 0, 0, 0, 0},
    {plyStrings[1], Float32, Float32, offsetof(PlyVertex,y), 0, 0, 0, 0},
    {plyStrings[2], Float32, Float32, offsetof(PlyVertex,z), 0, 0, 0, 0},
    {plyStrings[3], Float32, Float32, offsetof(PlyVertex,nx), 0, 0, 0, 0},
    {plyStrings[4], Float32, Float32, offsetof(PlyVertex,ny), 0, 0, 0, 0},
    {plyStrings[5], Float32, Float32, offsetof(PlyVertex,nz), 0, 0, 0, 0},
    {plyStrings[6], Uint8, Uint8, offsetof(PlyVertex,r), 0, 0, 0, 0},
    {plyStrings[7], Uint8, Uint8, offsetof(PlyVertex,g), 0, 0, 0, 0},
    {plyStrings[8], Uint8, Uint8, offsetof(PlyVertex,b), 0, 0, 0, 0},
  };

  PlyProperty face_props[] = { /* list of property information for a face */
    {plyStrings[9], Int32, Int32, offsetof(PlyFace,verts),
     1, Uint8, Uint8, offsetof(PlyFace,nverts)},
  };


  /*** the PLY object ***/
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


    if (QString("vertex") == QString(elem_name)) {

      /* create a vertex list to hold all the vertices */
      m_vlist = (PlyVertex **) new uchar[sizeof (PlyVertex *) * elem_count];
      m_nverts = elem_count;

      /* set up for getting vertex elements */

      setup_property_ply (in_ply, &vert_props[0]);
      setup_property_ply (in_ply, &vert_props[1]);
      setup_property_ply (in_ply, &vert_props[2]);

      for (j = 0; j < in_ply->elems[i]->nprops; j++) {
	PlyProperty *prop;
	prop = in_ply->elems[i]->props[j];
	if (QString("r") == QString(prop->name) ||
	    QString("red") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[3]);
	  per_vertex_color = true;
	}
	if (QString("g") == QString(prop->name) ||
	    QString("green") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[4]);
	  per_vertex_color = true;
	}
	if (QString("b") == QString(prop->name) ||
	    QString("blue") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[5]);
	  per_vertex_color = true;
	}
	if (QString("nx") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[6]);
	  has_normals = true;
	}
	if (QString("ny") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[7]);
	  has_normals = true;
	}
	if (QString("nz") == QString(prop->name)) {
	  setup_property_ply (in_ply, &vert_props[8]);
	  has_normals = true;
	}
      }

      /* grab all the vertex elements */
      for (j = 0; j < elem_count; j++) {
        m_vlist[j] = (PlyVertex *) new uchar[sizeof (PlyVertex)];
        get_element_ply (in_ply, (void *) m_vlist[j]);
      }
    }
    else if (QString("face") == QString(elem_name)) {

      /* create a list to hold all the face elements */
      m_flist = (PlyFace **) new uchar[sizeof (PlyFace *) * elem_count];
      m_nfaces = elem_count;

      /* set up for getting face elements */

      setup_property_ply (in_ply, &face_props[0]);

      /* grab all the face elements */
      for (j = 0; j < elem_count; j++) {
        m_flist[j] = (PlyFace *) new uchar[sizeof (PlyFace)];
        get_element_ply (in_ply, (void *) m_flist[j]);
      }
    }
    else
      get_other_element_ply (in_ply);
  }

  close_ply (in_ply);
  free_ply (in_ply);
  

  vcolor = new float[m_nverts*3];
  for(int i=0; i<m_nverts; i++)
    {
      vcolor[3*i+0] = m_vlist[i]->r/255.0f;
      vcolor[3*i+1] = m_vlist[i]->g/255.0f;
      vcolor[3*i+2] = m_vlist[i]->b/255.0f;
    }

//  QMessageBox::information(0, "", QString("Loaded input mesh : vertices %1  faces %2").\
//			   arg(m_nverts).\
//			   arg(m_nfaces));

  return true;
}

void
MeshSimplify::generateNormals()
{
  m_nverts = Simplify::vertices.size();
  m_nfaces = Simplify::triangles.size();

  for(int i=0; i<m_nverts; i++)
    {
      Simplify::vertices[i].n = vec3f(0,0,0);
    }
  
  for(int i=0; i<m_nfaces; i++)
    {
      int i0 = Simplify::triangles[i].v[0];
      int i1 = Simplify::triangles[i].v[1];
      int i2 = Simplify::triangles[i].v[2];

      vec3f n = Simplify::triangles[i].n;
      if (n.length() < 10)
	{
	  // add proportional to the area of triangle
	  vec3f p0 = Simplify::vertices[i0].p;
	  vec3f p1 = Simplify::vertices[i1].p;
	  vec3f p2 = Simplify::vertices[i2].p;
	  vec3f cp;
	  cp.cross(p1-p0,p2-p0);
	  double cpl = cp.length();
	  n = n*cpl;
	  
	  Simplify::vertices[i0].n = Simplify::vertices[i0].n + n;
	  Simplify::vertices[i1].n = Simplify::vertices[i1].n + n;
	  Simplify::vertices[i2].n = Simplify::vertices[i2].n + n;
	}
    }

  for(int i=0; i<m_nverts; i++)
    {
      Simplify::vertices[i].n.normalize();
    }
}

void
MeshSimplify::applyColorSmoothing(int smooth)
{
  m_nverts = Simplify::vertices.size();
  m_nfaces = Simplify::triangles.size();

  for(int iter = 0; iter < smooth; iter++)
    {
      // copy color to normal
      // normals will be calculated in generateNormals
      // so overwriting them should not cause problems
      for(int i=0; i<m_nverts; i++)
	{
	  Simplify::vertices[i].n = Simplify::vertices[i].c;
	  Simplify::vertices[i].tcount = 0;
	  Simplify::vertices[i].c = vec3f(0,0,0);
	}
      
      for(int i=0; i<m_nfaces; i++)
	{
	  int i0 = Simplify::triangles[i].v[0];
	  int i1 = Simplify::triangles[i].v[1];
	  int i2 = Simplify::triangles[i].v[2];
	  
	  // we have color in n
	  vec3f c0 = Simplify::vertices[i0].n;
	  vec3f c1 = Simplify::vertices[i1].n;
	  vec3f c2 = Simplify::vertices[i2].n;
	  vec3f c = (c0 + c1 + c2)/3;
	  
	  Simplify::vertices[i0].c = Simplify::vertices[i0].c + c;
	  Simplify::vertices[i1].c = Simplify::vertices[i1].c + c;
	  Simplify::vertices[i2].c = Simplify::vertices[i2].c + c;
	  
	  Simplify::vertices[i0].tcount += 1;
	  Simplify::vertices[i1].tcount += 1;
	  Simplify::vertices[i2].tcount += 1;
	}
      
      for(int i=0; i<m_nverts; i++)
	{
	  Simplify::vertices[i].c = Simplify::vertices[i].c / Simplify::vertices[i].tcount;
	}
    }
  
}

void
MeshSimplify::smoothMesh(int ntimes)
{
  m_meshLog->moveCursor(QTextCursor::End);
  m_meshLog->insertPlainText(" Applying smoothing before simplification ...\n");

  QMultiMap<int, int> imat;
  for(int i=0; i<m_nfaces; i++)
    {
      int a = m_flist[i]->verts[0];
      int b = m_flist[i]->verts[1];
      int c = m_flist[i]->verts[2];

      imat.insert(a, b);
      imat.insert(b, a);
      imat.insert(a, c);
      imat.insert(c, a);
      imat.insert(b, c);
      imat.insert(c, b);
    }

  QVector<Vec> V, newV;
  V.resize(m_nverts);
  newV.resize(m_nverts);

  for(int i=0; i<m_nverts; i++)
    {
      V[i] = Vec(m_vlist[i]->x, m_vlist[i]->y, m_vlist[i]->z);
    }

  for(int nt=0; nt<ntimes; nt++)
    {
      for(int i=0; i<m_nverts; i++)
	{	  
	  if (i%100 == 0)
	    {
	      m_meshProgress->setValue((int)(100.0*(float)i/(float)(m_nverts)));
	      qApp->processEvents();
	    }
	  
	  QList<int> idx = imat.values(i);
	  Vec v0 = V[i];
	  Vec v = Vec(0,0,0);
	  float sum = 0;
	  for(int j=0; j<idx.count(); j++)
	    {
	      Vec vj = V[idx[j]];
	      float ln = (v0-vj).norm();
	      if (ln > 0)
		{
		  sum += 1.0/ln;
		  v = v + vj/ln;
		}
	    }
	  if (sum > 0)
	    v0 = v0 + 0.5*(v/sum - v0);
	  newV[i] = v0;
	}
      
//      for(int i=0; i<m_nverts; i++)
//	{
//	  if (i%100 == 0)
//	    {
//	      m_meshProgress->setValue((int)(100.0*(float)i/(float)(m_nverts)));
//	      qApp->processEvents();
//	    }
//	  
//	  QList<int> idx = imat.values(i);
//	  Vec v0 = newV[i];
//	  Vec v = Vec(0,0,0);
//	  float sum = 0;
//	  for(int j=0; j<idx.count(); j++)
//	    {
//	      Vec vj = newV[idx[j]];
//	      float ln = (v0-vj).norm();
//	      if (ln > 0)
//		{
//		  sum += 1.0/ln;
//		  v = v + vj/ln;
//		}
//	    }
//	  if (sum > 0)
//	    v0 = v0 - 0.5*(v/sum - v0);
//	  V[i] = v0;
//	}

      V = newV;
    }

    for(int i=0; i<m_nverts; i++)
    {
      m_vlist[i]->x = V[i].x;
      m_vlist[i]->y = V[i].y;
      m_vlist[i]->z = V[i].z;
    }

    m_meshProgress->setValue(0);
    qApp->processEvents();
}
