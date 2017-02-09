/**

This demo draws a single triangle with SDL and OpenGL 3.1.

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
	SDL_Window* win = SDL_CreateWindow("SlimGL triangle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		in vec2 pos;
		in vec3 color;
		out vec3 interpolated_color;
		
		void main() {
			gl_Position = vec4(pos, 0, 1);
			interpolated_color = color;
		}
	), SGL_GLSL("#version 140",
		in vec3 interpolated_color;
		
		void main() {
			gl_FragColor = vec4(interpolated_color, 1);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Create a vertex buffer with one triangle in it
	struct { float x, y, r, g, b; } vertices[] = {
		{    0,  0.5,   1, 0, 0 },  // top
		{  0.5, -0.5,   0, 1, 0 },  // right
		{ -0.5, -0.5,   0, 0, 1 }   // left
	};
	GLuint buffer = sgl_buffer_new(vertices, sizeof(vertices));
	
	// Draw triangle whenever needed
	SDL_Event event;
	while( SDL_WaitEvent(&event) ) {
		if (event.type == SDL_QUIT) {
			break;
		} else if ( (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) || event.type == SDL_MOUSEBUTTONDOWN ) {
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLES, program, "pos %2f color %3f", buffer);
			SDL_GL_SwapWindow(win);
		}
	}
	
	// Cleanup
	sgl_buffer_destroy(buffer);
	sgl_program_destroy(program);
	
	SDL_GL_DeleteContext(gl_ctx);
	SDL_DestroyWindow(win);
	
	return 0;
}