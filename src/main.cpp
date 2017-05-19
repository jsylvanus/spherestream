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

#include "hemisphere.hpp"
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

	// Init webcam
	VideoCapture cap(0); // first usb cam?
	if (!cap.isOpened()) {
		cout << "Couldnt open camera stream." << endl;
		return 1;
	}
	// cap.set(CV_CAP_PROP_FORMAT, CV_8UC4); // we want RGBA not BGR24 but this doesn't work
	int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

	const GLfloat tW = w/2048.0;
	const GLfloat tH = h/2048.0;

	Shader prog("shader.vert", "shader.frag");

	// set up perspective view
	// 1/2 pi = 90deg
	const GLfloat pi = 3.141596;
	mat4 projection = glm::perspective(pi / 2.0f, (GLfloat)w / h, 90.0f, 0.1f);

	Mat frame;
	cap >> frame;

	bool running = true;

	// create pixel buffer
	GLuint buffer;
	glGenBuffers(1, &buffer);

	// geometry and texture coordinates
	HemisphereGeometry hemisphere(5.0, 50, 20, 3.1415926536);

	// create video texture
	GLuint videoTexture;
	glGenTextures(1, &videoTexture);
	glBindTexture(GL_TEXTURE_2D, videoTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	// out color conversion buffer for BGR24->RGBA
	GLubyte *convertBuffer = new GLubyte[w*h*4];

	GLfloat pitch = 0;
	GLfloat yaw = 0;

	SDL_SetRelativeMouseMode(SDL_TRUE);

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

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

#define CHECKERROR() do {if (GLenum err = glGetError()) { cerr << "ERROR: " << err << " at line " << __LINE__ << endl; }} while (0)

		prog.Use();

		// set up camera

		// update pitch/yaw
		int mdx, mdy;
		SDL_GetRelativeMouseState(&mdx, &mdy);
		yaw += mdx * 0.25;
		pitch += mdy * 0.25;
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;

		vec3 camera(0, 0, 0);
		vec3 target = glm::normalize(vec3(
			cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
			sin(glm::radians(pitch)),
			cos(glm::radians(pitch)) * sin(glm::radians(yaw))
		));
		vec3 up(0, 1, 0);
		mat4 view = glm::lookAt(camera, target, up);

		// TODO: capture mouse input and rotate camera pitch/yaw

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
			// convert from BGR24 to RGBA
			Mat properRGBA(frame.size(), CV_8UC4, ptr);
			cv::cvtColor(frame, properRGBA, CV_BGR2RGBA, 4);
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		// draw call here

		// eye radius
		glUniform1f(glGetUniformLocation(prog.Program, "uLensRadius"), 0.2099609);

		// FRONT CAMERA

		// scale and rotate model
		glm::mat4 modelmat = glm::scale(glm::mat4(1.0f), glm::vec3(5.0));
		glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uModelMat"), 1, GL_FALSE, glm::value_ptr(modelmat));
		// set UV coords
		vec2 uvcenter(0.70947265, 0.22705078);
		glUniform2fv(glGetUniformLocation(prog.Program, "uUVOffset"), 1, glm::value_ptr(uvcenter));
		// draw
		hemisphere.Draw();

		// REAR CAMERA
		// rotate modelmat 180 deg
		glm::mat4 modelmat2 = glm::rotate(glm::rotate(modelmat, 3.1415926536f, glm::vec3(0, 1, 0)), 3.1415926536f, glm::vec3(1, 0, 0));
		glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uModelMat"), 1, GL_FALSE, glm::value_ptr(modelmat2));
		// update uv center
		vec2 uvcenter2(0.2329101, 0.22705078);
		glUniform2fv(glGetUniformLocation(prog.Program, "uUVOffset"), 1, glm::value_ptr(uvcenter2));
		// draw again
		hemisphere.Draw();

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_GL_SwapWindow(win);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
}
