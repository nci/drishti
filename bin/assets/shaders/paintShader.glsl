#version 430 core

layout (location = 1) uniform vec3 hitPt;
layout (location = 2) uniform float radius;
layout (location = 3) uniform vec3 hitColor;
layout (location = 4) uniform int blendType;
layout (location = 5) uniform float blendFraction;
layout (location = 6) uniform int blendOctave;
layout (location = 7) uniform vec3 bmin;
layout (location = 8) uniform float blen;

  
layout (binding = 0) buffer vdata
{
  float v[];  // pos3+norm3+color3
};

layout (binding = 1) buffer cdata
{
  float origColor[];  // original color array
};

layout (local_size_x = 128) in;


//----------------------
//----------------------
// Author @patriciogv - 2015
// http://patriciogonzalezvivo.com

float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float noise (in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

float fbm(in vec2 st, int OCTAVES) {
    float value = 0.0;
    float amplitude = .5;
    for (int i = 0; i < OCTAVES; i++) {
      value += amplitude * noise(st);
      st *= 2.;
      amplitude *= .5;
    }
    return value;
}
float fbm(vec3 st, int OCTAVES) {
    float value = 0.0;
    float amplitude = .5;
    for (int i = 0; i < OCTAVES; i++) {
      value += amplitude * noise(st);
      st *= 2.;
      amplitude *= .5;
    }
    return value;
}

void main()
{
  uint index = gl_GlobalInvocationID.x;

  uint posidx = 9*index+0; // pos index
  uint coloridx = 9*index+6; // color index

  vec3 pos = vec3(v[posidx+0],v[posidx+1],v[posidx+2]);


  //------
  float roughness = fbm(pow(2.0,blendOctave)*(pos-bmin)/blen, blendOctave);
  //------

  
  float dst = length(pos-hitPt)/radius;
  if ( dst < 1.0)
    {
      if (hitColor.x < 1.1)
	{
	  if (blendType == 0)
	    { // fade in new color
	      float decay = 1.0 - smoothstep(0, 1, dst);
	      decay *= blendFraction * roughness;
	      v[coloridx+0] = mix(v[coloridx+0], hitColor.x, decay);
	      v[coloridx+1] = mix(v[coloridx+1], hitColor.y, decay);
	      v[coloridx+2] = mix(v[coloridx+2], hitColor.z, decay);
	    }
	  // darken
	  // float decay = smoothstep(0, 1, dst);
	  // decay = 0.99 + 0.01*decay;
	  // v[coloridx+0] *= decay;
	  // v[coloridx+1] *= decay;
	  // v[coloridx+2] *= decay;
	}
      else // restore original color
	{
	  if (blendType == 0)
	    { // fade in original color
	      float decay = 1.0 - smoothstep(0, 1, dst);
	      decay *= blendFraction * roughness;
	      v[coloridx+0] = mix(v[coloridx+0], origColor[3*index+0], decay);
	      v[coloridx+1] = mix(v[coloridx+1], origColor[3*index+1], decay);
	      v[coloridx+2] = mix(v[coloridx+2], origColor[3*index+2], decay);
	    }
	}
    }
}
