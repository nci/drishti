#include <QtGui>
#include "clipinformation.h"

ClipInformation::ClipInformation() { clear(); }
ClipInformation::~ClipInformation() { clear(); }
int ClipInformation::size() { return pos.size(); }

ClipInformation::ClipInformation(const ClipInformation& ci)
{
  show = ci.show;
  pos = ci.pos;
  rot = ci.rot;
  color = ci.color;
  solidColor = ci.solidColor;
  applyFlip = ci.applyFlip;
  apply = ci.apply;
  imageName = ci.imageName;
  imageFrame = ci.imageFrame;
  captionText = ci.captionText;
  captionFont = ci.captionFont;
  captionColor = ci.captionColor;
  captionHaloColor = ci.captionHaloColor;
  opacity = ci.opacity;
  stereo = ci.stereo;
  scale1 = ci.scale1;
  scale2 = ci.scale2;
  tfSet = ci.tfSet;
  viewport = ci.viewport;
  viewportType = ci.viewportType;
  viewportScale = ci.viewportScale;
  thickness = ci.thickness;
  showSlice = ci.showSlice;
  showOtherSlice = ci.showOtherSlice;
  showThickness = ci.showThickness;
  gridX = ci.gridX;
  gridY = ci.gridY;
}

ClipInformation&
ClipInformation::operator=(const ClipInformation& ci)
{
  show = ci.show;
  pos = ci.pos;
  rot = ci.rot;
  color = ci.color;
  solidColor = ci.solidColor;
  applyFlip = ci.applyFlip;
  apply = ci.apply;
  imageName = ci.imageName;
  imageFrame = ci.imageFrame;
  captionText = ci.captionText;
  captionFont = ci.captionFont;
  captionColor = ci.captionColor;
  captionHaloColor = ci.captionHaloColor;
  opacity = ci.opacity;
  stereo = ci.stereo;
  scale1 = ci.scale1;
  scale2 = ci.scale2;
  tfSet = ci.tfSet;
  viewport = ci.viewport;
  viewportType = ci.viewportType;
  viewportScale = ci.viewportScale;
  thickness = ci.thickness;
  showSlice = ci.showSlice;
  showOtherSlice = ci.showOtherSlice;
  showThickness = ci.showThickness;
  gridX = ci.gridX;
  gridY = ci.gridY;

  return *this;
}

void
ClipInformation::clear()
{
  show.clear();
  pos.clear();
  rot.clear();
  color.clear();
  solidColor.clear();
  applyFlip.clear();
  apply.clear();
  imageName.clear();
  imageFrame.clear();
  captionText.clear();
  captionFont.clear();
  captionColor.clear();
  captionHaloColor.clear();
  opacity.clear();
  stereo.clear();
  scale1.clear();
  scale2.clear();
  tfSet.clear();
  viewport.clear();
  viewportType.clear();
  viewportScale.clear();
  thickness.clear();
  showSlice.clear();
  showOtherSlice.clear();
  showThickness.clear();
  gridX.clear();
  gridY.clear();
}

ClipInformation
ClipInformation::interpolate(const ClipInformation clipInfo1,
			     const ClipInformation clipInfo2,
			     float frc)
{
  ClipInformation clipInfo;


  clipInfo.show = clipInfo1.show;
  clipInfo.applyFlip = clipInfo1.applyFlip;
  clipInfo.apply = clipInfo1.apply;
  clipInfo.imageName = clipInfo1.imageName;
  clipInfo.captionText = clipInfo1.captionText;
  clipInfo.tfSet = clipInfo1.tfSet;
  clipInfo.solidColor = clipInfo1.solidColor;
  clipInfo.showSlice = clipInfo1.showSlice;
  clipInfo.showOtherSlice = clipInfo1.showOtherSlice;
  clipInfo.showThickness = clipInfo1.showThickness;
  clipInfo.viewportType = clipInfo1.viewportType;

  // interpolate the rest
  for(int ci=0;
      ci<qMin(clipInfo1.pos.size(),
	      clipInfo2.pos.size());
      ci++)
    {
      Vec pos;
      Quaternion rot;
      pos = clipInfo1.pos[ci] + frc*(clipInfo2.pos[ci]-
				     clipInfo1.pos[ci]);
      rot = Quaternion::slerp(clipInfo1.rot[ci], clipInfo2.rot[ci], frc);

      float st, op, scl1, scl2;
      st = clipInfo1.stereo[ci] + frc*(clipInfo2.stereo[ci]-
					clipInfo1.stereo[ci]);
      op = clipInfo1.opacity[ci] + frc*(clipInfo2.opacity[ci]-
					clipInfo1.opacity[ci]);
      scl1 = clipInfo1.scale1[ci] + frc*(clipInfo2.scale1[ci]-
					 clipInfo1.scale1[ci]);
      scl2 = clipInfo1.scale2[ci] + frc*(clipInfo2.scale2[ci]-
					 clipInfo1.scale2[ci]);
      int grdx = clipInfo1.gridX[ci] + frc*(clipInfo2.gridX[ci]-
					    clipInfo1.gridX[ci]);
      int grdy = clipInfo1.gridY[ci] + frc*(clipInfo2.gridY[ci]-
					    clipInfo1.gridY[ci]);

      int frm;
      frm = clipInfo1.imageFrame[ci] + frc*(clipInfo2.imageFrame[ci]-
					    clipInfo1.imageFrame[ci]);

      Vec pcolor;
      pcolor = clipInfo1.color[ci] + frc*(clipInfo2.color[ci]-
					  clipInfo1.color[ci]);

      QVector4D vp;
      vp = clipInfo1.viewport[ci] + frc*(clipInfo2.viewport[ci]-
					 clipInfo1.viewport[ci]);

      float vps;
      vps = clipInfo1.viewportScale[ci] + frc*(clipInfo2.viewportScale[ci]-
					       clipInfo1.viewportScale[ci]);

      int thick;
      thick = clipInfo1.thickness[ci] + frc*(clipInfo2.thickness[ci]-
					     clipInfo1.thickness[ci]);

      QFont cfont = clipInfo1.captionFont[ci];
      QColor ccolor = clipInfo1.captionColor[ci];
      QColor chcolor = clipInfo1.captionHaloColor[ci];
      if (clipInfo1.captionText[ci] == clipInfo2.captionText[ci])
	{
	  QFont cfont2 = clipInfo1.captionFont[ci];

	  if (cfont.family() == cfont2.family())
	    {
	      // interpolate pointsize
	      int pt1 = cfont.pointSize();
	      int pt2 = cfont2.pointSize();
	      int pt = (1-frc)*pt1 + frc*pt2;
	      cfont.setPointSize(pt);
	    }

	  // interpolate color
	  float r1 = ccolor.redF();
	  float g1 = ccolor.greenF();
	  float b1 = ccolor.blueF();
	  float a1 = ccolor.alphaF();	  
	  QColor c = clipInfo2.captionColor[ci];
	  float r2 = c.redF();
	  float g2 = c.greenF();
	  float b2 = c.blueF();
	  float a2 = c.alphaF();	  
	  float r = (1-frc)*r1 + frc*r2;
	  float g = (1-frc)*g1 + frc*g2;
	  float b = (1-frc)*b1 + frc*b2;
	  float a = (1-frc)*a1 + frc*a2;	  	  
	  ccolor = QColor(r*255, g*255, b*255, a*255);


	  r1 = chcolor.redF();
	  g1 = chcolor.greenF();
	  b1 = chcolor.blueF();
	  a1 = chcolor.alphaF();	  
	   c = clipInfo2.captionHaloColor[ci];
	  r2 = c.redF();
	  g2 = c.greenF();
	  b2 = c.blueF();
	  a2 = c.alphaF();	  
	  r = (1-frc)*r1 + frc*r2;
	  g = (1-frc)*g1 + frc*g2;
	  b = (1-frc)*b1 + frc*b2;
	  a = (1-frc)*a1 + frc*a2;	  	  
	  chcolor = QColor(r*255, g*255, b*255, a*255);
	}

      
      clipInfo.pos.append(pos);
      clipInfo.rot.append(rot);
      clipInfo.opacity.append(op);
      clipInfo.stereo.append(st);
      clipInfo.scale1.append(scl1);
      clipInfo.scale2.append(scl2);
      clipInfo.gridX.append(grdx);
      clipInfo.gridY.append(grdy);
      clipInfo.imageFrame.append(frm);
      clipInfo.captionFont.append(cfont);
      clipInfo.captionColor.append(ccolor);
      clipInfo.captionHaloColor.append(chcolor);
      clipInfo.color.append(pcolor);
      clipInfo.viewport.append(vp);
      clipInfo.viewportScale.append(vps);
      clipInfo.thickness.append(thick);
    }

//  if (clipInfo1.pos.size() < clipInfo2.pos.size())
//    {
//      for(int ci=clipInfo1.pos.size();
//	  ci < clipInfo2.pos.size();
//	  ci++)
//	{
//	  clipInfo.pos.append(clipInfo2.pos[ci]);
//	  clipInfo.rot.append(clipInfo2.rot[ci]);
//	}
//    }
  if (clipInfo2.pos.size() < clipInfo1.pos.size())
    {
      for(int ci=clipInfo2.pos.size();
	  ci < clipInfo1.pos.size();
	  ci++)
	{
	  clipInfo.pos.append(clipInfo1.pos[ci]);
	  clipInfo.rot.append(clipInfo1.rot[ci]);
	  clipInfo.opacity.append(clipInfo1.opacity[ci]);
	  clipInfo.stereo.append(clipInfo1.stereo[ci]);
	  clipInfo.scale1.append(clipInfo1.scale1[ci]);
	  clipInfo.scale2.append(clipInfo1.scale2[ci]);
	  clipInfo.imageFrame.append(clipInfo1.imageFrame[ci]);
	  clipInfo.captionFont.append(clipInfo1.captionFont[ci]);
	  clipInfo.captionColor.append(clipInfo1.captionColor[ci]);
	  clipInfo.captionHaloColor.append(clipInfo1.captionHaloColor[ci]);
	  clipInfo.color.append(clipInfo1.color[ci]);
	  clipInfo.viewport.append(clipInfo1.viewport[ci]);
	  clipInfo.viewportScale.append(clipInfo1.viewportScale[ci]);
	  clipInfo.thickness.append(clipInfo1.thickness[ci]);
	}
    }
  
  return clipInfo;
}


void
ClipInformation::load(fstream &fin)
{
  clear();

  bool done = false;
  char keyword[100];
  float f[10];

  int n;
  fin.read((char*)&n, sizeof(int));  

  for(int i=0; i<n; i++)
    {
      show.append(true);
      pos.append(Vec(0,0,0));
      rot.append(Quaternion());
      solidColor.append(false);
      showSlice.append(true);
      showOtherSlice.append(true);
      showThickness.append(true);
      color.append(Vec(0.8,0.7,0.9));
      applyFlip.append(false);
      apply.append(true);
      imageName.append(QString());
      imageFrame.append(0);
      captionText.append(QString());
      captionFont.append(QFont("Helvetica", 48));
      captionColor.append(Qt::white);
      captionHaloColor.append(Qt::white);
      opacity.append(0.3f);
      stereo.append(1.0f);
      scale1.append(1);
      scale2.append(1);
      tfSet.append(-1); // i.e. do not texture clipplane
      viewport.append(QVector4D(-1,-1,-1,-1));
      viewportType.append(true);
      viewportScale.append(1.0f);
      thickness.append(0);
      gridX.append(0);
      gridY.append(0);
    }


  while(!done)
    { 
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "position") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));	      
	      pos[i] = Vec(f[0],f[1],f[2]);
	    }
	}
      else if (strcmp(keyword, "rotation") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      Vec axis;
	      float angle;
	      fin.read((char*)&f, 3*sizeof(float));	      
	      axis = Vec(f[0],f[1],f[2]);
	      fin.read((char*)&angle, sizeof(float));
	      Quaternion q(axis, angle);
	      rot[i] = q;
	    }
	}
      else if (strcmp(keyword, "show") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      show[i]=b;
	    }
	}
      else if (strcmp(keyword, "flip") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      applyFlip[i]=b;
	    }
	}
      else if (strcmp(keyword, "apply") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      apply[i]=b;
	    }
	}
      else if (strcmp(keyword, "imagename") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      int len;
	      fin.read((char*)&len, sizeof(int));
	      if (len > 0)
		{
		  char *str = new char[len];
		  fin.read((char*)str, len*sizeof(char));
		  imageName[i] = QString(str);
		  delete [] str;
		}
	    }
	}
      else if (strcmp(keyword, "imageframe") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      int frm;
	      fin.read((char*)&frm, sizeof(int));
	      imageFrame[i]=frm;
	    }
	}
      else if (strcmp(keyword, "captiontext") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      int len;
	      fin.read((char*)&len, sizeof(int));
	      if (len > 0)
		{
		  char *str = new char[len];
		  fin.read((char*)str, len*sizeof(char));
		  captionText[i] = QString(str);
		  delete [] str;
		}
	    }
	}
      else if (strcmp(keyword, "captionfont") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      int len;
	      fin.read((char*)&len, sizeof(int));
	      if (len > 0)
		{
		  char *str = new char[len];
		  fin.read((char*)str, len*sizeof(char));
		  QString fontStr = QString(str);
		  captionFont[i].fromString(fontStr); 
		  delete [] str;
		}
	    }
	}
      else if (strcmp(keyword, "captioncolor") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      unsigned char r, g, b, a;
	      fin.read((char*)&r, sizeof(unsigned char));
	      fin.read((char*)&g, sizeof(unsigned char));
	      fin.read((char*)&b, sizeof(unsigned char));
	      fin.read((char*)&a, sizeof(unsigned char));
	      captionColor[i] = QColor(r,g,b,a);
	    }
	}
      else if (strcmp(keyword, "captionhalocolor") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      unsigned char r, g, b, a;
	      fin.read((char*)&r, sizeof(unsigned char));
	      fin.read((char*)&g, sizeof(unsigned char));
	      fin.read((char*)&b, sizeof(unsigned char));
	      fin.read((char*)&a, sizeof(unsigned char));
	      captionHaloColor[i] = QColor(r,g,b,a);
	    }
	}
      else if (strcmp(keyword, "opacity") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      float op;
	      fin.read((char*)&op, sizeof(float));
	      opacity[i]=op;
	    }
	}
      else if (strcmp(keyword, "stereo") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      float op;
	      fin.read((char*)&op, sizeof(float));
	      stereo[i]=op;
	    }
	}
      else if (strcmp(keyword, "solidcolor") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      solidColor[i]=b;
	    }
	}
      else if (strcmp(keyword, "color") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));	      
	      color[i] = Vec(f[0],f[1],f[2]);
	    }
	}
      else if (strcmp(keyword, "scale1") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      float sc;
	      fin.read((char*)&sc, sizeof(float));
	      scale1[i]=sc;
	    }
	}
      else if (strcmp(keyword, "scale2") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      float sc;
	      fin.read((char*)&sc, sizeof(float));
	      scale2[i]=sc;
	    }
	}
      else if (strcmp(keyword, "tfset") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      int tf;
	      fin.read((char*)&tf, sizeof(int));
	      tfSet[i]=tf;
	    }
	}
      else if (strcmp(keyword, "gridx") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      float sc;
	      fin.read((char*)&sc, sizeof(int));
	      gridX[i]=sc;
	    }
	}
      else if (strcmp(keyword, "gridy") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      float sc;
	      fin.read((char*)&sc, sizeof(int));
	      gridY[i]=sc;
	    }
	}
      else if (strcmp(keyword, "viewport") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      fin.read((char*)&f, 4*sizeof(float));	      
	      viewport[i] = QVector4D(f[0],f[1],f[2],f[3]);
	    }
	}
      else if (strcmp(keyword, "viewporttype") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      viewportType[i]=b;
	    }
	}
      else if (strcmp(keyword, "viewportscale") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      fin.read((char*)&f, sizeof(float));	      
	      viewportScale[i] = f[0];
	    }
	}
      else if (strcmp(keyword, "thickness") == 0)
	{
	  int tk;
	  for(int i=0; i<n; i++)
	    {
	      fin.read((char*)&tk, sizeof(int));	      
	      thickness[i] = qBound(0, tk, 200);
	    }
	}
      else if (strcmp(keyword, "showslice") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      showSlice[i]=b;
	    }
	}
      else if (strcmp(keyword, "showotherslice") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      showOtherSlice[i]=b;
	    }
	}
      else if (strcmp(keyword, "showthickness") == 0)
	{
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      showThickness[i]=b;
	    }
	}
    }

}


void
ClipInformation::save(fstream &fout)
{
  char keyword[100];
  float f[10];

  memset(keyword, 0, 100);
  sprintf(keyword, "clipinformation");
  fout.write((char*)keyword, strlen(keyword)+1);

  int nclip = pos.size();
  fout.write((char*)&nclip, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      f[0] = pos[i].x;
      f[1] = pos[i].y;
      f[2] = pos[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }
  
  memset(keyword, 0, 100);
  sprintf(keyword, "rotation");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      Vec axis;
      float angle;
      rot[i].getAxisAngle(axis, angle);
      f[0] = axis.x;
      f[1] = axis.y;
      f[2] = axis.z;
      fout.write((char*)&f, 3*sizeof(float));
      fout.write((char*)&angle, sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "show");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&show[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "flip");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&applyFlip[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "apply");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&apply[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "imagename");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      int len = imageName[i].size()+1;
      fout.write((char*)&len, sizeof(int));
      if (len > 0)
	fout.write((char*)imageName[i].toAscii().data(), len*sizeof(char));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "imageframe");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&imageFrame[i], sizeof(int));
  

  memset(keyword, 0, 100);
  sprintf(keyword, "captiontext");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      int len = captionText[i].size()+1;
      fout.write((char*)&len, sizeof(int));
      if (len > 0)
	fout.write((char*)captionText[i].toAscii().data(), len*sizeof(char));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "captionfont");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      QString fontStr = captionFont[i].toString();
      int len = fontStr.size()+1;
      fout.write((char*)&len, sizeof(int));
      if (len > 0)	
	fout.write((char*)fontStr.toAscii().data(), len*sizeof(char));
    }
  
  memset(keyword, 0, 100);
  sprintf(keyword, "captioncolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      unsigned char r = captionColor[i].red();
      unsigned char g = captionColor[i].green();
      unsigned char b = captionColor[i].blue();
      unsigned char a = captionColor[i].alpha();
      fout.write((char*)&r, sizeof(unsigned char));
      fout.write((char*)&g, sizeof(unsigned char));
      fout.write((char*)&b, sizeof(unsigned char));
      fout.write((char*)&a, sizeof(unsigned char));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "captionhalocolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      unsigned char r = captionHaloColor[i].red();
      unsigned char g = captionHaloColor[i].green();
      unsigned char b = captionHaloColor[i].blue();
      unsigned char a = captionHaloColor[i].alpha();
      fout.write((char*)&r, sizeof(unsigned char));
      fout.write((char*)&g, sizeof(unsigned char));
      fout.write((char*)&b, sizeof(unsigned char));
      fout.write((char*)&a, sizeof(unsigned char));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&opacity[i], sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "solidcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&solidColor[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      f[0] = color[i].x;
      f[1] = color[i].y;
      f[2] = color[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "scale1");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&scale1[i], sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "scale2");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&scale2[i], sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "tfset");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&tfSet[i], sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "gridx");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&gridX[i], sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "gridy");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&gridY[i], sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "viewport");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    {
      f[0] = viewport[i].x();
      f[1] = viewport[i].y();
      f[2] = viewport[i].z();
      f[3] = viewport[i].w();
      fout.write((char*)&f, 4*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "viewporttype");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&viewportType[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "viewportscale");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&viewportScale[i], sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "thickness");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&thickness[i], sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "showslice");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&showSlice[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showotherslice");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&showOtherSlice[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showthickness");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&showThickness[i], sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "stereo");
  fout.write((char*)keyword, strlen(keyword)+1);
  for(int i=0; i<nclip; i++)
    fout.write((char*)&stereo[i], sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}
