#version 150
//410

in vec3 position;
out vec4 col;

float position_scale = 50.0/256.0f;
float color_scale = 1.0/255.0f;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position.x, position.y*position_scale,position.z,1.0f);
    float color = position.y * color_scale;
  col = vec4(color,color,color,1.0f);
}

