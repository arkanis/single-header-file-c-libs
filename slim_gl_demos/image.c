/**

This demo loads an image, creates a texture out of it and draws it with a triangle strip.

**/
#include <stdbool.h>
#include <SDL/SDL.h>
#ifdef WIN32
#include "windows/gl_3_1_core.h"
#else
#include "gl_3_1_core.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define SLIM_GL_IMPLEMENTATION
#include "../slim_gl.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s image-file\n", argv[0]);
		return 1;
	}
	
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	
	// Create an OpenGL 3.1 window
	SDL_Window* win = SDL_CreateWindow("SlimGL image", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(0);
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		in  vec2 pos;
		in  vec2 tex_coords;
		out vec2 interpolated_tex_coords;
		
		void main() {
			gl_Position = vec4(pos, 0, 1);
			interpolated_tex_coords = tex_coords;
		}
	), SGL_GLSL("#version 140",
		uniform sampler2D tex;
		
		in vec2 interpolated_tex_coords;
		
		void main() {
			gl_FragColor = texture2D(tex, interpolated_tex_coords);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Load image and create a texture with it
	int img_w = 0, img_h = 0;
	uint8_t* img = stbi_load(argv[1], &img_w, &img_h, NULL, 4);
		if (img == NULL)
			fprintf(stderr, "Failed to load image %s: %s\n", argv[1], stbi_failure_reason());
		GLuint texture = sgl_texture_new(img_w, img_h, 4, img, 0, 0);
	free(img);
	
	// Create a vertex buffer with one quad for the image (actually it's one
	// triangle strip since quads are depricated). The x and y values are in
	// normalized space coordinates, u and v are in texture coordinates.
	struct { float x, y, u, v; } vertices[] = {
		{  1,  1,    1, 0 },  // right top
		{  1, -1,    1, 1 },  // right bottom
		{ -1,  1,    0, 0 },  // left top
		{ -1, -1,    0, 1 }   // left bottom
	};
	GLuint buffer = sgl_buffer_new(vertices, sizeof(vertices));
	
	// Draw image whenever needed
	SDL_Event event;
	while( SDL_WaitEvent(&event) ) {
		if (event.type == SDL_QUIT) {
			break;
		} else if ( (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) || event.type == SDL_MOUSEBUTTONDOWN ) {
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLE_STRIP, program, "pos %2f tex_coords %2f tex %T", buffer, texture);
			SDL_GL_SwapWindow(win);
		}
	}
	
	// Cleanup
	sgl_buffer_destroy(buffer);
	sgl_texture_destroy(texture);
	sgl_program_destroy(program);
	
	SDL_GL_DeleteContext(gl_ctx);
	SDL_DestroyWindow(win);
	
	return 0;
}