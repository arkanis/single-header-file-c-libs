/**

Draws an Wavefront OBJ model as a wireframe. Also implements a WASD + mouselook camera so you can look around.

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



/**
 * A small OBJ reader that only reads vertex positions and triangle faces.
 */
bool load_model(const char* filename, float** vertex_buffer, size_t* vertex_buffer_size, uint16_t** index_buffer, size_t* index_buffer_size) {
	FILE* file = fopen(filename, "r");
	if (file == NULL)
		return false;
	
	// Count vertices and faces first
	size_t vertex_count = 0, face_count = 0;
	char line[1024];
	while( fgets(line, 1024, file) != NULL ) {
		if (line[0] == 'v' && line[1] == ' ')
			vertex_count++;
		else if (line[0] == 'f')
			face_count++;
	}
	
	// Allocate buffers
	*vertex_buffer_size = vertex_count * sizeof(float)*3;
	float* vb = malloc(*vertex_buffer_size);
	*index_buffer_size = face_count * sizeof(uint16_t)*3;
	uint16_t* ib = malloc(*index_buffer_size);
	
	// Read vertices and faces
	size_t vi = 0, fi = 0;
	rewind(file);
	while( fgets(line, 1024, file) != NULL ) {
		// Basic OBJ format (what kind of lines we have to expect):
		// http://en.wikipedia.org/wiki/Wavefront_.obj_file
		if (line[0] == 'v' && line[1] == ' ') {
			// vertex position (v)
			sscanf(line, "v %f %f %f", &vb[vi+0], &vb[vi+1], &vb[vi+2]);
			vi += 3;
		} else if (line[0] == 'f') {
			// face (be aware: indices start with 1 instead of 0!)
			sscanf(line, "f %hu %hu %hu", &ib[fi+0], &ib[fi+1], &ib[fi+2]);
			ib[fi+0] -= 1;
			ib[fi+1] -= 1;
			ib[fi+2] -= 1;
			fi += 3;
		}
	}
	
	*vertex_buffer = vb;
	*index_buffer = ib;
	
	// Print the vertex and index buffer for debugging
	printf("%zu vertices, %zu bytes:\n", vertex_count, *vertex_buffer_size);
	for(size_t i = 0; i < vertex_count; i++)
		printf("  [%2zu]: % 6.1f % 6.1f % 6.1f\n", i, vb[i*3+0], vb[i*3+1], vb[i*3+2]);
	printf("%zu faces, %zu bytes:\n", face_count, *index_buffer_size);
	for(size_t i = 0; i < face_count; i++)
		printf("  [%2zu]: %3hu %3hu %3hu\n", i, ib[i*3+0], ib[i*3+1], ib[i*3+2]);
	
	return true;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s obj-model-file\n", argv[0]);
		return 1;
	}
	
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	
	// Create an OpenGL 3.1 window
	int ww = 800, wh = 600;
	SDL_Window* win = SDL_CreateWindow("SlimGL OBJ wireframe", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ww, wh, SDL_WINDOW_OPENGL);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
	SDL_GL_SetSwapInterval(0);
	
	// Load OBJ model and create OpenGL vertex and index buffers
	size_t vertex_buffer_size = 0;
	float* vertex_buffer_ptr = NULL;
	size_t index_buffer_size = 0;
	uint16_t* index_buffer_ptr = NULL;
	
	if ( ! load_model(argv[1], &vertex_buffer_ptr, &vertex_buffer_size, &index_buffer_ptr, &index_buffer_size) ) {
		fprintf(stderr, "Failed to load model %s\n", argv[1]);
		return 1;
	}
	
	GLuint vertex_buffer = sgl_buffer_new(vertex_buffer_ptr, vertex_buffer_size);
	GLuint index_buffer = sgl_buffer_new(index_buffer_ptr, index_buffer_size);
	free(vertex_buffer_ptr);
	free(index_buffer_ptr);
	
	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		uniform mat4 model_view;
		uniform mat4 projection;
		
		in vec3 pos;
		
		void main() {
			vec4 camera_space = model_view * vec4(pos, 1);
			gl_Position = projection * camera_space;
		}
	), SGL_GLSL("#version 140",
		void main(){
			gl_FragColor = vec4(1, 1, 1, 1);
		}
	), NULL);
	if (!program)
		return 1;
	
	// Setup projection, camera, model and model-view matrix
	mat4_t projection_matrix = m4_perspective(60, (float)ww / wh, 0.1, 100);
	//mat4_t projection_matrix = m4_ortho(-2, 2, -2, 2, -2, 2);
	vec3_t camera_pos = {0, 0, 10}, camera_dir = {0, 0, -1}, camera_up = {0, 1, 0};
	mat4_t model_matrix = m4_identity();
	
	// Switch to wireframe rendering
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// Don't render (cull) backfaces, that are faces the camera can only see from behind
	glEnable(GL_CULL_FACE);
	
	SDL_Event event;
	while( SDL_WaitEvent(&event) ) {
		if (event.type == SDL_QUIT)
			break;
		
		bool redraw = false;
		switch(event.type) {
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
					redraw = true;
				break;
				
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_LEFT:
					model_matrix = m4_mul( m4_rotation_y(-0.1 * M_PI), model_matrix );
					redraw = true;
					break;
				case SDLK_RIGHT:
					model_matrix = m4_mul( m4_rotation_y( 0.1 * M_PI), model_matrix );
					redraw = true;
					break;
				case SDLK_UP:
					model_matrix = m4_mul( m4_rotation_x(-0.1 * M_PI), model_matrix );
					redraw = true;
					break;
				case SDLK_DOWN:
					model_matrix = m4_mul( m4_rotation_x( 0.1 * M_PI), model_matrix );
					redraw = true;
					break;
				case SDLK_w:
					camera_pos = v3_add(camera_pos, v3_muls(camera_dir, 0.5));
					redraw = true;
					break;
				case SDLK_s:
					camera_pos = v3_add(camera_pos, v3_muls(camera_dir, -0.5));
					redraw = true;
					break;
				case SDLK_a:
					camera_pos = v3_add(camera_pos, v3_muls(v3_cross(camera_dir, camera_up), -0.5));
					redraw = true;
					break;
				case SDLK_d:
					camera_pos = v3_add(camera_pos, v3_muls(v3_cross(camera_dir, camera_up), 0.5));
					redraw = true;
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
				if ( SDL_GetRelativeMouseMode() ) {
					if (event.motion.xrel != 0)
						camera_dir = m4_mul_dir( m4_rotation(0.001 * M_PI * -event.motion.xrel, camera_up), camera_dir );
					if (event.motion.yrel != 0)
						camera_dir = m4_mul_dir( m4_rotation(0.001 * M_PI * -event.motion.yrel, v3_cross(camera_dir, camera_up)), camera_dir );
					camera_dir = v3_norm(camera_dir);
					redraw = true;
				}
				break;
		}
		
		if (redraw) {
			mat4_t camera_matrix = m4_look_at(camera_pos, v3_add(camera_pos, camera_dir), camera_up);
			mat4_t model_view_matrix = m4_mul(camera_matrix, model_matrix);
			
			glClearColor(0, 0, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			sgl_draw(GL_TRIANGLES, program, "$sI model_view %4M projection %4M pos %3f", index_buffer, &model_view_matrix, &projection_matrix, vertex_buffer);
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