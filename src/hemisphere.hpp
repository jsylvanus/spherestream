#if !defined(HEMISPHERE_HPP)
#define HEMISPHERE_HPP

#ifndef __APPLE__
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL_opengl.h>
#else
#define NO_SDL_GLEXT
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#endif

#include <vector>
#include <glm/glm.hpp>

const float TAU = 6.2831853072;
const float PI = 3.1415926536;

class HemisphereGeometry {
	float radius;
	float aperture;
	int circleSegments;
	int heightSegments;

	GLuint vao;
	GLuint vertexBuffer;
	GLuint elementBuffer;

	struct point {
		GLfloat x, y, z, u, v;
	};

	std::vector<point> pointdata;
	std::vector<unsigned int> indices;

	void generatePoints();
	void createVAO();
public:
	HemisphereGeometry(float rad, int circleSeg, int heightSeg, float ap);
	void Draw();
};

#endif
