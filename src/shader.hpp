#if !defined(SHADER_HPP)
#define SHADER_HPP

#ifndef __APPLE__
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL_opengl.h>
#else
#define NO_SDL_GLEXT
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#endif

class Shader
{
public:
	// Program ID
	GLuint Program;
	Shader(const GLchar* vertexPath, const GLchar* fragmentPath);
	void Use();
};

#endif
