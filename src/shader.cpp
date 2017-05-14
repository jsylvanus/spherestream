#include "shader.hpp"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

string readFile(const GLchar* path) {
	ifstream file;
	file.exceptions(ifstream::badbit);
	file.open(path);
	stringstream stream;
	stream << file.rdbuf();
	file.close();
	return stream.str();
}

Shader::Shader(const GLchar* vertexPath, const GLchar* fragmentPath)
{
	// read files
	string vertexCode, fragmentCode;
	try {
		vertexCode = readFile(vertexPath);
		fragmentCode = readFile(fragmentPath);
	} catch (ifstream::failure e) {
		cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << endl;
	}
	const GLchar* vShaderCode = vertexCode.c_str();
	const GLchar* fShaderCode = fragmentCode.c_str();

	// compile shaders
	GLuint vertex, fragment;
	GLint success;
	GLchar infoLog[512];

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		cout << "VERTEX DERP: " << infoLog << endl;
	}

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		cout << "FRAGMENT DERP: " << infoLog << endl;
	}

	// link program
	this->Program = glCreateProgram();
	glAttachShader(this->Program, vertex);
	glAttachShader(this->Program, fragment);
	glLinkProgram(this->Program);
	glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
		cout << "PROGRAM DERP: " << infoLog << endl;
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::Use()
{
	glUseProgram(this->Program);
}
