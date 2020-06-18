#version 420 core

uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 localXform;
uniform float idx;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec3 colorIn;

out vec3 v3Normal;
out vec3 v3Color;
out vec3 pointPos;
out float zdepth;
out float surfId;
out vec3 oPosition;
out float zdepthLinear;

void main()
{
   oPosition = position;
   pointPos = (localXform * vec4(position, 1)).xyz;
   v3Color = colorIn;
   v3Normal = normalIn;
   gl_Position = MVP * vec4(position, 1);
   zdepth = ((gl_DepthRange.diff * gl_Position.z/gl_Position.w) +
              gl_DepthRange.near + gl_DepthRange.far) / 2.0;
   surfId = idx;

   zdepthLinear = -(MV * vec4(pointPos, 1.0)).z;
}
