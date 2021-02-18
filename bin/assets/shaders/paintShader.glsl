#version 430 core

layout (location = 1) uniform vec3 hitPt;
layout (location = 2) uniform float radius;
layout (location = 3) uniform vec3 hitColor;
layout (location = 4) uniform int blendType;

  
layout (binding = 0) buffer vdata
{
  float v[];  // pos3+norm3+color3
};

layout (binding = 1) buffer cdata
{
  float origColor[];  // original color array
};

layout (local_size_x = 128) in;

void main()
{
  uint index = gl_GlobalInvocationID.x;

  uint posidx = 9*index+0; // pos index
  uint coloridx = 9*index+6; // color index

  vec3 pos = vec3(v[posidx+0],v[posidx+1],v[posidx+2]);

  float dst = length(pos-hitPt)/radius;
  if ( dst < 1.0)
    {
      if (hitColor.x < 1.1)
	{
	  if (blendType == 0)
	    {
	      float decay = 1.0 - smoothstep(0, 1, dst);
	      decay *= 0.05;
	      v[coloridx+0] = mix(v[coloridx+0], hitColor.x, decay);
	      v[coloridx+1] = mix(v[coloridx+1], hitColor.y, decay);
	      v[coloridx+2] = mix(v[coloridx+2], hitColor.z, decay);
	    }
	  else
	    {
	      v[coloridx+0] = hitColor.x;
	      v[coloridx+1] = hitColor.y;
	      v[coloridx+2] = hitColor.z;
	    }
	  // darken
	  // float decay = smoothstep(0, 1, dst);
	  // decay = 0.99 + 0.01*decay;
	  // v[coloridx+0] *= decay;
	  // v[coloridx+1] *= decay;
	  // v[coloridx+2] *= decay;
	}
      else
	{
	  v[coloridx+0] = origColor[3*index+0];
	  v[coloridx+1] = origColor[3*index+1];
	  v[coloridx+2] = origColor[3*index+2];
	}
    }
}
