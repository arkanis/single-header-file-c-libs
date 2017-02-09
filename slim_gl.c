// For fmemstream()
#define _GNU_SOURCE

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "sgl.h"


static bool gl_error(const char* description, ...);
static GLuint create_and_compile_shader(GLenum shader_type, const char* code, GLint code_size, FILE* error_stream);

#ifndef _SGL_UTILS
void* fload(const char* filename, size_t* size);
#endif


//
// Shader functions
//

GLuint program_new(const char* vertex_shader_file, const char* fragment_shader_file, FILE* compiler_message_stream) {
	char* vertex_shader_code = fload(vertex_shader_file, NULL);
	if (vertex_shader_code == NULL) {
		if (compiler_message_stream) fprintf(compiler_message_stream, "Failed to read vertex shader file %s: %s\n", vertex_shader_file, strerror(errno));
		return 0;
	}
	
	char* fragment_shader_code = fload(fragment_shader_file, NULL);
	if (fragment_shader_code == NULL) {
		free(vertex_shader_code);
		if (compiler_message_stream) fprintf(compiler_message_stream, "Failed to read fragment shader file %s: %s\n", fragment_shader_file, strerror(errno));
		return 0;
	}
	
	GLuint program = program_new_from_string(vertex_shader_code, fragment_shader_code, compiler_message_stream);
	
	free(vertex_shader_code);
	free(fragment_shader_code);
	
	return program;
}

GLuint program_new_from_string(const char* vertex_shader_code, const char* fragment_shader_code, FILE* compiler_message_stream) {
	GLuint vertex_shader = create_and_compile_shader(GL_VERTEX_SHADER, vertex_shader_code, -1, compiler_message_stream);
	GLuint fragment_shader = create_and_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_code, -1, compiler_message_stream);
	if (vertex_shader == 0 || fragment_shader == 0)
		goto shaders_failed;
	
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vertex_shader);
	glAttachShader(prog, fragment_shader);
	glLinkProgram(prog);
	
	GLint result = GL_TRUE;
	glGetProgramiv(prog, GL_LINK_STATUS, &result);
	if (result == GL_FALSE){
		if (compiler_message_stream) {
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &result);
			char buffer[result];
			glGetProgramInfoLog(prog, result, NULL, buffer);
			fprintf(compiler_message_stream, "Linking of vertex and pixel shader failed:\n%s\n", buffer);
			program_inspect(prog, compiler_message_stream);
		}
		goto program_failed;
	}
	
	return prog;
	
	program_failed:
		if (prog)
			glDeleteProgram(prog);
		
	shaders_failed:
		if (vertex_shader)
			glDeleteShader(vertex_shader);
		if (fragment_shader)
			glDeleteShader(fragment_shader);
	
	return 0;
}


/**
 * Loads and compiles a source code file as a shader.
 * 
 * Returns the shaders GL object id on success or 0 on error. Compiler errors in the shader
 * are appended to the error buffer.
 */
static GLuint create_and_compile_shader(GLenum shader_type, const char* code, GLint code_size, FILE* error_stream) {
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, (const char*[]){ code }, (const int[]){ code_size });
	glCompileShader(shader);
	
	GLint result = GL_TRUE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result)
		return shader;
	
	if (error_stream) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &result);
		char buffer[result];
		glGetShaderInfoLog(shader, result, NULL, buffer);
		fprintf(error_stream, "Failed to compile shader:\n%s\n", buffer);
	}
	
	glDeleteShader(shader);
	return 0;
}

/**
 * Destorys the specified OpenGL program and all shaders attached to it.
 */
void program_destroy(GLuint program) {
	GLint shader_count = 0;
	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
	
	GLuint shaders[shader_count];
	glGetAttachedShaders(program, shader_count, NULL, shaders);
	
	glDeleteProgram(program);
	for(ssize_t i = 0; i < shader_count; i++)
		glDeleteShader(shaders[i]);
}

/**
 * Displays all attributes and uniforms of the OpenGL program on the output_stream.
 * If output_stream is NULL stderr will be used instead.
 */
void program_inspect(GLuint program, FILE* output_stream) {
	const char* type_to_string(GLenum type) {
		switch(type){
			case GL_FLOAT: return "float​";
			case GL_FLOAT_VEC2: return "vec2​";
			case GL_FLOAT_VEC3: return "vec3​";
			case GL_FLOAT_VEC4: return "vec4​";
			case GL_INT: return "int​";
			case GL_INT_VEC2: return "ivec2​";
			case GL_INT_VEC3: return "ivec3​";
			case GL_INT_VEC4: return "ivec4​";
			case GL_UNSIGNED_INT: return "unsigned int​";
			case GL_UNSIGNED_INT_VEC2: return "uvec2​";
			case GL_UNSIGNED_INT_VEC3: return "uvec3​";
			case GL_UNSIGNED_INT_VEC4: return "uvec4​";
			case GL_BOOL: return "bool​";
			case GL_BOOL_VEC2: return "bvec2​";
			case GL_BOOL_VEC3: return "bvec3​";
			case GL_BOOL_VEC4: return "bvec4​";
			case GL_FLOAT_MAT2: return "mat2​";
			case GL_FLOAT_MAT3: return "mat3​";
			case GL_FLOAT_MAT4: return "mat4​";
			case GL_FLOAT_MAT2x3: return "mat2x3​";
			case GL_FLOAT_MAT2x4: return "mat2x4​";
			case GL_FLOAT_MAT3x2: return "mat3x2​";
			case GL_FLOAT_MAT3x4: return "mat3x4​";
			case GL_FLOAT_MAT4x2: return "mat4x2​";
			case GL_FLOAT_MAT4x3: return "mat4x3​";
			case GL_SAMPLER_1D: return "sampler1D​";
			case GL_SAMPLER_2D: return "sampler2D​";
			case GL_SAMPLER_3D: return "sampler3D​";
			case GL_SAMPLER_CUBE: return "samplerCube​";
			case GL_SAMPLER_1D_SHADOW: return "sampler1DShadow​";
			case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow​";
			case GL_SAMPLER_1D_ARRAY: return "sampler1DArray​";
			case GL_SAMPLER_2D_ARRAY: return "sampler2DArray​";
			case GL_SAMPLER_1D_ARRAY_SHADOW: return "sampler1DArrayShadow​";
			case GL_SAMPLER_2D_ARRAY_SHADOW: return "sampler2DArrayShadow​";
			case GL_SAMPLER_2D_MULTISAMPLE: return "sampler2DMS​";
			case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: return "sampler2DMSArray​";
			case GL_SAMPLER_CUBE_SHADOW: return "samplerCubeShadow​";
			case GL_SAMPLER_BUFFER: return "samplerBuffer​";
			case GL_SAMPLER_2D_RECT: return "sampler2DRect​";
			case GL_SAMPLER_2D_RECT_SHADOW: return "sampler2DRectShadow​";
			case GL_INT_SAMPLER_1D: return "isampler1D​";
			case GL_INT_SAMPLER_2D: return "isampler2D​";
			case GL_INT_SAMPLER_3D: return "isampler3D​";
			case GL_INT_SAMPLER_CUBE: return "isamplerCube​";
			case GL_INT_SAMPLER_1D_ARRAY: return "isampler1DArray​";
			case GL_INT_SAMPLER_2D_ARRAY: return "isampler2DArray​";
			case GL_INT_SAMPLER_2D_MULTISAMPLE: return "isampler2DMS​";
			case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "isampler2DMSArray​";
			case GL_INT_SAMPLER_BUFFER: return "isamplerBuffer​";
			case GL_INT_SAMPLER_2D_RECT: return "isampler2DRect​";
			case GL_UNSIGNED_INT_SAMPLER_1D: return "usampler1D​";
			case GL_UNSIGNED_INT_SAMPLER_2D: return "usampler2D​";
			case GL_UNSIGNED_INT_SAMPLER_3D: return "usampler3D​";
			case GL_UNSIGNED_INT_SAMPLER_CUBE: return "usamplerCube​";
			case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return "usampler2DArray​";
			case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray​";
			case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return "usampler2DMS​";
			case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "usampler2DMSArray​";
			case GL_UNSIGNED_INT_SAMPLER_BUFFER: return "usamplerBuffer​";
			case GL_UNSIGNED_INT_SAMPLER_2D_RECT: return "usampler2DRect​";
			default: return "unknown";
		}
	}
	
	if (output_stream == NULL)
		output_stream = stderr;
	
	GLint size;
	GLenum type;
	{
		GLint attrib_count = 0, buffer_size = 0;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attrib_count);
		fprintf(output_stream, "%d attributes:\n", attrib_count);
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &buffer_size);
		char buffer[buffer_size];
		
		for(ssize_t i = 0; i < attrib_count; i++){
			glGetActiveAttrib(program, i, buffer_size, NULL, &size, &type, buffer);
			fprintf(output_stream, "- %s %s", buffer, type_to_string(type));
			if (size != 1)
				fprintf(output_stream, "[%d]", size);
			fprintf(output_stream, "\n");
		}
	}
	
	{
		GLint uniform_count = 0, buffer_size = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
		fprintf(output_stream, "%d uniforms:\n", uniform_count);
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &buffer_size);
		char buffer[buffer_size];
		
		for(ssize_t i = 0; i < uniform_count; i++){
			glGetActiveUniform(program, i, buffer_size, NULL, &size, &type, buffer);
			fprintf(output_stream, "- %s %s", buffer, type_to_string(type));
			if (size != 1)
				fprintf(output_stream, "[%d]", size);
			fprintf(output_stream, "\n");
		}
	}
}


//
// Buffer functions
//

/**
 * Creates a new vertex buffer with the specified size and initial data uploaded. The initial data
 * is uploaded with the GL_STATIC_DRAW usage, meant to be used for model data that does not change.
 * 
 * If `data` is `NULL` but a size is given the buffer will be allocated but no data is uploaded.
 * If `size` is `0` only the OpenGL object is created but nothing is allocated.
 * 
 * Returns the vertex buffer on success or `0` on error.
 */
GLuint buffer_new(const void* data, size_t size) {
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	if (buffer == 0)
		return 0;
	
	if (size > 0)
		buffer_update(buffer, data, size, GL_STATIC_DRAW);
	
	return buffer;
}

void buffer_destroy(GLuint buffer) {
	glDeleteBuffers(1, (const GLuint[]){ buffer });
}

/**
 * Updates the vertex buffer with new data. The `usage` parameter is the same as of the
 * `glBufferData()` function: GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW,
 * GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ or GL_DYNAMIC_COPY.
 */
void buffer_update(GLuint buffer, const void* data, size_t size, GLenum usage) {
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


//
// Texture functions
//

/**
 * Creates a normal 2D texture.
 * 
 * The textures minifing filter is set to GL_LINEAR_MIPMAP_LINEAR for better quality (default value
 * is GL_NEAREST_MIPMAP_LINEAR).
 */
GLuint texture_new(const void* data, size_t width, size_t height, uint8_t components) {
	GLenum internal_format = 0, data_format = 0;
	switch(components) {
		case 1: internal_format = GL_R8;    data_format = GL_RED;  break;
		case 2: internal_format = GL_RG8;   data_format = GL_RG;   break;
		case 3: internal_format = GL_RGB8;  data_format = GL_RGB;  break;
		case 4: internal_format = GL_RGBA8; data_format = GL_RGBA; break;
		default: return 0;
	}
	
	GLuint texture = 0;
	glGenTextures(1, &texture);
	if (texture == 0)
		return 0;
	
	GLsizei mipmap_levels = 1;
	size_t w = width, h = height;
	while (w > 1 || h > 1) {
		mipmap_levels++;
		w /= 2;
		h /= 2;
	}
	
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexStorage2D(GL_TEXTURE_2D, mipmap_levels, internal_format, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	if (data) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, data_format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	
	return texture;
}


/**
 * Creates and uploads a rectangular texture of the specified dimensions and with the specified
 * number of components (1, 2, 3 or 4). If data is NULL the texture will be allocated but no data
 * is uploaded.
 * 
 * To make the API easier this function only supports textures with 8 bits per pixel (GL_R8,
 * GL_RG8, GL_RGB8 and GL_RGBA8).
 * 
 * Returns the texture object on success or `0` on error.
 */
GLuint texture_new_rect(const void* data, size_t width, size_t height, uint8_t components) {
	GLenum internal_format = 0, data_format = 0;
	switch(components) {
		case 1: internal_format = GL_R8;    data_format = GL_RED;  break;
		case 2: internal_format = GL_RG8;   data_format = GL_RG;   break;
		case 3: internal_format = GL_RGB8;  data_format = GL_RGB;  break;
		case 4: internal_format = GL_RGBA8; data_format = GL_RGBA; break;
		default: return 0;
	}
	
	GLuint texture = 0;
	glGenTextures(1, &texture);
	if (texture == 0)
		return 0;
	
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);
	glTexStorage2D(GL_TEXTURE_RECTANGLE, 1, internal_format, width, height);
	if (data)
		glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, width, height, data_format, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	
	return texture;
}

void texture_destroy(GLuint texture) {
	glDeleteTextures(1, (const GLuint[]){ texture });
}

/**
 * Uploads new data for the specified texture. The data is expected to be as large as the entire
 * texture and to have the same number of components.
 */
void texture_update(GLuint texture, const void* data) {
	GLenum texture_type = GL_TEXTURE_2D;
	glBindTexture(texture_type, texture);
	if ( glGetError() == GL_INVALID_OPERATION ) {
		texture_type = GL_TEXTURE_RECTANGLE;
		glBindTexture(texture_type, texture);
	}
	
	GLint width = 0, height = 0, internal_format = 0;
	glGetTexLevelParameteriv(texture_type, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(texture_type, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(texture_type, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
	
	GLenum data_format = 0;
	switch(internal_format) {
		case GL_R8:    data_format = GL_RED;  break;
		case GL_RG8:   data_format = GL_RG;   break;
		case GL_RGB8:  data_format = GL_RGB;  break;
		case GL_RGBA8: data_format = GL_RGBA; break;
	}
	
	if (data_format != 0) {
		glTexSubImage2D(texture_type, 0, 0, 0, width, height, data_format, GL_UNSIGNED_BYTE, data);
		if (texture_type == GL_TEXTURE_2D)
			glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	glBindTexture(texture_type, 0);
}


//
// Frame buffer functions
//

GLuint framebuffer_new(GLuint color_buffer_texture) {
	GLint prev_draw_fb = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fb);
	
	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_buffer_texture, 0);
	
	if ( glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		glDeleteFramebuffers(1, &framebuffer);
		framebuffer = 0;
	}
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fb);
	return framebuffer;
}

void framebuffer_destroy(GLuint framebuffer) {
	glDeleteFramebuffers(1, &framebuffer);
}

/**
 * Note: Binds the read and draw framebuffers to GL_READ_FRAMEBUFFER and GL_DRAW_FRAMEBUFFER if they're
 * not already bound there. The bindings are left there so repeated calls don't need to bind them again.
 */
void framebuffer_blit(GLuint read_framebuffer, GLint rx, GLint ry, GLint rw, GLint rh, GLuint draw_framebuffer, GLint dx, GLint dy, GLint dw, GLint dh) {
	GLint read_fb = 0, draw_fb = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fb);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fb);
	
	if ((GLuint)read_fb != read_framebuffer)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, read_framebuffer);
	if ((GLuint)draw_fb != draw_framebuffer)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_framebuffer);
	
	glBlitFramebuffer(rx, ry, rx+rw, ry+rh, dx, dy, dx+dw, dy+dh, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void framebuffer_bind(GLuint framebuffer, GLsizei width, GLsizei height) {
	// Check framebuffer binding, and bind the target framebuffer if necessary
	GLint bound_framebuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_framebuffer);
	if ((GLuint)bound_framebuffer != framebuffer) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		if ( !gl_error("Failed to bind framebuffer %u. glBindFramebuffer()", framebuffer) && width != 0 && height != 0 )
			glViewport(0, 0, width, height);
	}
}


//
// Drawing functions
//

typedef struct {
	char name[128];
	char modifiers[16];
	uint8_t modifier_length;
	uint16_t type;
} directive_t, *directive_p;

typedef struct {
	GLenum opengl_type;
	GLint type_size, components;
	bool normalized, upload_as_int;
} attribute_info_t, *attribute_info_p;

static int next_directive(const char** bindings, directive_p output_directive);
static bool parse_attribute_directive(directive_p directive, attribute_info_p attribute_info);


/**
 * Form of directives: <attribute or uniform name> %<dimensions><modifiers><type>
 * e.g. "projection %4tM"
 * 
 * Uniforms (and textures): uppercase types
 * 
 * F (float)
 * 	%1F  glUniform1fv
 * 	%2F  glUniform2fv
 * 	%3F  glUniform3fv
 * 	%4F  glUniform4fv
 * I (integer)
 * 	%1I  glUniform1iv
 * 	%2I  glUniform2iv
 * 	%3I  glUniform3iv
 * 	%4I  glUniform4iv
 * U (unsigned integer)
 * 	%1U  glUniform1ui
 * 	%2U  glUniform2ui
 * 	%3U  glUniform3ui
 * 	%4U  glUniform4ui
 * M (matrix)
 * 	%2M    glUniformMatrix2fv
 * 	%2x3M  glUniformMatrix2x3fv
 * 	%2x4M  glUniformMatrix2x4fv
 * 	%3M    glUniformMatrix3fv
 * 	%3x2M  glUniformMatrix3x2fv
 * 	%3x4M  glUniformMatrix3x4fv
 * 	%4M    glUniformMatrix4fv
 * 	%4x2M  glUniformMatrix4x2fv
 * 	%4x3M  glUniformMatrix4x3fv
 * 	
 * 	Modifiers:
 * 		t  transpose matrix
 * 
 * T (textures)
 * 	%T  bind a GL_TEXTURE_2D texture to a sampler uniform
 * 	
 * 	Modifiers:
 * 		r  texture is a GL_TEXTURE_RECTANGLE texture
 * 
 * 
 * Attributes: lower case types
 * 	
 * 	The first attribute consumes one argument that has to be an OpenGL buffer object.
 * 	This buffer is used for all following attributes or until an ";" is encountered.
 * 	The next attribute after ";" consumes a new buffer argument.
 * 	Stride and offsets of attributes are calculated automatically.
 * 	The attribute name "_" is used for padding.
 * 
 * 	Dimensions: 1, 2, 3, 4
 * 
 * f  GL_FLOAT
 * 	h  GL_HALF_FLOAT
 * 	f  GL_FIXED
 * b  GL_BYTE
 * 	u  GL_UNSIGNED_BYTE
 * 	n  normalized
 * 	i  upload as int (use glVertexAttribIPointer())
 * 
 * s  GL_SHORT
 * 	u  GL_UNSIGNED_SHORT
 * 	n  normalized
 * 	i  upload as int (use glVertexAttribIPointer())
 * 
 * i  GL_INT
 * 	u  GL_UNSIGNED_INT
 * 	n  normalized
 * 	i  upload as int (use glVertexAttribIPointer())
 * 
 * Global options: start with "$" instead of "%"
 * 
 * 	$I  draw with an index buffer of GL_UNSIGNED_INT indices
 *    b  indices are of type GL_UNSIGNED_BYTE
 *    s  indices are of type GL_UNSIGNED_SHORT
 * 
 * Ideas (not yet implemented):
 * 
 * 	$F  render into that frame buffer
 * 	$E  output error messages into that FILE* stream
 */
int render(GLenum primitive, GLuint program, const char* bindings, ...) {
	va_list args;
	directive_t d;
	attribute_info_t ai;
	size_t active_textures = 0;
	GLsizei current_buffer_stride = 0;
	size_t current_buffer_offset = 0;
	GLint current_buffer_size = 0;
	uint32_t vertecies_to_render = UINT32_MAX;
	bool use_index_buffer = false;
	GLenum index_buffer_type = 0;
	uint32_t indices_to_render = 0;
	//GLuint use_framebuffer = 0;
	
	// Make sure no previous error code messes up our state
	glGetError();
	
	GLuint vertex_array_object = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vertex_array_object);
	if (vertex_array_object == 0) {
		glGenVertexArrays(1, &vertex_array_object);
		glBindVertexArray(vertex_array_object);
		gl_error("Failed to generate and bind a new vertex array object. glBindVertexArray()");
	}
	
	glUseProgram(program);
	if ( gl_error("Can't use OpenGL program for drawing. glUseProgram()") )
		return -1;
	
	va_start(args, bindings);
	const char* setup_pass_bindings = bindings;
	while( next_directive(&setup_pass_bindings, &d) ) {
		if (d.type == ';') {
			// User no longer wants to use the current buffer for attributes. So reset all the buffer
			// stuff. If a new attribute directive comes around it will consume the next buffer.
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			gl_error("Failed to unbind vertex buffer. glBindBuffer(GL_ARRAY_BUFFER)");
			current_buffer_stride = 0;
			current_buffer_offset = 0;
			current_buffer_size = 0;
			continue;
		} else if (d.type == 'I' + 256) {
			// User wants to draw with an index buffer, consume an argument and see which type the indices have
			GLuint index_buffer = va_arg(args, GLuint);
			index_buffer_type = GL_UNSIGNED_INT;
			size_t index_type_size = sizeof(GLuint);
			
			for(char* m = d.modifiers; *m != '\0'; m++) {
				switch(*m) {
					case 'b': index_buffer_type = GL_UNSIGNED_BYTE;  index_type_size = sizeof(GLubyte);  break;
					case 's': index_buffer_type = GL_UNSIGNED_SHORT; index_type_size = sizeof(GLushort); break;
					default:  fprintf(stderr, "Invalid index buffer directive $%s%c\n", d.modifiers, d.type - 256);
				}
			}
			
			// Got a vailid type, so bind the index buffer and figure out how many indices are in there
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
			if ( ! gl_error("Unable to bind index buffer. glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)") ) {
				int index_buffer_size = 0;
				glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &index_buffer_size);
				if ( !gl_error("Unable to determine size of index buffer. glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE)") )
					indices_to_render = index_buffer_size / index_type_size;
			}
			
			use_index_buffer = true;
			continue;
		//} else if (d.type == 'F' + 256) {
		//	// User wants to draw into a framebuffer. Consume an argument and use it as framebuffer.
		//	use_framebuffer = va_arg(args, GLuint);
		//	continue;
		} else if (d.type > 256) {
			// Got an unknown global option
			fprintf(stderr, "Unknown global option: $%s%c. Ignoring but consuming one argument.\n", d.modifiers, d.type - 256);
			va_arg(args, GLuint);
			continue;
		}
		
		GLint location = 0;
		if (isupper(d.type)) {
			// Upper case types are uniforms
			location = glGetUniformLocation(program, d.name);
			if ( gl_error("Error on looking up uniform %s. glGetUniformLocation()", d.name) ) {
				// All glGetUniformLocation errors are caused by invalid programs. So no point in
				// trying anything else since the program is broken.
				return -1;
			} else if (location == -1) {
				fprintf(stderr, "Program has no uniform \"%s\", ignoring uniform.\n", d.name);
				continue;
			}
		} else {
			// Lower case types are attributes
			if (d.name[0] == '_' && d.name[1] == '\0') {
				// The attribute name "_" is used for padding, so don't try to look it up. Just use it
				// for offset and stride calculations.
				location = -1;
			} else {
				location = glGetAttribLocation(program, d.name);
				if ( gl_error("Error on looking up attribute %s. glGetAttribLocation()", d.name) ) {
					// All glGetAttribLocation errors are caused by invalid programs. So no point in
					// trying anything else since the program is broken.
					return -1;
				} else if (location == -1) {
					fprintf(stderr, "Program has no attribute \"%s\", attribute unused and it's space will be skipped in the buffer.\n", d.name);
				}
			}
			
			// We e don't know the stride of the attribute we're going to bind. So sum the size of the
			// current attribute and all further attributes that use this buffer (so all attributes left
			// or until we encounter a buffer reset ";").
			// When we know how large one vertex will be in this buffer we know the stride (vertex size)
			// and how many vertecies are in the buffer.
			if (current_buffer_stride == 0) {
				attribute_info_t ai;
				if ( parse_attribute_directive(&d, &ai) )
					current_buffer_stride += ai.type_size * ai.components;
				
				directive_t ld;
				const char* lookahead_bindings = setup_pass_bindings;
				while( next_directive(&lookahead_bindings, &ld), ld.type && ld.type != ';' ) {
					// Skip uniforms and global parameters
					if (ld.type > 256 || isupper(ld.type))
						continue;
					
					if ( parse_attribute_directive(&ld, &ai) )
						current_buffer_stride += ai.type_size * ai.components;
				}
				
				// Consume an argument, bind it as array buffer and determine the number of vertecies in
				// there (only needed when we don't use an index buffer for rendering).
				glBindBuffer(GL_ARRAY_BUFFER, va_arg(args, GLuint));
				if ( ! gl_error("Unable to bind vertex buffer at attribute %s. glBindBuffer(GL_ARRAY_BUFFER)", d.name) && !use_index_buffer ) {
					glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_buffer_size);
					// See how many vertecies are in that buffer. In the end we want to render as many
					// vertecies as are present in all buffers.
					if ( !gl_error("Unable to determine buffer size at attribute %s. glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE)", d.name) && current_buffer_size > 0 && current_buffer_stride > 0) {
						uint32_t vertecies_in_buffer = current_buffer_size / current_buffer_stride;
						if (vertecies_in_buffer < vertecies_to_render)
							vertecies_to_render = vertecies_in_buffer;
					}
					//printf("using buffer: size %d, stride %d, vertecies_to_render: %u\n", current_buffer_size, current_buffer_stride, vertecies_to_render);
				}
			}
		}
		
		switch(d.type) {
			// Uniforms
			case 'F':
				if (d.modifier_length != 1) break;
				switch(d.modifiers[0]) {
					case '1': glUniform1fv(location, 1, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniform1fv()", d.name); continue;
					case '2': glUniform2fv(location, 1, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniform2fv()", d.name); continue;
					case '3': glUniform3fv(location, 1, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniform3fv()", d.name); continue;
					case '4': glUniform4fv(location, 1, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniform4fv()", d.name); continue;
				}
				break;
			case 'I':
				if (d.modifier_length != 1) break;
				switch(d.modifiers[0]) {
					case '1': glUniform1iv(location, 1, va_arg(args, GLint*)); gl_error("Failed to set uniform %s. glUniform1iv()", d.name); continue;
					case '2': glUniform2iv(location, 1, va_arg(args, GLint*)); gl_error("Failed to set uniform %s. glUniform2iv()", d.name); continue;
					case '3': glUniform3iv(location, 1, va_arg(args, GLint*)); gl_error("Failed to set uniform %s. glUniform3iv()", d.name); continue;
					case '4': glUniform4iv(location, 1, va_arg(args, GLint*)); gl_error("Failed to set uniform %s. glUniform4iv()", d.name); continue;
				}
				break;
			case 'U':
				if (d.modifier_length != 1) break;
				switch(d.modifiers[0]) {
					case '1': glUniform1uiv(location, 1, va_arg(args, GLuint*)); gl_error("Failed to set uniform %s. glUniform1uiv()", d.name); continue;
					case '2': glUniform2uiv(location, 1, va_arg(args, GLuint*)); gl_error("Failed to set uniform %s. glUniform2uiv()", d.name); continue;
					case '3': glUniform3uiv(location, 1, va_arg(args, GLuint*)); gl_error("Failed to set uniform %s. glUniform3uiv()", d.name); continue;
					case '4': glUniform4uiv(location, 1, va_arg(args, GLuint*)); gl_error("Failed to set uniform %s. glUniform4uiv()", d.name); continue;
				}
				break;
			case 'M': {
				GLboolean transpose = false;
				
				// Skip the 3 byte (e.g. "3x2") or 1 byte (e.g. "2") matrix dimensions and loop over all remaining modifiers.
				// Skip right over everything if we got an invalid modifier so the user gets an error message about it.
				for(char* m = (d.modifiers[1] == 'x') ? d.modifiers + 3 : d.modifiers + 1; *m != '\0'; m++) {
					switch(*m) {
						case 't': transpose = true; break;
						default: goto invalid_matrix_modifier;
					}
				}
				
				switch(d.modifiers[0]) {
					case '2':
						if (d.modifiers[1] == 'x') {
							if (d.modifiers[2] == '3')      { glUniformMatrix2x3fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix2x3fv()", d.name); continue; }
							else if (d.modifiers[2] == '4') { glUniformMatrix2x4fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix2x4fv()", d.name); continue; }
						} else {
							glUniformMatrix2fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix2fv()", d.name); continue;
						}
						break;
					case '3':
						if (d.modifiers[1] == 'x') {
							if (d.modifiers[2] == '2')      { glUniformMatrix3x2fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix3x2fv()", d.name); continue; }
							else if (d.modifiers[2] == '4') { glUniformMatrix3x4fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix3x4fv()", d.name); continue; }
						} else {
							glUniformMatrix3fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix3fv()", d.name); continue;
						}
						break;
					case '4':
						if (d.modifiers[1] == 'x') {
							if (d.modifiers[2] == '2')      { glUniformMatrix4x2fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix4x2fv()", d.name); continue; }
							else if (d.modifiers[2] == '3') { glUniformMatrix4x3fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix4x3fv()", d.name); continue; }
						} else {
							glUniformMatrix4fv(location, 1, transpose, va_arg(args, GLfloat*)); gl_error("Failed to set uniform %s. glUniformMatrix4fv()", d.name); continue;
						}
						break;
				}
				
				}
				invalid_matrix_modifier:
				break;
			
			// Textures, only increment active_textures when the texture can be used successfully. Otherwise
			// we try to reuse the current texture image unit for the next texture.
			case 'T': {
				GLenum target = GL_TEXTURE_2D;
				
				for(char* m = d.modifiers; *m != '\0'; m++) {
					switch(*m) {
						case 'r': target = GL_TEXTURE_RECTANGLE; break;
						default: goto invalid_texture_modifier;
					}
				}
				
				glActiveTexture(GL_TEXTURE0 + active_textures);
				if ( gl_error("Failed to activate texture image unit %d for texture %s. Probably to many textures. glActiveTexture()", active_textures, d.name) )
					continue;
				glBindTexture(target, va_arg(args, GLint));
				if ( gl_error("Failed to bind texture for %s to %s. glBindTexture()", d.name, (target == GL_TEXTURE_2D) ? "GL_TEXTURE_2D" : "GL_TEXTURE_RECTANGLE") )
					continue;
				glUniform1i(location, active_textures);
				if ( gl_error("Failed to set uniform for texture %s. glUniform1i()", d.name) ) {
					glBindTexture(target, 0);
					continue;
				}
				
				active_textures++;
				continue;
				
				}
				invalid_texture_modifier:
				break;
			
			// Attributes
			default:
				// Don't process unknown attributes (would just lead to errors) and ignore padding attributes
				// TODO: Check if this actually skipps padding attributes in the offset calculation
				if ( location == -1 )
					continue;
				
				if ( parse_attribute_directive(&d, &ai) ) {
					// Make sure that we count the attributes size in future offsets. So the buffer layout is the
					// same even if we fail to use some attributes.
					size_t offset = current_buffer_offset;
					current_buffer_offset += ai.type_size * ai.components;
					
					glEnableVertexAttribArray(location);
					if (gl_error("Failed to enable vertex attribute %s. glEnableVertexAttribArray()", d.name))
						continue;
					
					if (ai.upload_as_int)
						glVertexAttribIPointer(location, ai.components, ai.opengl_type, current_buffer_stride, (GLvoid*)offset);
					else
						glVertexAttribPointer(location, ai.components, ai.opengl_type, ai.normalized, current_buffer_stride, (GLvoid*)offset);
					
					gl_error("Failed to setup buffer layout for attribute %s. %s()", d.name, ai.upload_as_int ? "glVertexAttribIPointer" : "glVertexAttribPointer");
					continue;
				}
				break;
		}
		
		// We'll only arrive down here if the type is invalid so print out an error message
		fprintf(stderr, "Invalid type %%%s%c for %s\n", d.modifiers, d.type, d.name);
	}
	va_end(args);
	
	/*
	// Check framebuffer binding, and bind the target framebuffer if necessary
	GLint bound_framebuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_framebuffer);
	if ((GLuint)bound_framebuffer != use_framebuffer) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, use_framebuffer);
		// TODO: Figure out how large the frame buffer is and set glViewport accordingly
	}
	*/
	
	// Draw stuff, start of the fireworks... finally
	if ( ! use_index_buffer ) {
		glDrawArrays(primitive, 0, vertecies_to_render);
		gl_error("Drawcall failed. glDrawArrays()");
	} else {
		glDrawElements(primitive, indices_to_render, index_buffer_type, 0);
		gl_error("Drawcall failed. glDrawElements()");
	}
	
	
	// Cleanup all texture image units we bound textures to
	for(size_t i = 0; i < active_textures; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	}
	glActiveTexture(GL_TEXTURE0);
	
	// Unbind any buffer that has been used by the last attribute
	if (current_buffer_stride != 0)
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// Disable all vertex attribute arrays
	const char* cleanup_bindings = bindings;
	while( next_directive(&cleanup_bindings, &d) ) {
		// Skip uniforms, padding attributes and global options
		if (isupper(d.type) || (d.name[0] == '_' && d.name[1] == '\0') || d.type > 256 )
			continue;
		
		// Skip unknown attributes
		GLint location = glGetAttribLocation(program, d.name);
		if (location == -1)
			continue;
		
		glDisableVertexAttribArray(location);
	}
	
	glUseProgram(0);
	
	return 0;
}

/**
 * Helper function for `draw()`. Parses the next directive in `*bindings`. Returns the type of
 * the next directive or 0 if there is an error or no next directive. Additional data about the
 * directive is returned in `output_directive`. `*bindings` is advanced so it points to the end
 * of the parsed directive.
 * 
 * Whitespaces and commas at the beginning of `*bindings` are ignored. In case of an parser error
 * a message is printed to stderr. To iterate over all directives in a string use:
 * 
 *   const char* bindings = ...;
 *   directive_t directive;
 *   while( next_directive(&bindings, &directive) ) {
 *     // ...
 *   }
 */
static int next_directive(const char** bindings, directive_p output_directive) {
	int consumed_bytes = 0, matched_items = 0;
	
	// Remember start for error messages
	const char** start = bindings;
	
	// Reset output values
	output_directive->name[0] = '\0';
	output_directive->modifiers[0] = '\0';
	output_directive->modifier_length = 0;
	output_directive->type = '\0';
	
	// Consume spaces and comma signs
	while ( *bindings[0] != '\0' && (isspace(*bindings[0]) || *bindings[0] == ',') )
		*bindings += 1;
	
	switch(*bindings[0]) {
		case ';':
			// Got a reset buffer directive
			*bindings += 1;
			output_directive->type = ';';
			return output_directive->type;
		case '%':
			// Got the type of a uniform or attribute directive. But this is invalid since
			// we need a name first. Probably an easily made error so we print an extra
			// error message.
			fprintf(stderr, "Missing name before uniform or attribute directive \"%s\"\n", *start);
			return 0;
		case '$':
			// Got a global option directive, type is the letter of the type + 256
			output_directive->type += 256;
			matched_items = sscanf(*bindings, "$%15s%n", output_directive->modifiers, &consumed_bytes);
			if (matched_items == EOF)
				return 0;
			if (matched_items < 1) {
				fprintf(stderr, "Failed to parse global option directive \"%s\"\n", *start);
				return 0;
			}
			*bindings += consumed_bytes;
			break;
		default:
			matched_items = sscanf(*bindings, "%127s %%%15s%n", output_directive->name, output_directive->modifiers, &consumed_bytes);
			if (matched_items == EOF)
				return 0;
			if (matched_items < 2 ) {
				fprintf(stderr, "Failed to parse uniform or attribute directive \"%s\"\n", *start);
				return 0;
			}
			*bindings += consumed_bytes;
			break;
	}
		
	// Move the last letter of the modifiers to the type field.
	// Only add the type to output_directive->type since global options already have a 256 offset there. 
	output_directive->modifier_length = strlen(output_directive->modifiers) - 1;
	output_directive->type += output_directive->modifiers[output_directive->modifier_length];
	output_directive->modifiers[output_directive->modifier_length] = '\0';
	return output_directive->type;
}


/**
 * Tries to parses `directive` as an attribute directive. Output data like the OpenGL data type,
 * component count, etc. are stored in `attribute_info`. Returns `true` if the directive is a
 * attribute directive, `false` otherwise.
 */
static bool parse_attribute_directive(directive_p directive, attribute_info_p attribute_info) {
	attribute_info_p ai = attribute_info;
	memset(ai, 0, sizeof(attribute_info_t));
	char* m = directive->modifiers;
	
	// Take care of the component count
	switch(*(m++)) {
		case '1': ai->components = 1; break;
		case '2': ai->components = 2; break;
		case '3': ai->components = 3; break;
		case '4': ai->components = 4; break;
		default: return false;
	}
	
	switch(directive->type) {
		case 'f':
			ai->opengl_type = GL_FLOAT;
			ai->type_size = sizeof(GLfloat);
			
			for( ; *m != '\0'; m++) {
				switch(*m) {
					case 'h': ai->opengl_type = GL_HALF_FLOAT; ai->type_size = sizeof(GLhalf);  break;
					case 'f': ai->opengl_type = GL_FIXED;      ai->type_size = sizeof(GLfixed); break;
					default: return false;
				}
			}
			
			return true;
			
		case 'b':
			ai->opengl_type = GL_BYTE;
			ai->type_size = sizeof(GLbyte);
			
			for( ; *m != '\0'; m++) {
				switch(*m) {
					case 'u': ai->opengl_type = GL_UNSIGNED_BYTE; ai->type_size = sizeof(GLubyte); break;
					case 'n': ai->normalized = true;     break;
					case 'i': ai->upload_as_int = true;  break;
					default: return false;
				}
			}
			
			return true;
			
		case 's':
			ai->opengl_type = GL_SHORT;
			ai->type_size = sizeof(GLshort);
			
			for( ; *m != '\0'; m++) {
				switch(*m) {
					case 'u': ai->opengl_type = GL_UNSIGNED_SHORT; ai->type_size = sizeof(GLushort); break;
					case 'n': ai->normalized = true;     break;
					case 'i': ai->upload_as_int = true;  break;
					default: return false;
				}
			}
			
			return true;
		
		case 'i':
			ai->opengl_type = GL_INT;
			ai->type_size = sizeof(GLint);
			
			for( ; *m != '\0'; m++) {
				switch(*m) {
					case 'u': ai->opengl_type = GL_UNSIGNED_INT; ai->type_size = sizeof(GLuint); break;
					case 'n': ai->normalized = true;     break;
					case 'i': ai->upload_as_int = true;  break;
					default: return false;
				}
			}
			
			return true;
	}
	
	// Unknown or unsupported attribute type
	return false;
}

/**
 * If there is a last OpenGL error this function returns `true` and prints `description` followed
 * by ": " and the last pending OpenGL error(s) to stderr (a bit like `perror()`). Otherwise `false`
 * is returned.
 * 
 * The `glGetError()` docs say that it might hold multiple pending errors. So before using this function
 * you should make sure to consume all pending GL errors by looping over `glGetError()` until it returns
 * `GL_NO_ERROR​`.
 * 
 * `gl_error()` behaves like `printf()` so you can use stuff like %s in `directive`
 * to print additional error information.
 */
static bool gl_error(const char* description, ...) {
	GLenum error = glGetError();
	if (error == GL_NO_ERROR)
		return false;
	
	va_list args;
	va_start(args, description);
	vfprintf(stderr, description, args);
	va_end(args);
	
	const char* gl_error_message = NULL;
	switch(error) {
		case GL_INVALID_ENUM:                  gl_error_message = "invalid enum";                  break;
		case GL_INVALID_VALUE:                 gl_error_message = "invalid value";                 break;
		case GL_INVALID_OPERATION:             gl_error_message = "invalid operation";             break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: gl_error_message = "invalid framebuffer operation"; break;
		case GL_OUT_OF_MEMORY:                 gl_error_message = "out of memory";                 break;
		case GL_STACK_UNDERFLOW:               gl_error_message = "stack underflow";               break;
		case GL_STACK_OVERFLOW:                gl_error_message = "stack overflow";                break;
		default:                               gl_error_message = "unknown OpenGL error";          break;
	}
	
	fprintf(stderr, ": %s\n", gl_error_message);
	return true;
}


//
// Utility functions
//

/**
 * Checks if an OpenGL extention is avaialbe.
 */
static bool gl_ext_present(const char *ext_name){
	GLint ext_count;
	glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
	for(ssize_t i = 0; i < ext_count; i++){
		if ( strcmp((const char*)glGetStringi(GL_EXTENSIONS, i), ext_name) == 0 )
			return true;
	}
	return false;
}

/**
 * The library requires some OpenGL extentions. This function checks if these are available. If not
 * an error message is printed to stderr for each missing extention.
 * 
 * Returns whether the requirements are met or not.
 */
bool check_required_gl_extentions(){
	const char* extentions[] = {
		// Both extentions are required for texture format and handling
		"GL_ARB_texture_rectangle",
		"GL_ARB_texture_storage",
		NULL
	};
	
	bool requirements_met = true;
	for(size_t i = 0; extentions[i] != NULL; i++){
		if ( !gl_ext_present(extentions[i]) ) {
			requirements_met = false;
			fprintf(stderr, "Required OpenGL extention not available: %s\n", extentions[i]);
		}
	}
	
	return requirements_met;
}


//
// Utility functions
//

/**
 * Platform indipendent function to read an entire file into memory.
 * 
 * Returns a pointer to the zero terminated malloced contents of the file. If size is
 * not NULL it's target is set to the size of the file not including the zero terminator
 * at the end of the memory block.
 * 
 * On error NULL is returned and errno is set accordingly.
 */
void* fload(const char* filename, size_t* size) {
	long filesize = 0;
	char* data = NULL;
	
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	
	if ( fseek(f, 0, SEEK_END)              == -1       ) goto fail;
	if ( (filesize = ftell(f))              == -1       ) goto fail;
	if ( fseek(f, 0, SEEK_SET)              == -1       ) goto fail;
	if ( (data = malloc(filesize + 1))      == NULL     ) goto fail;
	if ( (long)fread(data, 1, filesize, f)  != filesize ) goto free_and_fail;
	fclose(f);
	
	data[filesize] = '\0';
	if (size)
		*size = filesize;
	return (void*)data;
	
	
	int error = -1;
	
	free_and_fail:
		error = errno;
		free(data);
	
	fail:
		if (error == -1)
			error = errno;
		fclose(f);
	
	errno = error;
	return NULL;
}