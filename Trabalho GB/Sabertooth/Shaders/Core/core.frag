#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D tex;
uniform vec3 color;
uniform int hasTexture;

void main()
{
    if (hasTexture == 1)
        FragColor = texture(tex, TexCoord);
    else
        FragColor = vec4(color, 1.0);
}