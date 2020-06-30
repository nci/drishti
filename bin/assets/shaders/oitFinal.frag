#version 420 core

// Output data
layout(location=0) out vec4 color;

uniform sampler2DRect oitTex;
uniform sampler2DRect alphaTex;
uniform float roughness;
uniform float specular;

//---------------------------
vec3 shadingSpecularGGX(vec3 N, vec3 V, vec3 L, float roughness, vec3 F0)
{
    // see http://www.filmicworlds.com/2014/04/21/optimizing-ggx-shaders-with-dotlh/
    vec3 H = normalize(V + L);

    float dotLH = max(dot(L, H), 0.0);
    float dotNH = max(dot(N, H), 0.0);
    float dotNL = max(dot(N, L), 0.0);
    float dotNV = max(dot(N, V), 0.0);

    float alpha = roughness * roughness;

    // D (GGX normal distribution)
    float alphaSqr = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
    float D = alphaSqr / (denom * denom);
    // no pi because BRDF -> lighting

    // F (Fresnel term)
    float F_a = 1.0;
    float F_b = pow(1.0 - dotLH, 5); // manually?
    vec3 F = mix(vec3(F_b), vec3(F_a), F0);

    // G (remapped hotness, see Unreal Shading)
    float k = (alpha + 2 * roughness + 1) / 8.0;
    float G = dotNL / (mix(dotNL, 1, k) * mix(dotNV, 1, k));

    return D * F * G / 4.0;
}
//---------------------------

void main()
{
  vec4 oit = texture2DRect(oitTex, gl_FragCoord.xy);

  if (oit.a < 0.01)
     discard;

  float alpha = texture2DRect(alphaTex, gl_FragCoord.xy).x;
  color = vec4(oit.rgb/max(oit.a,0.0001), 1.0)*(1.0-alpha);

//
//  vec2 spos = gl_FragCoord.xy;
//
//  float dx = 0.0;
//  float dy = 0.0;
//  for(int i=0; i<10; i++)
//    {
//      float offset = 1-(1+i)%3;
//      dx += texture2DRect(alphaTex, spos.xy+vec2(1+i,offset)).z -
//            texture2DRect(alphaTex, spos.xy-vec2(1+i,offset)).z;
//      dy += texture2DRect(alphaTex, spos.xy+vec2(offset,1+i)).z -
//            texture2DRect(alphaTex, spos.xy-vec2(offset,1+i)).z;
//    }  
//  vec3 N = normalize(vec3(dx*50, dy*50, roughness));
//  vec3 spec = shadingSpecularGGX(N, vec3(0,0,1),  vec3(0,0,1), roughness*0.2, color.rgb);
//  color.rgb += 0.5*specular*spec;
//
//  color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
}
