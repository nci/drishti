#version 420 core
uniform sampler2D diffuseTex;
uniform vec3 viewDir;
uniform vec4 extras;
uniform float sceneRadius;
uniform float roughness;
uniform float ambient;
uniform float diffuse;
uniform float specular;
uniform int nclip;
uniform vec3 clipPos[10];
uniform vec3 clipNormal[10];
uniform bool hasUV;
uniform float featherSize;
uniform vec3 hatchPattern;
uniform sampler3D solidTex;
uniform float revealTransparency;

in vec3 v3Color;
in vec3 v3Normal;
in vec3 pointPos;
in float zdepth;
in float surfId;
in vec3 oPosition;
in float zdepthLinear;

// Ouput data
layout (location=0) out vec4 outputColor;
layout (location=1) out vec3 alpha;
  
layout (depth_greater) out float gl_FragDepth;

void main()
{
  gl_FragDepth = zdepth;
  

  alpha = vec3(1, 1, zdepthLinear/sceneRadius);

  // transparent reveal => edges are more opaque
  float NdotV = abs(dot(v3Normal,viewDir));
  
  float decay = 1.0 - clamp(NdotV/(1.0-min(extras.y, 0.999)), 0.0, 1.0);
  decay = mix(decay, 0.0, step(0.95, extras.y));
  alpha.x = max(revealTransparency, sqrt(decay));
  
  
  if (hasUV)
     outputColor = texture(diffuseTex, vec2(v3Color.x, 1-v3Color.y));
  else
     outputColor = vec4(v3Color, 1.0);

  vec3 defCol = mix(vec3(0.0), outputColor.rgb, NdotV*NdotV);
  outputColor.rgb = mix(outputColor.rgb, vec3(0.0), pow(decay, 0.5/revealTransparency));
  outputColor.rgb = mix(outputColor.rgb, defCol, step(0.95, extras.y));
    
  float cfeather = 1.0;
  if (nclip > 0)
    {
      for(int c=0; c<nclip; c++)
        {
          vec3 cpos = clipPos[c];
          vec3 cnorm = clipNormal[c];
          float cp = dot(pointPos-cpos, cnorm);
          if (cp > 0.0)
            discard;
          else
            cfeather *= smoothstep(0.0, featherSize, -cp);
        }
      cfeather = 1.0 - cfeather;
    }

    if (nclip > 0)
      outputColor.rgb = mix(outputColor.rgb, vec3(1,1,1), cfeather);


  vec3 glowColor = outputColor.rgb;

  //---------------------------------------------------------
  //---------------------------------------------------------
  // apply solid texture
  float a = 1.0/128.0;
  vec3 solidTexCoord = a/2 + (1.0-a)*fract(hatchPattern.y*(abs(oPosition)/vec3(sceneRadius)));

  vec3 solidColor = texture(solidTex, solidTexCoord).rgb;
  outputColor.rgb = mix(outputColor.rgb, solidColor, hatchPattern.z*step(0.05, hatchPattern.z));

  float patType = abs(hatchPattern.z);
  float solidLum = (0.2*solidColor.r+0.7*solidColor.g+0.1*solidColor.b);
  outputColor.rgb = mix(outputColor.rgb,
  		        outputColor.rgb*solidLum,
                        patType*step(0.05, -hatchPattern.z)*step(-hatchPattern.z, 1.05));
  outputColor.rgb = mix(outputColor.rgb,
  		        outputColor.rgb*(1.0-solidLum),
			(patType-1.0)*step(1.0, -hatchPattern.z)*step(-hatchPattern.z, 2.05));
  
  //---------------------------------------------------------
  //---------------------------------------------------------
  

  // glow when active or has glow switched on
  outputColor.rgb += 0.5*max(extras.z,extras.x)*glowColor.rgb;

  // desaturate if required
  outputColor.rgb *= extras.w;


  //=======================================  
//  vec3 Amb = ambient*outputColor.rgb;
//  float diffMag = abs(dot(v3Normal, viewDir));
//  vec3 Diff = diffuse*diffMag*outputColor.rgb;
//  outputColor.rgb = Amb + Diff;

  vec3 Amb = ambient * outputColor.rgb;
  float diffMag = abs(dot(v3Normal, viewDir));
  vec3 Diff = diffuse * diffMag*outputColor.rgb;
  outputColor.rgb = Amb + Diff;

//  vec3 reflecvec = reflect(viewDir, v3Normal);
//  float Spec = pow(abs(dot(v3Normal, reflecvec)), 256.0);
//  outputColor.rgb += 0.1*specular*vec3(Spec);
  //=======================================  

  // more glow for higher transparency
  //outputColor.rgb += 0.3*(1.0-alpha.x)*outputColor.rgb;
  outputColor.rgb += vec3(0.2+0.3*extras.y)*outputColor.rgb;

  //=======================================  
  float w = 3.0-zdepthLinear/sceneRadius;
  outputColor = vec4(outputColor.rgb, 1.0)*alpha.x*pow(w, 2.0);
  //=======================================  

}
