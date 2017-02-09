/**

This demo loads an image, creates a texture out of it and draws it with a triangle strip.
As an added bonus it magnifies the image around the mouse cursor.

**/
#include <stdbool.h>

#include <SDL/SDL.h>
#ifdef WIN32
#include "windows/gl_3_1_core.h"
#else
#include "gl_3_1_core.h"
#endif

#define SLIM_GL_IMPLEMENTATION
#include "../slim_gl.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s image-file\n", argv[0]);
		return 1;
	}
	
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	
	// Create an OpenGL 3.1 window
	int win_w = 800, win_h = 600;
	SDL_Window* win = SDL_CreateWindow("SlimGL image bubble", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, win_w, win_h, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(0);
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		uniform mat3 projection;

		in  vec2 pos;
		in  vec2 tex_coords;
		out vec2 interpolated_tex_coords;
		out vec2 screen_pos;
		
		void main() {
			gl_Position = vec4(projection * vec3(pos, 1), 1);
			interpolated_tex_coords = tex_coords;
			screen_pos = pos;
		}
	), SGL_GLSL("#version 140",
		uniform sampler2DRect tex;
		uniform vec2 mouse_pos;
		uniform float img_pixels_per_quad_pixel;
		
		in vec2 interpolated_tex_coords;
		in vec2 screen_pos;
		
		// Zoom level inside the bubble (2 = double magnification, 4 = four times the size, etc.)
		float zoom_level = 2.0;
		// The distance from the mouse cursor the bubble starts to zoom slightly
		float bubble_outer_radius = 200;
		// The distance from the mouse cursor where the bubble arrives at full zoom. Everything
		// inside is zoomed with `zoom_level`.
		float bubble_inner_radius = 100;
		
		void main() {
			vec2 to_mouse = mouse_pos - screen_pos;
			float distance = length(to_mouse);
			
			// 0 where no zoom is applied (outside of bubble), 1 where full zoom (inside bubble), blended on the edge
			float zoom_mask = 1 - smoothstep(bubble_inner_radius, bubble_outer_radius, distance);
			vec2 offset = to_mouse * mix(0, 1.0 - 1.0 / zoom_level, zoom_mask);
			gl_FragColor = texture2DRect(tex, interpolated_tex_coords + offset * img_pixels_per_quad_pixel);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Load image and create a texture with it
	int img_w = 0, img_h = 0;
	uint8_t* img = stbi_load(argv[1], &img_w, &img_h, NULL, 4);
		GLuint texture = sgl_texture_new(img_w, img_h, 4, img, 0, SGL_RECT);
	free(img);
	
	// Calculate the size of the quad we use to display the image (based on the
	// image and window aspect ratios).
	int quad_w = win_w, quad_h = win_h;
	float img_ar = (float)img_w / img_h, win_ar = (float)win_w / win_h;
	if (img_ar > win_ar) {
		quad_h = quad_w / img_ar;
	} else {
		quad_w = quad_h * img_ar;
	}
	float img_pixels_per_quad_pixel = (float)img_w / quad_w;
	
	// Create a vertex buffer with one quad for the image (actually it's one
	// triangle strip since quads are depricated). The x and y values are in
	// window coordinates, u and v are in texture coordinates.
	float border_x = (win_w - quad_w) / 2.0f, border_y = (win_h - quad_h) / 2.0f;
	struct { float x, y, u, v; } vertices[] = {
		{ win_w - border_x,         border_y,    img_w, 0     },  // right top
		{ win_w - border_x, win_h - border_y,    img_w, img_h },  // right bottom
		{         border_x,         border_y,    0,     0     },  // left top
		{         border_x, win_h - border_y,    0,     img_h }   // left bottom
	};
	GLuint buffer = sgl_buffer_new(vertices, sizeof(vertices));
	
	// Setup the screen space to normalized space projection matrix. We write
	// it straight down but note that from OpenGL's point of view (column-major
	// notation) it's transposed.
	float projection[9] = {
		2.0 / win_w,            0, -1,
		          0, -2.0 / win_h,  1,
		          0,            0,  1
	};
	
	
	SDL_Event event;
	float mouse_pos[2];
	while( SDL_WaitEvent(&event) ) {
		bool redraw = true;
		
		if (event.type == SDL_QUIT) {
			break;
		} else if (event.type == SDL_MOUSEMOTION) {
			mouse_pos[0] = event.motion.x;
			mouse_pos[1] = event.motion.y;
			redraw = true;
		} else if ( (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED) || event.type == SDL_MOUSEBUTTONDOWN ) {
			redraw = true;
		}
		
		if (redraw) {
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLE_STRIP, program, "projection %3tM mouse_pos %2F img_pixels_per_quad_pixel %1F pos %2f tex_coords %2f tex %rT", projection, mouse_pos, &img_pixels_per_quad_pixel, buffer, texture);
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