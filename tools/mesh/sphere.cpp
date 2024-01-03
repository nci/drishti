void icosahedron()
{
  float X = 0.525731112119133606;
  float Z = 0.850650808352039932;
  QList<Vec> vdata;
  vdata << Vec(-X, 0.0, Z);
  vdata << Vec(X, 0.0, Z);
  vdata << Vec(-X, 0.0, -Z);
  vdata << Vec(X, 0.0, -Z);
  vdata << Vec(0.0, Z, X);
  vdata << Vec(0.0, Z, -X);
  vdata << Vec(0.0, -Z, X);
  vdata << Vec(0.0, -Z, -X);
  vdata << Vec(Z, X, 0.0);
  vdata << Vec(-Z, X, 0.0);
  vdata << Vec(Z, -X, 0.0);
  vdata << Vec(-Z, -X, 0.0);

  int tindices[60] = { 0,4,1,
		       0,9,4,
		       9,5,4,
		       4,5,8,
		       4,8,1,
		       8,10,1,
		       8,3,10,
		       5,3,8,
		       5,2,3,
		       2,7,3,
		       7,10,3,
		       7,6,10,
		       7,11,6,
		       11,0,6,
		       0,1,6,
		       6,1,10,
		       9,0,11,
		       9,11,2,
		       9,2,5,
		       7,2,11 };
}

void
subdivide(Vec v1, Vec v2, Vec v3, int depth)
{
  Vec v12, v23, v31;

  if (depth == 0)
    {
      glNormal3fv(v1); glVertex3fv(v1);
      glNormal3fv(v2); glVertex3fv(v2);
      glNormal3fv(v3); glVertex3fv(v3);
      return;
    }

  for(int i=0; i<3; i++)
    {
      v12 = v1 + v2;
      v23 = v2 + v3;
      v31 = v3 + v1;
    }
  v12.normalize();
  v23.normalize();
  v31.normalize();

  subdivide(v1, v12, v31, depth-1);
  subdivide(v2, v23, v12, depth-1);
  subdivide(v2, v31, v23, depth-1);
  subdivide(v12, v23, v31, depth-1);
}

void
sphere(int depth)
{
  glBegin(GL_TRIANGLES);
  for(int i=0; i<20; i++)
    {
      subdivide(vdata[tindices[3*i]],
		vdata[tindices[3*i+1]],
		vdata[tindices[3*i+2]],
		depth);
    }
  glEnd();
}
