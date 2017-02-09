/**

Renders a rotating cube with an image on it into a framebuffer.
The framebuffer content is then drawn onto the screen with a zoom bubble effect that magnifies the pixels under the
mouse cursor.

You can use the mouse and WASD keys to navigate.

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
	int ww = 800, wh = 600;
	SDL_Window* win = SDL_CreateWindow("SlimGL framebuffer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ww, wh, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(1);
	
	// Enable primitive restart
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xff);
	
	// Create the color buffer texture and framebuffer object
	GLuint color_buffer = sgl_texture_new(ww, wh, 4, NULL, 0, SGL_RECT);
	GLuint framebuffer = sgl_framebuffer_new(color_buffer, SGL_RECT);
	
	// Compile vertex and fragment shaders to draw the color buffer with a zoom bubble
	GLuint screen_rect_program = sgl_program_from_strings(SGL_GLSL("#version 140",
		in  vec2 pos;
		
		void main() {
			gl_Position = vec4(pos, 0, 1);
		}
	), SGL_GLSL("#version 140",
		uniform sampler2DRect tex;
		uniform vec2 mouse_pos;
		
		// Zoom level inside the bubble (2 = double magnification, 4 = four times the size, etc.)
		float zoom_level = 2.0;
		// The distance from the mouse cursor the bubble starts to zoom slightly
		float bubble_outer_radius = 200;
		// The distance from the mouse cursor where the bubble arrives at full zoom. Everything
		// inside is zoomed with `zoom_level`.
		float bubble_inner_radius = 100;
		
		void main() {
			vec2 to_mouse = mouse_pos - gl_FragCoord.xy;
			float distance = length(to_mouse);
			
			// 0 where no zoom is applied (outside of bubble), 1 where full zoom (inside bubble), blended on the edge
			float zoom_mask = 1 - smoothstep(bubble_inner_radius, bubble_outer_radius, distance);
			vec2 offset = to_mouse * mix(0, 1.0 - 1.0 / zoom_level, zoom_mask);
			gl_FragColor = texture2DRect(tex, gl_FragCoord.xy + offset);
		}
	), NULL);
	if (!screen_rect_program)
		return 1;
	
	// Create a vertex buffer with one quad that fills the entire screen.
	struct { float x, y; } screen_rect_vertices[] = {
		{  1, -1 },  // left bottom
		{  1,  1 },  // left top
		{ -1, -1 },  // right bottom
		{ -1,  1 }   // right top
	};
	GLuint screen_rect_buffer = sgl_buffer_new(screen_rect_vertices, sizeof(screen_rect_vertices));
	
	
	// Load image and create a texture with it
	int img_w = 0, img_h = 0;
	uint8_t* img = stbi_load(argv[1], &img_w, &img_h, NULL, 4);
		GLuint texture = sgl_texture_new(img_w, img_h, 4, img, 0, 0);
	free(img);
	float img_aspect_ratio = (float)img_w / img_h;
	
	// Create the vertex and index buffers for a cube
	float w = 1.0 / 2, h = (1.0 / img_aspect_ratio) / 2;
	struct { float x, y, z,   u, v; } vertices[] = {
		// Front
		{  w, -h,  w,   1, 1 },
		{  w,  h,  w,   1, 0 },
		{ -w, -h,  w,   0, 1 },
		{ -w,  h,  w,   0, 0 },
		// Left
		{ -w, -h,  w,   1, 1 },
		{ -w,  h,  w,   1, 0 },
		{ -w, -h, -w,   0, 1 },
		{ -w,  h, -w,   0, 0 },
		// Back
		{ -w, -h, -w,   1, 1 },
		{ -w,  h, -w,   1, 0 },
		{  w, -h, -w,   0, 1 },
		{  w,  h, -w,   0, 0 },
		// Right
		{  w,  h,  w,   0, 0 },
		{  w, -h,  w,   0, 1 },
		{  w,  h, -w,   1, 0 },
		{  w, -h, -w,   1, 1 },
		// Top
		{  w,  h,  w,   0, 0 },
		{  w,  h, -w,   0, 0 },
		{ -w,  h,  w,   0, 0 },
		{ -w,  h, -w,   0, 0 },
		// Bottom
		{  w, -h, -w,   0, 0 },
		{  w, -h,  w,   0, 0 },
		{ -w, -h, -w,   0, 0 },
		{ -w, -h,  w,   0, 0 }
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
		uniform mat4 model_view;
		uniform mat4 projection;
		
		in  vec3 pos;
		in  vec2 tex_coords;
		out vec2 interpolated_tex_coords;
		
		void main() {
			vec4 camera_space = model_view * vec4(pos, 1);
			gl_Position = projection * camera_space;
			interpolated_tex_coords = tex_coords;
		}
	), SGL_GLSL("#version 140",
		uniform sampler2D tex;
		
		in vec2 interpolated_tex_coords;
		
		void main(){
			gl_FragColor = texture2D(tex, interpolated_tex_coords);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Setup projection, camera, model and model-view matrix
	mat4_t projection_matrix = m4_perspective(60, (float)ww / wh, 0.1f, 100);
	vec3_t camera_pos = {-1, 0.5, 2}, camera_dir = {1, -0.5, -2}, camera_up = {0, 1, 0};
	mat4_t model_matrix = m4_identity();
	
	// Don't render (cull) backfaces, that are faces the camera can only see from behind.
	glEnable(GL_CULL_FACE);
	
	bool quit = false;
	float mouse_pos[2];
	uint32_t ticks_last_frame = SDL_GetTicks(), ticks_this_frame = 0;
	while(!quit) {
		// Calculate how much time has passed since the last frame (dt in seconds)
		ticks_this_frame = SDL_GetTicks();
		float dt = (ticks_this_frame - ticks_last_frame) / 1000.0f;
		ticks_last_frame = ticks_this_frame;
		
		// React to user input
		SDL_Event event;
		while( SDL_PollEvent(&event) ) {
			if (event.type == SDL_QUIT) {
				quit = true;
				break;
			}
			
			switch(event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_LEFT:
					model_matrix = m4_mul( m4_rotation_y(-0.1 * M_PI), model_matrix );
					break;
				case SDLK_RIGHT:
					model_matrix = m4_mul( m4_rotation_y( 0.1 * M_PI), model_matrix );
					break;
				case SDLK_UP:
					model_matrix = m4_mul( m4_rotation_x(-0.1 * M_PI), model_matrix );
					break;
				case SDLK_DOWN:
					model_matrix = m4_mul( m4_rotation_x( 0.1 * M_PI), model_matrix );
					break;
				case SDLK_w:
					camera_pos = v3_add(camera_pos, v3_muls(camera_dir, 0.5));
					break;
				case SDLK_s:
					camera_pos = v3_add(camera_pos, v3_muls(camera_dir, -0.5));
					break;
				case SDLK_a:
					camera_pos = v3_add(camera_pos, v3_muls(v3_cross(camera_dir, camera_up), -0.5));
					break;
				case SDLK_d:
					camera_pos = v3_add(camera_pos, v3_muls(v3_cross(camera_dir, camera_up), 0.5));
					break;
				}
				break;
				
			case SDL_MOUSEBUTTONDOWN:
				SDL_SetRelativeMouseMode(true);
				break;
			case SDL_MOUSEBUTTONUP:
				SDL_SetRelativeMouseMode(false);
				break;
			case SDL_MOUSEMOTION:
				// Store mouse pos in OpenGL window space (origin in bottom left corner)
				mouse_pos[0] = event.motion.x;
				mouse_pos[1] = wh - event.motion.y;
				
				if ( SDL_GetRelativeMouseMode() ) {
					if (event.motion.xrel != 0)
						camera_dir = m4_mul_dir( m4_rotation(0.001 * M_PI * -event.motion.xrel, camera_up), camera_dir );
					if (event.motion.yrel != 0)
						camera_dir = m4_mul_dir( m4_rotation(0.001 * M_PI * -event.motion.yrel, v3_cross(camera_dir, camera_up)), camera_dir );
					camera_dir = v3_norm(camera_dir);
				}
				break;
			}
		}
		
		// Simulate
		float angular_velocity = 0.25 * M_PI;
		model_matrix = m4_mul( m4_rotation_y(angular_velocity * dt), model_matrix );
		
		// Refresh camera and model-view matrix
		mat4_t camera_matrix = m4_look_at(camera_pos, v3_add(camera_pos, camera_dir), camera_up);
		mat4_t model_view_matrix = m4_mul(camera_matrix, model_matrix);
		
		// Draw
		sgl_framebuffer_bind(framebuffer, ww, wh);
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLE_STRIP, program, "$bI model_view %4M projection %4M tex %T pos %3f tex_coords %2f", index_buffer, &model_view_matrix, &projection_matrix, texture, vertex_buffer);
		sgl_framebuffer_bind(0, ww, wh);
		
		sgl_draw(GL_TRIANGLE_STRIP, screen_rect_program, "mouse_pos %2F tex %rT pos %2f", mouse_pos, color_buffer, screen_rect_buffer);
		SDL_GL_SwapWindow(win);
	}
	
	// Cleanup
	sgl_framebuffer_destroy(framebuffer);
	sgl_texture_destroy(color_buffer);
	
	sgl_texture_destroy(texture);
	sgl_buffer_destroy(vertex_buffer);
	sgl_buffer_destroy(index_buffer);
	sgl_program_destroy(program);
	
	SDL_GL_DeleteContext(gl_ctx);
	SDL_DestroyWindow(win);
	
	return 0;
}