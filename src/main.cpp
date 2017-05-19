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

// must be 3:2
#define WIN_WIDTH 1920
#define WIN_HEIGHT 1280
#define VIEWPORT_SZ 640

#define ONE_DEGREE 0.01745329252f
#define ONE_PIXEL 0.00048828125f

float cameraFOVRadians = 3.1415926536f * 0.5; // 90deg to start
// glm::vec2 cameraLens(0.2099609375f, 0.2099609375f);
// glm::vec2 frontLensCenter(0.70947265, 0.22705078);
// glm::vec2 rearLensCenter(0.2329101, 0.22705078);
glm::vec2 cameraLens(0.208496f, 0.208984f);
glm::vec2 frontLensCenter(0.701172f, 0.233887f);
glm::vec2 rearLensCenter(0.23584f, 0.233887f);

GLfloat g_pitch = 0;
GLfloat g_yaw = 0;
bool cubemap = true;

void DrawVideoHemisphere(const HemisphereGeometry& hemisphere, const Shader& prog) {
	// scale and rotate model
	glm::mat4 modelmat = glm::scale(glm::mat4(1.0f), glm::vec3(50.0));
	glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uModelMat"), 1, GL_FALSE, glm::value_ptr(modelmat));
	// set UV coords
	glUniform2fv(glGetUniformLocation(prog.Program, "uUVOffset"), 1, glm::value_ptr(frontLensCenter));
	// draw
	hemisphere.Draw();

	// REAR CAMERA
	// flip the hemisphere around because what
	glm::mat4 modelmat2 = glm::rotate(glm::rotate(modelmat, 3.1415926536f, glm::vec3(0, 1, 0)), 3.1415926536f, glm::vec3(1, 0, 0));
	glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uModelMat"), 1, GL_FALSE, glm::value_ptr(modelmat2));
	// update uv center
	glUniform2fv(glGetUniformLocation(prog.Program, "uUVOffset"), 1, glm::value_ptr(rearLensCenter));
	// draw again
	hemisphere.Draw();
}

void DrawFullView(const HemisphereGeometry& hemisphere, const Shader& prog) {
	// update g_pitch/g_yaw
	int mdx, mdy;
	SDL_GetRelativeMouseState(&mdx, &mdy);
	g_yaw -= mdx * 0.25;
	g_pitch -= mdy * 0.25;
	if (g_pitch > 89.0f) g_pitch = 89.0f;
	if (g_pitch < -89.0f) g_pitch = -89.0f;

	glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

	vec3 camera(0, 0, 0);
	vec3 direction(
		cos(glm::radians(g_pitch)) * cos(glm::radians(g_yaw)),
		sin(glm::radians(g_pitch)),
		cos(glm::radians(g_pitch)) * sin(-glm::radians(g_yaw))
	);
	vec3 up(0,1,0);
	vec3 target = glm::normalize(direction);
	mat4 view = glm::lookAt(camera, target, up);

	const GLfloat pi = 3.141596;
	mat4 projection = glm::perspective(pi * 0.45f, (float)WIN_WIDTH / (float)WIN_HEIGHT, 90.0f, 0.1f);

	// set uniforms (projection and view matrices)
	glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uProjectionMat"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uViewMat"), 1, GL_FALSE, glm::value_ptr(view));

	DrawVideoHemisphere(hemisphere, prog);
}

void DrawView(const glm::vec3& direction, const glm::vec3& up, int x, int y, const HemisphereGeometry& hemisphere, const Shader& prog) {
	glViewport(x * VIEWPORT_SZ, y * VIEWPORT_SZ, VIEWPORT_SZ, VIEWPORT_SZ);

	vec3 camera(0, 0, 0);
	vec3 target = glm::normalize(direction);
	mat4 view = glm::lookAt(camera, target, up);

	const GLfloat pi = 3.141596;
	mat4 projection = glm::perspective(cameraFOVRadians, 1.0f, 90.0f, 0.1f);

	// set uniforms (projection and view matrices)
	glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uProjectionMat"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(prog.Program, "uViewMat"), 1, GL_FALSE, glm::value_ptr(view));

	DrawVideoHemisphere(hemisphere, prog);
}

void handleKeyDown(const SDL_Keycode& sym) {
	bool caught = true;
	switch (sym) {

		// WASD changes lens shape
		case SDLK_w:
			cameraLens.y += ONE_PIXEL;
			break;
		case SDLK_s:
			cameraLens.y -= ONE_PIXEL;
			break;
		case SDLK_d:
			cameraLens.x += ONE_PIXEL;
			break;
		case SDLK_a:
			cameraLens.x -= ONE_PIXEL;
			break;

		// TFGH moves read lens (left one)
		case SDLK_t:
			rearLensCenter.y += ONE_PIXEL;
			break;
		case SDLK_g:
			rearLensCenter.y -= ONE_PIXEL;
			break;
		case SDLK_h:
			rearLensCenter.x += ONE_PIXEL;
			break;
		case SDLK_f:
			rearLensCenter.x -= ONE_PIXEL;
			break;

		// IJKL moves front lens (right one)
		case SDLK_i:
			frontLensCenter.y += ONE_PIXEL;
			break;
		case SDLK_k:
			frontLensCenter.y -= ONE_PIXEL;
			break;
		case SDLK_l:
			frontLensCenter.x += ONE_PIXEL;
			break;
		case SDLK_j:
			frontLensCenter.x -= ONE_PIXEL;
			break;

		case SDLK_c:
			cubemap = !cubemap; caught = false;
			cout << "Cubemap toggled" << endl;
			break;

		default:
			caught = false;
			break;
	}
	if (caught) {
		cout << "UV configuration updated:" << endl;
		cout << "  Lens shape (" << cameraLens.x << ", " << cameraLens.y << ")" << endl;
		cout << "  Front center (" << frontLensCenter.x << ", " << frontLensCenter.y << ")" << endl;
		cout << "  Rear center (" << rearLensCenter.x << ", " << rearLensCenter.y << ")" << endl;
	}
}

int main() {
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window *win = SDL_CreateWindow(
		"SphereStream",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WIN_WIDTH, WIN_HEIGHT,
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

	SDL_SetRelativeMouseMode(SDL_TRUE);


	while (running) {
		// input handle
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
					handleKeyDown(e.key.keysym.sym);
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
		glUniform2fv(glGetUniformLocation(prog.Program, "uLensShape"), 1, glm::value_ptr(cameraLens)); // TODO: extract constant to a setting

		if (cubemap) {

			DrawView(vec3(1, 0, 0), vec3(0,1,0), 0, 0, hemisphere, prog); // right
			DrawView(vec3(-1, 0, 0), vec3(0,1,0), 1, 0, hemisphere, prog); // left
			DrawView(vec3(0, 1, 0), vec3(0,0,-1), 2, 0, hemisphere, prog); // up
			DrawView(vec3(0, -1, 0), vec3(0,0,1), 0, 1, hemisphere, prog); // down
			DrawView(vec3(0, 0, 1), vec3(0,1,0), 1, 1, hemisphere, prog); // front
			DrawView(vec3(0, 0, -1), vec3(0,1,0), 2, 1, hemisphere, prog); // back

		} else {

			DrawFullView(hemisphere, prog);

		}

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_GL_SwapWindow(win);
	}

	SDL_DestroyWindow(win);
	SDL_Quit();
}
