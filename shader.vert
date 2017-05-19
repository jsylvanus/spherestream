#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoordsIn;

uniform mat4 uViewMat;
uniform mat4 uProjectionMat;
uniform mat4 uModelMat;

out vec2 texCoords;

void main() {
	gl_Position = uProjectionMat * uViewMat * uModelMat * vec4(position, 1.0);
	texCoords = texCoordsIn;
}
