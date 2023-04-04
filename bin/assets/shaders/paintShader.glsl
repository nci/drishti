#version 430 core

layout (location = 1) uniform vec3 hitPt;
layout (location = 2) uniform float radius;
layout (location = 3) uniform vec3 hitColor;
layout (location = 4) uniform int blendType;
layout (location = 5) uniform float blendFraction;
layout (location = 6) uniform int blendOctave;
layout (location = 7) uniform vec3 bmin;
layout (location = 8) uniform float blen;
layout (location = 9) uniform int roughnessType;

  
layout (binding = 0) buffer vdata
{
  float v[];  // pos3+norm3+color3
};

layout (binding = 1) buffer cdata
{
  float origColor[];  // original color array
};

layout (local_size_x = 128) in;


//-----------------------
// Some useful functions
float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}

vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }
vec4 permute(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

// simplex noise
float snoise(vec2 v)
{     
  // Precompute values for skewed triangular grid
  const vec4 C = vec4(0.211324865405187,
		      // (3.0-sqrt(3.0))/6.0
		      0.366025403784439,
		      // 0.5*(sqrt(3.0)-1.0)
		      -0.577350269189626,
		      // -1.0 + 2.0 * C.x
		      0.024390243902439);
                      // 1.0 / 41.0

  // First corner (x0)
  vec2 i  = floor(v + dot(v, C.yy));
  vec2 x0 = v - i + dot(i, C.xx);
  
  // Other two corners (x1, x2)
  vec2 i1 = vec2(0.0);
  i1 = (x0.x > x0.y)? vec2(1.0, 0.0):vec2(0.0, 1.0);
  vec2 x1 = x0.xy + C.xx - i1;
  vec2 x2 = x0.xy + C.zz;

  // Do some permutations to avoid
  // truncation effects in permutation
  i = mod289(i);
  vec3 p = permute(
		   permute( i.y + vec3(0.0, i1.y, 1.0))
		   + i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(
			  dot(x0,x0),
			  dot(x1,x1),
			  dot(x2,x2)
			  ), 0.0);

  m = m*m ;
  m = m*m ;

  // Gradients:
  //  41 pts uniformly over a line, mapped onto a diamond
  //  The ring size 17*17 = 289 is close to a multiple
  //      of 41 (41*7 = 287)
  
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

  // Normalise gradients implicitly by scaling m
  // Approximation of: m *= inversesqrt(a0*a0 + h*h);
  m *= 1.79284291400159 - 0.85373472095314 * (a0*a0+h*h);

  // Compute final noise value at P
  vec3 g = vec3(0.0);
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * vec2(x1.x,x2.x) + h.yz * vec2(x1.y,x2.y);
  return 130.0 * dot(m, g);
}

float ridge(float h, float offset)
{
    h = abs(h);     // create creases
    h = offset - h; // invert so creases are at top
    h = h * h;      // sharpen creases
    return h;
}



vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
  {
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
  }

//-----------------------

float random (in vec2 st) {
  return fract(sin(dot(st.xy,
		       vec2(12.9898,78.233)))*
	       43758.5453123);
}

float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = permute(b.xyxy);
    vec4 k2 = permute(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = permute(c);
    vec4 k4 = permute(c + 1.0);

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

float ridgedMF(vec3 p, int OCTAVES)
{
    float lacunarity = 2.0;
    float gain = 0.5;
    float offset = 0.9;

    float sum = 0.0;
    float freq = 1.0;
    float amp = 0.5;
    float prev = 1.0;
    for(int i=0; i < OCTAVES; i++) {      
      float n = ridge(snoise((p)*freq), offset);
      sum += n*amp;
      sum += n*amp*prev;  // scale by previous octave
      prev = n;
      freq *= lacunarity;
      amp *= gain;
    }
    return sum;
}


float edge(float v, float center, float edge0, float edge1) {
    return 1.0 - smoothstep(edge0, edge1, abs(v - center));
}

float marble(vec3 p, int OCTAVES)
{
  float v0 = edge(fbm(p * 18.0, OCTAVES), 0.5, 0.0, 0.2);
  float v1 = smoothstep(0.5, 0.51, fbm(p * 14.0, OCTAVES));
  float v2 = edge(fbm(p * 14.0, OCTAVES), 0.5, 0.0, 0.05);
  float v3 = edge(fbm(p * 14.0, OCTAVES), 0.5, 0.0, 0.25);
  
  float value = 1.0;
  value -= v0 * 0.75;
  value = mix(value, 0.97, v1);
  value = mix(value, 0.51, v2);
  value -= v3 * 0.2;

  return 1.0-value;
}

void main()
{
  uint index = gl_GlobalInvocationID.x;

  uint posidx = 9*index+0; // pos index
  uint coloridx = 9*index+6; // color index

  vec3 pos = vec3(v[posidx+0],v[posidx+1],v[posidx+2]);


  //------
  vec3 p = (pos-bmin)/blen;
  float roughness = 1.0;
  if (roughnessType == 0)
    roughness = ridgedMF(pow(blendOctave,2.0)*p, 6);
  else if (roughnessType == 1)
    roughness = marble(p*(1.0+0.1*blendOctave), 6);
  else if (roughnessType == 2)
    {
      roughness = fbm(pow(2.0,blendOctave)*p, 6);
      roughness *= roughness;
    }
  //------

  
  float dst = length(pos-hitPt)/radius;
  if ( dst < 1.0)
    {
      if (blendType > 9)
	{  // restore original color
	  if (dot(hitColor,hitColor) < 0.001)
	    {
	      v[coloridx+0] = origColor[3*index+0];
	      v[coloridx+1] = origColor[3*index+1];	
	      v[coloridx+2] = origColor[3*index+2];
	    }
	  else // fade in original color
	    {
	      float decay = 1.0 - smoothstep(0, 1, dst);
	      decay *= blendFraction * roughness;
	      v[coloridx+0] = mix(v[coloridx+0], origColor[3*index+0], decay);
	      v[coloridx+1] = mix(v[coloridx+1], origColor[3*index+1], decay);
	      v[coloridx+2] = mix(v[coloridx+2], origColor[3*index+2], decay);
	    }
	}
      else if (blendType == 0)
	{
	  if (dot(hitColor,hitColor) < 0.001)
	    {
	      v[coloridx+0] = 0;
	      v[coloridx+1] = 0;	
	      v[coloridx+2] = 0;
	    }
	  else // fade in new color
	    {
	      float decay = 1.0 - smoothstep(0, 1, dst);
	      decay *= blendFraction * roughness;
	      v[coloridx+0] = mix(v[coloridx+0], hitColor.x, decay);
	      v[coloridx+1] = mix(v[coloridx+1], hitColor.y, decay);
	      v[coloridx+2] = mix(v[coloridx+2], hitColor.z, decay);
	    }
	}
//      else if (blendType == 1)
//	{
//	  // darken
//	  float decay = smoothstep(0, 1, dst);
//	  decay = 0.99 + 0.01*decay;
//	  v[coloridx+0] *= decay;
//	  v[coloridx+1] *= decay;
//	  v[coloridx+2] *= decay;
//	}
    }
}
