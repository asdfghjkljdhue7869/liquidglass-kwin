#version 140

uniform mat4 modelViewProjectionMatrix;
uniform vec2 bgSize;

in vec2 position;
in vec2 texcoord;

out vec2 uv;
out vec2 vertex;

void main(void)
{
    vec2 drawPos = (bgSize.x > 0.0) ? vec2(texcoord.x * bgSize.x, (1.0 - texcoord.y) * bgSize.y) : position;
    gl_Position = modelViewProjectionMatrix * vec4(drawPos, 0.0, 1.0);
    uv = texcoord;
    vertex = position;
}
