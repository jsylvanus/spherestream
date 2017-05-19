#version 330 core

in vec2 texCoords;

out vec4 color;

uniform sampler2D streamingTexture;
uniform vec2 uUVOffset;
uniform float uLensRadius;

void main()
{
	color = texture(streamingTexture, (texCoords * uLensRadius) + uUVOffset);
}
