/**

This demo draws some quads using triangle strips, an index buffer and primitive restart.

**/
#include <SDL/SDL.h>
#ifdef WIN32
#include "windows/gl_3_1_core.h"
#else
#include "gl_3_1_core.h"
#endif

#define SLIM_GL_IMPLEMENTATION
#include "../slim_gl.h"


int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	
	// Create an OpenGL 3.1 window
	SDL_Window* win = SDL_CreateWindow("SlimGL index buffer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);
	
	// Enable primitive restart
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xff);
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		in  vec2 pos;
		in  vec4 color;
		out vec4 interpolated_color;
		
		void main() {
			gl_Position = vec4(pos, 0, 1);
			interpolated_color = color;
		}
	), SGL_GLSL("#version 140",
		in vec4 interpolated_color;
		
		void main() {
			gl_FragColor = interpolated_color;
		}
	), NULL);
	if (!program)
		return 1;
	
	// Create the vertex and index buffers
	struct { float x, y; uint8_t r, g, b, a; } vertices[] = {
		{ -0.75,  0.75,   255,   0,   0, 255 },
		{ -0.25,  0.75,     0, 255,   0, 255 },
		{ -0.25,  0.25,     0,   0, 225, 255 },
		{ -0.75,  0.25,     0, 255, 225, 255 },
		
		{  0.75,  0.75,   255,   0,   0, 255 },
		{  0.25,  0.75,     0, 255,   0, 255 },
		{  0.25,  0.25,     0,   0, 225, 255 },
		{  0.75,  0.25,     0, 255, 225, 255 },
		
		{  0.75, -0.75,     0, 225,   0, 255 },
		{ -0.75, -0.75,     0, 225,   0, 255 },
		{ -0.75, -0.25,     0,   0, 225, 255 },
		{  0.75, -0.25,     0,   0, 225, 255 }
	};
	GLuint vertex_buffer = sgl_buffer_new(vertices, sizeof(vertices));
	uint8_t indices[] = { 1, 2, 0, 3,  0xff,  5, 6, 4, 7,  0xff,  9, 10, 8, 11 };
	GLuint index_buffer = sgl_buffer_new(indices, sizeof(indices));
	
	// Draw triangles whenever needed
	SDL_Event event;
	while( SDL_WaitEvent(&event) ) {
		if (event.type == SDL_QUIT) {
			break;
		} else if ( (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) || event.type == SDL_MOUSEBUTTONDOWN ) {
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLE_STRIP, program, "$bI pos %2f color %4nub", index_buffer, vertex_buffer);
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