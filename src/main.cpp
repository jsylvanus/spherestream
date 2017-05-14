#include "SDL.h"

#ifndef __APPLE__
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL_opengl.h>
#else
#define NO_SDL_GLEXT
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#endif

#include "shader.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "opencv2/opencv.hpp"
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace cv;
using namespace glm;

typedef unsigned char U8;

int main() {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window *win = SDL_CreateWindow(
		"SphereStream",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1920, 1080,
		SDL_WINDOW_OPENGL
	);
	if (!win) {
		cout << "Couldn't create window: " << SDL_GetError() << endl;
		return 1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(win);

	// this is basically our rendering surface we're going to point a camera at
	// for our texture video texture.
	GLfloat vbuffer[] = {
		// vertex               // UV
		-0.88888f , -0.5f , 0 , 0.0f , 0.0f , // lower right tri
		0.88888f  , -0.5f , 0 , 1.0f , 0.0f ,
		0.88888f  , 0.5f  , 0 , 1.0f , 1.0f ,
		0.88888f  , 0.5f  , 0 , 1.0f , 1.0f , // upper left tri
		-0.88888f , 0.5f  , 0 , 0.0f , 1.0f ,
		-0.88888f , -0.5f , 0 , 0.0f , 0.0f ,
	};

	// Init webcam
	VideoCapture cap(0); // first usb cam?
	if (!cap.isOpened()) {
		cout << "Couldnt open camera stream." << endl;
		return 1;
	}
	cap.set(CV_CAP_PROP_FORMAT, CV_8UC4); // we want RGBA not BGR24
	int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	Shader prog("shader.vert", "shader.frag");

	// set up camera
	vec3 camera(0, 0, -0.7);
	vec3 origin(0,0,0);
	vec3 up(0, 1, 0);
	mat4 view = glm::lookAt(camera, origin, up);

	// set up perspective view
	// 1/2 pi = 90deg
	const GLfloat pi = 3.141596;
	mat4 projection = glm::perspective(pi / 2.0f, (GLfloat)w / h, 50.0f, 0.1f);

	Mat frame;
	cap >> frame;

	bool running = true;

	// create pixel buffer
	GLuint buffer;
	glGenBuffers(1, &buffer);


	GLuint VBO;
	glGenBuffers(1, &VBO);

	GLuint VAO;
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vbuffer), vbuffer, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// create video texture
	GLuint videoTexture;
	glGenTextures(1, &videoTexture);
	glBindTexture(GL_TEXTURE_2D, videoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	while (running) {
		// input handle
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
				break;
			}
		}

		// render

		// get stream from OPENCV
		cap >> frame; // this is by default a BGR24 stream

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

#define CHECKERROR() do {if (GLenum err = glGetError()) { cerr << "ERROR: " << err << " at line " << __LINE__ << endl; }} while (0)

		prog.Use();

		// set uniforms (projection and view matrices)
		glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uViewMat"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uProjectionMat"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform1i(glGetUniformLocation(prog.Program, "streamingTexture"), 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, videoTexture);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		// prepare to buffer the pixel data into the texture
		glBufferData(GL_PIXEL_UNPACK_BUFFER, w * h * 4, 0, GL_STREAM_DRAW);

		// get mapped memory for pixel data
		GLubyte *ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

		if (ptr) {
			memcpy(ptr, frame.ptr(), w * h * 4);
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		// draw call here
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// unbind buffer between calls
		glBindVertexArray(0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_GL_SwapWindow(win);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
}
