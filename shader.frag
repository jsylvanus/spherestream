#version 330 core

in vec2 texCoords;

out vec4 color;

uniform sampler2D streamingTexture;
uniform vec2 uUVOffset;
uniform vec2 uLensShape;

void main()
{
	color = texture(streamingTexture, vec2(texCoords.x * uLensShape.x, texCoords.y * uLensShape.y) + uUVOffset);
}
