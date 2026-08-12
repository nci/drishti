#ifndef CPUMESHPAINT_H
#define CPUMESHPAINT_H

#include <cmath>

namespace CpuMeshPaint
{
  inline float fract(float value)
  {
    return value - std::floor(value);
  }

  inline float mix(float a, float b, float amount)
  {
    return a + amount*(b-a);
  }

  inline float smoothstep(float edge0, float edge1, float value)
  {
    if (edge1 <= edge0)
      return value < edge0 ? 0.0f : 1.0f;

    float amount = (value-edge0)/(edge1-edge0);
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    return amount*amount*(3.0f-2.0f*amount);
  }

  inline float mod289(float value)
  {
    return value-std::floor(value/289.0f)*289.0f;
  }

  inline float permute(float value)
  {
    return mod289(((value*34.0f)+1.0f)*value);
  }

  // This is the scalar form of the value-noise function in paintShader.glsl.
  inline float noise(float x, float y, float z)
  {
    const float ax = std::floor(x);
    const float ay = std::floor(y);
    const float az = std::floor(z);

    float dx = x-ax;
    float dy = y-ay;
    float dz = z-az;
    dx = dx*dx*(3.0f-2.0f*dx);
    dy = dy*dy*(3.0f-2.0f*dy);
    dz = dz*dz*(3.0f-2.0f*dz);

    const float bx[4] = {ax, ax+1.0f, ay, ay+1.0f};
    const float k1[4] = {permute(bx[0]), permute(bx[1]),
                         permute(bx[0]), permute(bx[1])};
    const float k2[4] = {permute(k1[0]+bx[2]), permute(k1[1]+bx[2]),
                         permute(k1[2]+bx[3]), permute(k1[3]+bx[3])};

    float o1[4];
    float o2[4];
    for (int i=0; i<4; ++i)
      {
        o1[i] = fract(permute(k2[i]+az)/41.0f);
        o2[i] = fract(permute(k2[i]+az+1.0f)/41.0f);
      }

    float o3[4];
    for (int i=0; i<4; ++i)
      o3[i] = mix(o1[i], o2[i], dz);

    const float o4x = mix(o3[0], o3[1], dx);
    const float o4y = mix(o3[2], o3[3], dx);
    return mix(o4x, o4y, dy);
  }

  inline float fbm(float x, float y, float z, int octaves)
  {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i=0; i<octaves; ++i)
      {
        value += amplitude*noise(x, y, z);
        x *= 2.0f;
        y *= 2.0f;
        z *= 2.0f;
        amplitude *= 0.5f;
      }
    return value;
  }

  inline float ridge(float value, float offset)
  {
    value = offset-std::fabs(value);
    return value*value;
  }

  inline float ridged(float x, float y, float z)
  {
    float sum = 0.0f;
    float frequency = 1.0f;
    float amplitude = 0.5f;
    float previous = 1.0f;
    for (int i=0; i<6; ++i)
      {
        const float signedNoise = 2.0f*noise(x*frequency,
                                             y*frequency,
                                             z*frequency)-1.0f;
        const float value = ridge(signedNoise, 0.9f);
        sum += value*amplitude;
        sum += value*amplitude*previous;
        previous = value;
        frequency *= 2.0f;
        amplitude *= 0.5f;
      }
    return sum;
  }

  inline float edge(float value, float center, float edge0, float edge1)
  {
    return 1.0f-smoothstep(edge0, edge1, std::fabs(value-center));
  }

  inline float marble(float x, float y, float z)
  {
    const float v0 = edge(fbm(x*18.0f, y*18.0f, z*18.0f, 6),
                          0.5f, 0.0f, 0.2f);
    const float v1 = smoothstep(0.5f, 0.51f,
                                fbm(x*14.0f, y*14.0f, z*14.0f, 6));
    const float v2 = edge(fbm(x*14.0f, y*14.0f, z*14.0f, 6),
                          0.5f, 0.0f, 0.05f);
    const float v3 = edge(fbm(x*14.0f, y*14.0f, z*14.0f, 6),
                          0.5f, 0.0f, 0.25f);

    float value = 1.0f;
    value -= v0*0.75f;
    value = mix(value, 0.97f, v1);
    value = mix(value, 0.51f, v2);
    value -= v3*0.2f;
    return 1.0f-value;
  }

  inline float roughness(float x, float y, float z,
                         int blendOctave, int roughnessType)
  {
    if (roughnessType == 0)
      {
        const float scale = static_cast<float>(blendOctave*blendOctave);
        return ridged(scale*x, scale*y, scale*z);
      }
    if (roughnessType == 1)
      {
        const float scale = 1.0f+0.1f*blendOctave;
        return marble(scale*x, scale*y, scale*z);
      }
    if (roughnessType == 2)
      {
        const float scale = std::pow(2.0f, static_cast<float>(blendOctave));
        const float value = fbm(scale*x, scale*y, scale*z, 6);
        return value*value;
      }
    return 1.0f;
  }

  inline bool apply(float px, float py, float pz,
                    float hitX, float hitY, float hitZ,
                    float radius,
                    float hitR, float hitG, float hitB,
                    int blendType, float blendFraction,
                    int blendOctave, int roughnessType,
                    float boxX, float boxY, float boxZ, float boxLength,
                    float origR, float origG, float origB,
                    float &red, float &green, float &blue)
  {
    if (!(radius > 0.0f))
      return false;

    const float dx = px-hitX;
    const float dy = py-hitY;
    const float dz = pz-hitZ;
    const float distanceSquared = dx*dx+dy*dy+dz*dz;
    const float radiusSquared = radius*radius;
    if (distanceSquared >= radiusSquared)
      return false;

    const float inverseBoxLength = boxLength > 0.000001f ? 1.0f/boxLength : 0.0f;
    const float texture = roughness((px-boxX)*inverseBoxLength,
                                    (py-boxY)*inverseBoxLength,
                                    (pz-boxZ)*inverseBoxLength,
                                    blendOctave, roughnessType);
    const float distance = std::sqrt(distanceSquared)/radius;
    const bool erase = hitR*hitR+hitG*hitG+hitB*hitB < 0.001f;

    if (blendType > 9)
      {
        if (erase)
          {
            red = origR;
            green = origG;
            blue = origB;
          }
        else
          {
            const float decay = (1.0f-smoothstep(0.0f, 1.0f, distance))*
                                blendFraction*texture;
            red = mix(red, origR, decay);
            green = mix(green, origG, decay);
            blue = mix(blue, origB, decay);
          }
        return true;
      }

    if (blendType != 0)
      return false;

    if (erase)
      {
        red = 0.0f;
        green = 0.0f;
        blue = 0.0f;
      }
    else
      {
        const float decay = (1.0f-smoothstep(0.0f, 1.0f, distance))*
                            blendFraction*texture;
        red = mix(red, hitR, decay);
        green = mix(green, hitG, decay);
        blue = mix(blue, hitB, decay);
      }
    return true;
  }
}

#endif
