/**

This demo shows the usage of math_3d.h for perspective projection of vertices.

**/
#include <SDL/SDL.h>
#ifdef WIN32
#include "windows/gl_3_1_core.h"
#else
#include "gl_3_1_core.h"
#endif

#define MATH_3D_IMPLEMENTATION
#include "../math_3d.h"
#define SLIM_GL_IMPLEMENTATION
#include "../slim_gl.h"


int main(int argc, char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	
	// Create an OpenGL 3.1 window
	SDL_Window* win = SDL_CreateWindow("math_3d.h demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		uniform mat4 projection;
		uniform mat4 transform;
		in vec3 pos;
		
		void main() {
			gl_Position = (projection * transform) * vec4(pos, 1);
		}
	), SGL_GLSL("#version 140",
		void main() {
			gl_FragColor = vec4(0, gl_FragCoord.z, 0, 1);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Create a vertex buffer with one triangle in it
	struct { float x, y, z; } vertices[] = {
		{  0,  0,  0 },  // left bottom
		{  1,  0, -1 },  // right bottom
		{  1,  1, -1 }   // top
	};
	GLuint buffer = sgl_buffer_new(vertices, sizeof(vertices));
	
	// Switch to wireframe rendering
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	mat4_t projection = m4_perspective(60, 800.0 / 600.0, 1, 10);
	vec3_t from = vec3(0, 0.5, 2), to = vec3(0, 0, 0), up = vec3(0, 1, 0);
	mat4_t transform = m4_look_at(from, to, up);
	
	vec3_t world_space = vec3(1, 1, -1);
	mat4_t world_to_screen_space = m4_mul(projection, transform);
	vec3_t screen_space = m4_mul_pos(world_to_screen_space, world_space);
	printf("%.2f %.2f %.2f\n", screen_space.x, screen_space.y, screen_space.z);
	
	// Draw triangle whenever needed
	SDL_Event event;
	while( SDL_WaitEvent(&event) ) {
		if (event.type == SDL_QUIT) {
			break;
		} else if ( (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) || event.type == SDL_MOUSEBUTTONDOWN ) {
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLES, program, "projection %4M transform %4M pos %3f", &projection, &transform, buffer);
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