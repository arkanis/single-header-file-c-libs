/**

Demonstrates basic Slim GL usage to render a wireframe cube with an orthographic projection.

**/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <SDL/SDL.h>
#ifdef WIN32
#include "windows/gl_3_1_core.h"
#else
#include "gl_3_1_core.h"
#endif

#define SLIM_GL_IMPLEMENTATION
#include "../slim_gl.h"
#define MATH_3D_IMPLEMENTATION
#include "../math_3d.h"



int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	
	// Create an OpenGL 3.1 window
	int ww = 800, wh = 600;
	SDL_Window* win = SDL_CreateWindow("SlimGL cube", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ww, wh, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(0);
	
	// Enable primitive restart (an index of 0xff restarts the primitive, e.g. triangle strip)
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xff);
	
	// Create the vertex and index buffers for a cube
	float w = 1.0 / 2, h = 1.0 / 2;
	struct { float x, y, z; } vertices[] = {
		// Front
		{  w, -h,  w },
		{  w,  h,  w },
		{ -w, -h,  w },
		{ -w,  h,  w },
		// Left
		{ -w, -h,  w },
		{ -w,  h,  w },
		{ -w, -h, -w },
		{ -w,  h, -w },
		// Back
		{ -w, -h, -w },
		{ -w,  h, -w },
		{  w, -h, -w },
		{  w,  h, -w },
		// Right
		{  w,  h,  w },
		{  w, -h,  w },
		{  w,  h, -w },
		{  w, -h, -w },
		// Top
		{  w,  h,  w },
		{  w,  h, -w },
		{ -w,  h,  w },
		{ -w,  h, -w },
		// Bottom
		{  w, -h, -w },
		{  w, -h,  w },
		{ -w, -h, -w },
		{ -w, -h,  w }
	};
	GLuint vertex_buffer = sgl_buffer_new(vertices, sizeof(vertices));
	uint8_t indices[] = {
		 0,  1,  2,  3,  0xff,
		 4,  5,  6,  7,  0xff,
		 8,  9, 10, 11,  0xff,
		12, 13, 14, 15,  0xff,
		16, 17, 18, 19,  0xff,
		20, 21, 22, 23
	};
	GLuint index_buffer = sgl_buffer_new(indices, sizeof(indices));
	
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		uniform mat4 projection;
		in vec3 pos;
		
		void main() {
			gl_Position = projection * vec4(pos, 1);
		}
	), SGL_GLSL("#version 140",
		void main() {
			gl_FragColor = vec4(1);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Setup projection, camera, model and model-view matrix
	mat4_t projection_matrix = m4_ortho(-2, 2, -2, 2, -2, 2);
	mat4_t model_view_matrix = m4_identity();
	
	// Switch to wireframe rendering
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// Don't render (cull) backfaces, that are faces the camera can only see from behind.
	glEnable(GL_CULL_FACE);
	
	SDL_Event event;
	while( SDL_WaitEvent(&event) ) {
		if (event.type == SDL_QUIT) {
			break;
		} else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) {
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLE_STRIP, program, "$bI projection %4M pos %3f", index_buffer, &projection_matrix, vertex_buffer);
			SDL_GL_SwapWindow(win);
		}
	}
	
	// Cleanup
	sgl_buffer_destroy(vertex_buffer);
	sgl_buffer_destroy(index_buffer);
	sgl_program_destroy(program);
	
	SDL_GL_DeleteContext(gl_ctx);
	SDL_DestroyWindow(win);
	
	return 0;
}