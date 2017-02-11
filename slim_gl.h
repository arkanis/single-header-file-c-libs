/**

SlimGL v1.0.0 - Compact OpenGL shorthand functions for common cases


Do this:

#define SLIM_GL_IMPLEMENTATION

before you include this file in *one* C file to create the implementation.

// i.e. it should look like this:
#include ...
#include ...
#include ...
#define SLIM_GL_IMPLEMENTATION
#include "slim_gl.h"


Core ideas:

- Single header file library (like stb_image.h).
- A simple API as shortcuts for the most common cases.
- Can be combined with direct OpenGL calls for more complex code (SlimGL functions return native OpenGL object IDs).
- Emphasis on ease of use, not so much on performance.
- Use a printf()/scanf() style API for uniform and attribute setup to make drawcalls more compact and easier to read.

Right now the API covers the following:

- OpenGL programs (vertex and fragment shaders): sgl_program_from_files(), sgl_program_from_strings(), sgl_program_destroy()
  sgl_program_inspect() and the SGL_GLSL() macro.
- Drawcalls that draw the complete vertex buffer, uniform and vertex attribute setup: sgl_draw().
- Textures: sgl_texture_new(), sgl_texture_destroy(), sgl_texture_update(), sgl_texture_update_sub() and sgl_texture_dimensions().
- Framebuffers with just one color attachment: sgl_framebuffer_new(), sgl_framebuffer_destroy() and sgl_framebuffer_bind().
- Some useful utilities: sgl_error(), sgl_fload() and sgl_strappendf().

Example code to render a white triangle on black background:

	// Compile vertex and fragment shaders into an OpenGL program
	GLuint program = sgl_program_from_strings(SGL_GLSL("#version 140",
		in vec2 pos;
		void main() {
			gl_Position = vec4(pos, 0, 1);
		}
	), SGL_GLSL("#version 140",
		void main() {
			gl_FragColor = vec4(1);
		}
	), NULL);
	
	// Create a vertex buffer with one triangle in it
	struct { float x, y; } vertices[] = {
		{    0,  0.5 },  // top
		{  0.5, -0.5 },  // right
		{ -0.5, -0.5 }   // left
	};
	GLuint buffer = sgl_buffer_new(vertices, sizeof(vertices));
	
	// Draw background and triangle
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	sgl_draw(GL_TRIANGLES, program, "pos %2f", buffer);

For more details please take a look at the documentation of the individual functions.

Revision history:

  v1.0.0 (2015-09-22)   First released version

License: MIT License

*/


#ifndef SLIM_GL_HEADER
#define SLIM_GL_HEADER

#include <stdint.h>

//
// OpenGL program functions
//

GLuint sgl_program_new(const char* args, ...);

/**
 * Compiles a vertex and fragment shader from two files, links them into an OpenGL program reports compiler errors on failure.
 * 
 *   // Without error reporting
 *   GLuint object_prog = sgl_program_from_files("object.vs", "object.fs", NULL);
 *   
 *   // With error reporting
 *   char* compiler_errors = NULL;
 *   GLuint object_prog = sgl_program_from_files("object.vs", "object.fs", &compiler_errors);
 *   if (!object_prog) {
 *     printf("Failed to compile object shaders:\n%s", compiler_errors);
 *     free(compiler_errors);
 *     compiler_errors = NULL;
 *   }
 * 
 * On success the OpenGL program ID is returned. On failure `0` is returned and compiler errors are reported.
 * If `compiler_errors` is `NULL` the errors are printed to `stderr`. Otherwise it's target is set to a string
 * containing the compiler messages. That string has to be `free()`ed by the caller. On error incomplete
 * shader and program objects are deleted.
 * 
 * Changed OpenGL state: None.
 */
GLuint sgl_program_from_files(const char* vertex_shader_file, const char* fragment_shader_file, char** compiler_errors);

/**
 * Same as `sgl_program_from_files()` but loads the shader code from strings instead of files.
 */
GLuint sgl_program_from_strings(const char* vertex_shader_code, const char* fragment_shader_code, char** compiler_errors);

/**
 * Destroys the OpenGL program and all shaders attached to it.
 * 
 * Changed OpenGL state: None.
 */
void sgl_program_destroy(GLuint program);

/**
 * Prints all attributes and uniforms of the OpenGL program on stderr.
 * 
 * Changed OpenGL state: None.
 */
void sgl_program_inspect(GLuint program);

/**
 * A small macro that can be used to write strings containing GLSL code like normal C. Instead of
 * 
 *   "#version 130\n"
 *   "in vec2 pos;\n"
 *   "void main() {\n"
 *   "	gl_Position = vec4(pos, 0, 1);\n"
 *   "}\n"
 * 
 * you can write this
 * 
 *   SGL_GLSL("#version 130",
 *   	in vec2 pos;
 *   	void main() {
 *   		gl_Position = vec4(pos, 0, 1);
 *   	}
 *   )
 * 
 * All the macro does is to convert the GLSL code to a string and add it to the preprocessor directives
 * ("#version 130" in the example above). This way you don't have to take care of the quotes yourself
 * and syntax-highlighting works for the GLSL code. For the macro to work properly all commas in the GLSL
 * code have to be enclosed in brackets. Otherwise the different parts of the code are interpreted as
 * multiple arguments to the macro:
 * 
 *   SGL_GLSL("#version 130",
 *   	in vec2 pos, focus;  // <-- breaks, use two declarations instead
 *   	void main() {
 *   		vec a, b, c;  // <-- breaks, again, use multiple declarations
 *   		gl_Position = vec4(pos, 0, 1);  // <-- ok since commas are surrounded by brackets
 *   	}
 *   )
 * 
 * GLSL preprocessor directives have to be inside the first string argument or the C preprocessor would
 * already interpret them. Note that without a #version directive GLSL version 1.10 is assumed (OpenGL 2.0).
 * So you should always define the version you want (e.g. "#version 130" for GLSL 1.30 which is part of
 * OpenGL 3.0).
 * 
 * Inspired by the examples of the Chipmunk2D physics library.
 */
#define SGL_GLSL(preproc_directives, code) preproc_directives "\n" #code


//
// Buffer functions
//

/**
 * Creates a new vertex buffer with the specified size and initial data uploaded. The initial data
 * is uploaded with the `GL_STATIC_DRAW` usage, meant to be used for model data that does not change.
 * 
 * If `data` is `NULL` but a size is given the buffer will be allocated but no data is uploaded.
 * If `size` is `0` only the OpenGL object is created but nothing is allocated.
 * 
 * Returns the vertex buffer on success or `0` on error.
 * 
 * Changed OpenGL state: If data is uploaded GL_ARRAY_BUFFER binding is reset to 0.
 */
GLuint sgl_buffer_new(const void* data, size_t size);

/**
 * Destroys the buffer object.
 * 
 * Changed OpenGL state: None.
 */
void sgl_buffer_destroy(GLuint buffer);

/**
 * Updates the vertex buffer with new data. The `usage` parameter is the same as of the
 * `glBufferData()` function:
 * GL_STREAM_DRAW,  GL_STREAM_READ,  GL_STREAM_COPY,
 * GL_STATIC_DRAW,  GL_STATIC_READ,  GL_STATIC_COPY,
 * GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY.
 * 
 * Changed OpenGL state: GL_ARRAY_BUFFER binding is reset to 0.
 */
void sgl_buffer_update(GLuint buffer, const void* data, size_t size, GLenum usage);



//
// Textures
//

#define SGL_RECT          (1 << 0)
#define SGL_SKIP_MIPMAPS  (1 << 1)

/**
 * Creates and optionally uploads a 2D or rectangle texture with the specified number of components (1, 2, 3 or 4).
 * If `data` is `NULL` the texture will be allocated but no data is uploaded. If `stride_in_pixels` is `0` the `width`
 * is used as stride.
 * 
 * By default a GL_TEXTURE_2D with mipmaps is created and the minifing filter is set to GL_LINEAR_MIPMAP_LINEAR (better
 * quality). If data is uploaded the mipmaps are generated as well. You can use SGL_SKIP_MIPMAPS to skip mipmap
 * generation (but the levels are still allocated, you have to fill them later on).
 * 
 * To make the API easier this function only supports textures with 8 bits per pixel (GL_R8, GL_RG8, GL_RGB8 and GL_RGBA8).
 * 
 * Flags:
 * 
 * - SGL_RECT: Create a GL_TEXTURE_RECTANGLE instead of a GL_TEXTURE_2D. Rectangle textures don't have mipmaps.
 * - SGL_SKIP_MIPMAPS: Skip `glGenerateMipmap()` call after uploading data for a GL_TEXTURE_2D. For example if you want
 *                     to upload many small images into the texture and call `glGenerateMipmap()` after that.
 * 
 * Returns the texture object on success or `0` on error.
 * 
 * Changed OpenGL state: None.
 */
GLuint sgl_texture_new(uint32_t width, uint32_t height, uint8_t components, const void* data, size_t stride_in_pixels, uint32_t flags);

/**
 * Destroys the texture object.
 * 
 * Changed OpenGL state: None.
 */
void sgl_texture_destroy(GLuint texture);

/**
 * Uploads new data for the specified texture. The data is expected to be as large as the entire texture and to have the
 * same number of components as the texture.  If `stride_in_pixels` is `0` the textures width is used as stride.
 * 
 * Flags:
 * 
 * - SGL_RECT: Texture is a GL_TEXTURE_RECTANGLE.
 * - SGL_SKIP_MIPMAPS: Skip calling `glGenerateMipmap()` after upload.
 * 
 * Changed OpenGL state: None.
 */
void sgl_texture_update(GLuint texture, const void* data, size_t stride_in_pixels, uint32_t flags);

/**
 * Uploads new data for a part of a texture. The data is expected to have the same number of components. If `w` or `h`
 * is `0` they are set to the remaining width or height of the texture. If `stride_in_pixels` is `0` it's assumed to be
 * the same as `w` (or the remaining width if `w` is `0`).
 * 
 * Flags:
 * 
 * - SGL_RECT: Texture is a GL_TEXTURE_RECTANGLE.
 * - SGL_SKIP_MIPMAPS: Skip calling `glGenerateMipmap()` after upload.
 * 
 * Changed OpenGL state: None.
 */
void sgl_texture_update_sub(GLuint texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void* data, size_t stride_in_pixels, uint32_t flags);

/**
 * Returns the texutre's dimensions in `width` and `height`. A `width` or `height` of `NULL` is ignored and safe to use.
 * 
 * Flags:
 * 
 * - SGL_RECT: Texture is a GL_TEXTURE_RECTANGLE.
 * 
 * Changed OpenGL state: None.
 * Temporarily changed and restored OpenGL state: GL_TEXTURE_BINDING_2D or GL_TEXTURE_BINDING_RECTANGLE (if the SGL_RECT flag is set).
 * 
 * Changed OpenGL state: None.
 */
void sgl_texture_dimensions(GLuint texture, uint32_t* width, uint32_t* height, uint32_t flags);



//
// Frame buffer functions
//

/**
 * Creates a framebuffer with `color_buffer_texture` as its color butter attachtment.
 * 
 * Flags:
 * 
 * - SGL_RECT: `color_buffer_texture` is a GL_TEXTURE_RECTANGLE instead of a GL_TEXTURE_2D.
 * 
 * Returns the framebuffer object on success or `0` on error.
 * 
 * Changed OpenGL state: None.
 */
GLuint sgl_framebuffer_new(GLuint color_buffer_texture, uint32_t flags);

/**
 * Destroys the framebuffer object.
 * 
 * Changed OpenGL state: None.
 */
void sgl_framebuffer_destroy(GLuint framebuffer);

/**
 * Binds the specified framebuffer and sets the viewport dimensions to `width` and `height`.
 * 
 * Changed OpenGL state: GL_DRAW_FRAMEBUFFER_BINDING.
 */
void sgl_framebuffer_bind(GLuint framebuffer, GLsizei width, GLsizei height);



//
// Draw functions
//

/**
 * Draws `primitive`s with a specified OpenGL `program` using glDrawElements(). If an index buffer is
 * specified glDrawArrays() is used (see ... below). Uniforms and vertex data is setup according to
 * directives in the `bindings` string (similar to `printf()` directives).
 * 
 * The `bindings` string is a compact notation for the glGetUniformLocation, glUniform, glGetAttribLocation,
 * glBindBuffer, glVertexAttribPointer, glBindTexture, etc. calls. For each directive `sgl_draw` calls
 * these functions to setup a uniform, an attribute or other state for the draw call (like an index buffer).
 * 
 * 
 * # Specifying vertex data
 * 
 * Vertex data is specified with `printf()` style directives. The function call
 * 
 *   sgl_draw(GL_TRIANGLES, program, "pos %3f color %3f", vertex_buffer);
 * 
 * tells OpenGL that the "pos" and "color" attributes are three component float vectors and the data should be
 * fetched from the buffer `vertex_buffer`. You only have to specify an OpenGL buffer object for the first
 * attribute directive, all other attributes fetch their data from the same buffer.
 * 
 * Each directive consists of a name and a format specifier, e.g. "pos" and "%3f". For each one the function
 * 
 * - queries the location of the attribute with glGetAttribLocation(),
 * - enables it with glEnableVertexAttribArray(),
 * - and tells OpenGL the format of the attribute with glVertexAttribPointer().
 * 
 * The stride and size for each attribute are calculated automatically under the assumption that the attributes
 * are tightly packed in the vertex buffer. You can use the special attribute name "_" to specify padding. For
 * example "_ %4f" will skip the space of a 4 component float vector in the buffer.
 * 
 * 
 * ## Vertex format specifier
 * 
 * Each specifier is in the form:   "%"  <dimensions>  <flags>  <type>
 * 
 * A format specifier is nothing more than a compact way to write the glVertexAttribPointer() arguments.
 * Please take a look at the documentation of glVertexAttribPointer() if you ask yourself what the dimensions,
 * flags and type mean.
 * 
 * Possible dimensions: "1", "2", "3", "4"
 * The dimension is mandatory. Even if you just have one float you need to write "%1f".
 * 
 * Possible types:
 * 
 * "f" (GL_FLOAT)
 * "b" (GL_BYTE)
 * "s" (GL_SHORT)
 * "i" (GL_INT)
 * 
 * Possible flags for type "f" (GL_FLOAT):
 * 
 * "h"  Use GL_HALF_FLOAT instead of GL_FLOAT
 * "f"  Use GL_FIXED instead of GL_FLOAT
 * 
 * Possible flags for type "b" (GL_BYTE):
 * 
 * "u"  Use GL_UNSIGNED_BYTE instead of GL_BYTE
 * "n"  Values are normalized. The entire value range of the data type is mapped to 0..1 in the shader.
 * "i"  Upload as int (use glVertexAttribIPointer())
 * 
 * Possible flags for type "s" (GL_SHORT):
 * 
 * "u"  Use GL_UNSIGNED_SHORT instead of GL_SHORT
 * "n"  Values are normalized. The entire value range of the data type is mapped to 0..1 in the shader.
 * "i"  Upload as int (use glVertexAttribIPointer())
 * 
 * Possible flags for type "i" (GL_INT):
 * 
 * "u"  Use GL_UNSIGNED_INT instead of GL_INT
 * "n"  Values are normalized. The entire value range of the data type is mapped to 0..1 in the shader.
 * "i"  Upload as int (use glVertexAttribIPointer())
 * 
 * For example the format specifier "%4unb" tells glVertexAttribPointer() to unpack 4 unsigned and
 * normalized bytes into a 4 component vector in the shader. The 4 values {128, 255, 64, 255} in the
 * vertex buffer will be mapped to vec4(0.5, 1.0, 0.25, 1.0) in the shader. By using this format specifier
 * you can store colors in the vertex buffer in a very compact way.
 * 
 * 
 * # Specifying uniforms
 * 
 * Uniforms are specified similarly to attributes but they use upper case types and different flags. Each
 * uniform also consumes one argument of the sgl_draw() function.
 * 
 *   float projection[16] = { ... };
 *   struct{ float x, y, z; } light = { ... };
 *   sgl_draw(GL_TRIANGLES, program, "projection %4tM light_pos %3F pos %3f color %3f", projection, &light, vertex_buffer);
 * 
 * This call sets the uniform "projection" to the values of the "projection" array. The uniform is a 4x4
 * matrix (type "M" with 4 dimensions). A float[16] on the CPU and a mat4 in the shader. The matrix is
 * transposed when uploaded ("t" flag). The second uniform "light_pos" is set to a 3 component float vector
 * (type "F" with 3 dimensions).
 * 
 * Each uniform directive consumes one argument of the `sgl_draw()` function. The arguments are consumed
 * in the same order as the directives are listed in the `bindings` string. The arguments for uniforms have
 * to be _pointers_ to the data for the uniform (sgl_draw() always uses the glUniform*v() functions). In the
 * above example the variable "projection" is already a pointer to the first byte of the float[16] array.
 * But the variable "light" is a struct so you need to get the pointer to it's location in memory. This is
 * the same even for a single float:
 * 
 *   float threshold = ...;
 *   sgl_draw(GL_TRIANGLES, program, "threshold %1F pos %3f color %3f", &threshold, vertex_buffer);
 * 
 * You can freely mix uniform and attribute directives in any order. But keep in mind that only the first
 * vertex attribute directive (lower case type) consumes an argument.
 * 
 * 
 * ## Uniform format specifier
 * 
 * Each specifier is in the form:   "%"  <dimensions>  <flags>  <type>
 * 
 * A format specifier is nothing more than a compact way to choose one of the glUniform*v() functions and set
 * some of its flags. Please take a look at the documentation of glUniform*v() if you ask yourself what the
 * dimensions, flags and type mean.
 * 
 * Possible dimensions: "1", "2", "3", "4"
 * The dimension is mandatory. Even if you just have one float you need to write "%1F".
 * For matrix uniforms you can also use: "2x3", "2x4", "3x2", "3x4", "4x2", "4x3"
 * 
 * Possible types:
 * 
 * "F" (float, glUniform*fv)
 * "I" (integer, glUniform*iv)
 * "U" (unsigned integer, glUniform*uiv)
 * "M" (matrix, glUniformMatrix*fv)
 * 
 * Possible flags for type "F" (float): None.
 * Possible flags for type "I" (integer): None.
 * Possible flags for type "U" (unsigned integer): None.
 * Possible flags for type "M" (matrix):
 * 
 * "t"  Transpose matrix.
 * 
 * 
 * # Specifying textures
 * 
 * Textures are specified as uniforms with the type "T". It binds a GL_TEXTURE_2D texture to a sampler uniform.
 * The matching argument has to be the object ID of the texture (GLuint), not a pointer. The function automatically
 * calls glActiveTexture(), glBindTexture() and glUniform1i().
 * 
 * Possible flags for type "T" (texture):
 * 
 * "r"  Texture is a GL_TEXTURE_RECTANGLE texture instead of a GL_TEXTURE_2D texture
 * "*"  The uniform is a texture array. The argument list must contain a size_t (the array length) and a
 *      GLuint* (pointer to an array of OpenGL texture names) instead of a single GLuint.
 * 
 * 
 * # Global options
 * 
 * Global options start with "$" instead of "%". Right now there is just one:
 * 
 * $I  Draw with an index buffer of GL_UNSIGNED_INT indices. One argument of type GLuint is consumed.
 * 
 * Possible flags:
 * 
 * "b"  Indices are of type GL_UNSIGNED_BYTE
 * "s"  Indices are of type GL_UNSIGNED_SHORT
 * 
 * 
 * # Using data from multiple vertex buffers
 * 
 * The first attribute directive consumes one argument that has to be an OpenGL buffer object.
 * This buffer is used for all following attributes or until an ";" is encountered. The next attribute
 * after ";" then consumes a new buffer argument. Stride and offsets of attributes are calculated
 * automatically.
 * 
 * Example:
 * 
 *   sgl_draw(GL_TRIANGLES, program, "pos %3f normal %3f ; color %4unb", geometry_vertex_buffer, color_vertex_buffer);
 * 
 * This draw call takes the data for the "pos" and "normal" attributes from the "geometry_vertex_buffer"
 * and the "color" attribute is read from the "color_vertex_buffer". Each buffer contains its data tighly
 * packed.
 * 
 * 
 * # TODO
 * 
 * - Change unsigned integer uniforms from %1U to %1uI (as "u" flag for "I" type instead of an extra type).
 *   Should be more consistent with attribute format specifier (where it's "%1ui").
 * - Check if cleanup of texture binding of really necessary. If so make sure we unbind the proper target.
 *   Basically we have to iterate over the directives again to get the target of each directive.
 * 
 * 
 * # Reference
 * 
 * Form of directives: <attribute or uniform name> "%" <dimensions> <flags> <type>
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
 * 	%1U  glUniform1uiv
 * 	%2U  glUniform2uiv
 * 	%3U  glUniform3uiv
 * 	%4U  glUniform4uiv
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
 * 	Flags:
 * 		t  transpose matrix
 * 
 * T (textures)
 * 	%T  bind a GL_TEXTURE_2D texture to a sampler uniform
 * 	
 * 	Flags:
 * 		r  texture is a GL_TEXTURE_RECTANGLE texture
 *  	*  the uniform is a texture array. The argument list must contain a size_t (the array length) and a
 *  	   GLuint* (pointer to an array of OpenGL texture names) instead of a single GLuint.
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
 * 
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
 */
int sgl_draw(GLenum primitive, GLuint program, const char* bindings, ...);


//
// Utilities
//

/**
 * Returns the last OpenGL error and outputs an error message to stderr (a bit like `perror()`). If no
 * error happened `0` is returned and nothing is printed. To make the code more readable `sgl_error()`
 * is meant to be used in the `if` that checks for errors:
 * 
 *   location = glGetUniformLocation(program, name);
 *   if ( sgl_error("Failed to lookup uniform %s. glGetUniformLocation()", name) ) {
 *   	// react to error
 *   }
 * 
 * If there is a last OpenGL error this function prints `description` followed by ": " and the last
 * pending OpenGL error to stderr. It behaves like `printf()` so you can use stuff like %s in `description`
 * to print additional error information.
 */
GLenum sgl_error(const char* description, ...);

/**
 * Returns a pointer to the zero terminated `malloc()`ed contents of the file. If size is
 * not `NULL` it's target is set to the size of the file not including the zero terminator
 * at the end of the memory block.
 * 
 * On error `NULL` is returned and `errno` is set accordingly.
 */
void* sgl_fload(const char* filename, size_t* size);

/**
 * Appends a printf style string to an already existing string pointed to by `dest`. The
 * existing string is realloced to be large enough. If `dest` is `NULL` a new string is
 * allocated. Returns the result string or `NULL` on error.
 * 
 *   char* a = NULL;
 *   sgl_strappendf(&a, "Hello %s!\n", "World");
 *   sgl_strappendf(&a, "x: %f y: %f\n", 7.0, 13.7);
 *   
 *   char* b = sgl_strappendf(NULL, "%d %s\n", 42, "is the answer");
 * 
 */
char* sgl_strappendf(char** dest, const char* format, ...);

#endif // SLIM_GL_HEADER




#ifdef SLIM_GL_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <stdbool.h>
#include <ctype.h>


//
// OpenGL program functions
//

static const char* sgl__type_to_string(GLenum type);
static GLuint sgl__create_and_compile_program(const char* vertex_shader_code, const char* fragment_shader_code, const char* vertex_shader_name, const char* fragment_shader_name, char** compiler_errors);
static GLuint sgl__create_and_compile_shader(GLenum shader_type, const char* code, const char* filename_for_errors, char** compiler_errors);


typedef struct {
	const char* error_at;
	const char* error_message;
	// We need a zero terminated string for glGetAttribLocation() so use a buffer instead of pointer + length into the
	// argument string.
	char name[128];
	char modifiers[16];
	char type;
} sgl_arg_t, *sgl_arg_p;

/**
 * Returns true if a character is a whitespace as defined by the GLSL spec (3.1 Character Set).
 * That are spaces and the ASCII range containing horizontal tab, new line, vertical tab, form feed and carriage return.
 * If you look at the ASCII table (man ascii) you'll see that all whitespaces except space are consecutive ASCII codes.
 * So we cover all these characters with one range check.
 */
static inline int sgl__is_whitespace(char c) {
	return c == ' ' || (c >= '\t' && c <= '\r');
}

/**
 * Returns true if a character is valid for an GLSL identifier (spec chapter 3.7 Identifiers) or a minus sign (-).
 * Names starting with a minus are internal options (e.g. -index or -padding) instead of attribute or uniform names.
 */
static inline int sgl__is_name(char c) {
	return (c >= 'a' && c <= 'z') || c == '_' || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
}

#define SGL__NAMED_ARGS        (1 << 0)
#define SGL__BUFFER_DIRECTIVES (1 << 1)

static const char* sgl__next_argument(const char* string, int flags, sgl_arg_p arg) {
	const char* c = string;
	
#define EXIT_WITH_ERROR_IF(condition, message) if(condition){ arg->error_at = c; arg->error_message = (message); return NULL; }
	
	// A return value of NULL signals the end of arguments or an error. Return NULL when we're called with a NULL string
	// so we can be called repeatedly at the end. It also makes sure we can dereference the string after this.
	if (c == NULL)
		return NULL;
	
	// Skip whitespaces
	while( sgl__is_whitespace(*c) )
		c++;
	
	// Return NULL when we're at the zero terminator to signal the end of arguments
	if (*c == '\0')
		return NULL;
	
	if (flags & SGL__BUFFER_DIRECTIVES) {
		if (*c == ';') {
			// Got a buffer end directive
			arg->type = *c++;
			return c;
		}
	}
	
	if (flags & SGL__NAMED_ARGS) {
		// Read the name into the name buffer and add the zero-terminator
		size_t name_len = 0;
		while ( sgl__is_name(*c) ) {
			// We can only fill the name buffer up until one byte it left. We need that byte for the zero terminator.
			EXIT_WITH_ERROR_IF(name_len >= sizeof(arg->name) - 1, "Name is to long");
			arg->name[name_len++] = *c++;
		}
		arg->name[name_len] = '\0';
		// If the name isn't followed by a whitespace char we either got an invalid char in a name or the zero terminator.
		// Either way we can't continue.
		EXIT_WITH_ERROR_IF(!sgl__is_whitespace(*c), "Got invalid character in name");
		
		// Skip whitespaces after name
		while( sgl__is_whitespace(*c) )
			c++;
	}
	
	EXIT_WITH_ERROR_IF(*c != '%', "Expected at '%' at the start of a directive");
	c++;
	
	// Read all following chars as modifiers (including the last one for now)
	size_t mod_len = 0;
	while ( !(sgl__is_whitespace(*c) || *c == '\0') ) {
		EXIT_WITH_ERROR_IF(mod_len >= sizeof(arg->modifiers), "To many modifiers for directive");
		arg->modifiers[mod_len++] = *c++;
	}
	EXIT_WITH_ERROR_IF(mod_len < 1, "At least one character for the type is necessary after a '%'");
	
	// The last char in the modifiers buffer is our type so put it into the type field. Then overwrite the last char
	// with the zero terminator.
	arg->type = arg->modifiers[mod_len-1];
	arg->modifiers[mod_len-1] = '\0';
	
	return c;
	
#undef EXIT_WITH_ERROR_IF
}


GLuint sgl_program_from_files(const char* vertex_shader_file, const char* fragment_shader_file, char** compiler_errors) {
	char* vertex_shader_code = sgl_fload(vertex_shader_file, NULL);
	if (vertex_shader_code == NULL) {
		if (compiler_errors)
			*compiler_errors = sgl_strappendf(NULL, "Can't read vertex shader file %s: %s\n", vertex_shader_file, strerror(errno));
		return 0;
	}
	
	char* fragment_shader_code = sgl_fload(fragment_shader_file, NULL);
	if (fragment_shader_code == NULL) {
		free(vertex_shader_code);
		if (compiler_errors)
			*compiler_errors = sgl_strappendf(NULL, "Can't read fragment shader file %s: %s\n", fragment_shader_file, strerror(errno));
		return 0;
	}
	
	GLuint program = sgl__create_and_compile_program(vertex_shader_code, fragment_shader_code, vertex_shader_file, fragment_shader_file, compiler_errors);
	
	free(vertex_shader_code);
	free(fragment_shader_code);
	
	return program;
}

GLuint sgl_program_from_strings(const char* vertex_shader_code, const char* fragment_shader_code, char** compiler_errors) {
	return sgl__create_and_compile_program(vertex_shader_code, fragment_shader_code, "vertex shader", "fragment shader", compiler_errors);
}

void sgl_program_destroy(GLuint program) {
	GLint shader_count = 0;
	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
	
	GLuint shaders[shader_count];
	glGetAttachedShaders(program, shader_count, NULL, shaders);
	
	glDeleteProgram(program);
	for(ssize_t i = 0; i < shader_count; i++)
		glDeleteShader(shaders[i]);
}

void sgl_program_inspect(GLuint program) {
	GLint size;
	GLenum type;
	{
		GLint attrib_count = 0, buffer_size = 0;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attrib_count);
		fprintf(stderr, "%d attributes:\n", attrib_count);
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &buffer_size);
		char buffer[buffer_size];
		
		for(ssize_t i = 0; i < attrib_count; i++){
			glGetActiveAttrib(program, i, buffer_size, NULL, &size, &type, buffer);
			fprintf(stderr, "- %s %s", buffer, sgl__type_to_string(type));
			if (size > 1)
				fprintf(stderr, "[%d]", size);
			fprintf(stderr, "\n");
		}
	}
	
	{
		GLint uniform_count = 0, buffer_size = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
		fprintf(stderr, "%d uniforms:\n", uniform_count);
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &buffer_size);
		char buffer[buffer_size];
		
		for(ssize_t i = 0; i < uniform_count; i++){
			glGetActiveUniform(program, i, buffer_size, NULL, &size, &type, buffer);
			fprintf(stderr, "- %s %s", buffer, sgl__type_to_string(type));
			if (size > 1)
				fprintf(stderr, "[%d]", size);
			fprintf(stderr, "\n");
		}
	}
}

/**
 * Helper function to return the printable name for an OpenGL data type.
 */
static const char* sgl__type_to_string(GLenum type) {
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
		case GL_INT_SAMPLER_BUFFER: return "isamplerBuffer​";
		case GL_INT_SAMPLER_2D_RECT: return "isampler2DRect​";
		case GL_UNSIGNED_INT_SAMPLER_1D: return "usampler1D​";
		case GL_UNSIGNED_INT_SAMPLER_2D: return "usampler2D​";
		case GL_UNSIGNED_INT_SAMPLER_3D: return "usampler3D​";
		case GL_UNSIGNED_INT_SAMPLER_CUBE: return "usamplerCube​";
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return "usampler2DArray​";
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray​";
		case GL_UNSIGNED_INT_SAMPLER_BUFFER: return "usamplerBuffer​";
		case GL_UNSIGNED_INT_SAMPLER_2D_RECT: return "usampler2DRect​";
		
#		ifdef GL_VERSION_3_2
			case GL_SAMPLER_2D_MULTISAMPLE: return "sampler2DMS​";
			case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: return "sampler2DMSArray​";
			case GL_INT_SAMPLER_2D_MULTISAMPLE: return "isampler2DMS​";
			case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "isampler2DMSArray​";
			case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return "usampler2DMS​";
			case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "usampler2DMSArray​";
#		endif
		
		default: return "unknown";
	}
}

/**
 * Helper function to compile and link a complete program.
 * 
 * Returns the program object ID or `0` on error. Compiler errors are returned in `compiler_errors` if it's not `NULL`.
 */
static GLuint sgl__create_and_compile_program(const char* vertex_shader_code, const char* fragment_shader_code, const char* vertex_shader_name, const char* fragment_shader_name, char** compiler_errors) {
	char* errors = NULL;
	
	GLuint vertex_shader = sgl__create_and_compile_shader(GL_VERTEX_SHADER, vertex_shader_code, vertex_shader_name, &errors);
	GLuint fragment_shader = sgl__create_and_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_code, fragment_shader_name, &errors);
	if (vertex_shader == 0 || fragment_shader == 0)
		goto shaders_failed;
	
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vertex_shader);
	glAttachShader(prog, fragment_shader);
	glLinkProgram(prog);
	
	GLint result = GL_TRUE;
	glGetProgramiv(prog, GL_LINK_STATUS, &result);
	if (result == GL_FALSE){
		result = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &result);
		char buffer[result];
		glGetProgramInfoLog(prog, result, NULL, buffer);
		sgl_strappendf(&errors, "Can't link vertex and pixel shader:\n%s\n", buffer);
		
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
	
	if (*errors) {
		if (compiler_errors) {
			*compiler_errors = errors;
		} else {
			fprintf(stderr, "%s", errors);
			free(errors);
		}
	}

	return 0;
}

/**
 * Helper function to load and compile source code as a shader.
 * 
 * Returns the shaders object ID on success or `0` on error. Compiler errors in the shader are appended to the
 * `compiler_errors` string (if it's not `NULL`).
 */
static GLuint sgl__create_and_compile_shader(GLenum shader_type, const char* code, const char* filename_for_errors, char** compiler_errors) {
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, (const char*[]){ code }, (const int[]){ -1 });
	glCompileShader(shader);
	
	GLint result = GL_TRUE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result)
		return shader;
	
	if (compiler_errors) {
		result = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &result);
		char buffer[result];
		glGetShaderInfoLog(shader, result, NULL, buffer);
		sgl_strappendf(compiler_errors, "Can't compile %s:\n%s\n", filename_for_errors, buffer);
	}
	
	glDeleteShader(shader);
	return 0;
}



//
// Buffer functions
//

GLuint sgl_buffer_new(const void* data, size_t size) {
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	if (buffer == 0)
		return 0;
	
	if (size > 0)
		sgl_buffer_update(buffer, data, size, GL_STATIC_DRAW);
	
	return buffer;
}

void sgl_buffer_destroy(GLuint buffer) {
	glDeleteBuffers(1, (const GLuint[]){ buffer });
}

void sgl_buffer_update(GLuint buffer, const void* data, size_t size, GLenum usage) {
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



//
// Texture functions
//

GLuint sgl_texture_new(uint32_t width, uint32_t height, uint8_t components, const void* data, size_t stride_in_pixels, uint32_t flags) {
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
	
	GLenum target = 0;
	GLsizei mipmap_levels = 1;
	GLint prev_bound_texture = 0;
	if (flags & SGL_RECT) {
		target = GL_TEXTURE_RECTANGLE;
		glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &prev_bound_texture);
	} else {
		target = GL_TEXTURE_2D;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_bound_texture);
		
		// We just look for the highest bit set. Then we know how often we can half the width or hight.
		mipmap_levels = 32 - __builtin_clz(width | height);
	}
	
	glBindTexture(target, texture);
		// Version with ARB_texture_storage
		//glTexStorage2D(target, mipmap_levels, internal_format, width, height);
		// Replacement taken out of the glTexStorage2D() docs
		GLsizei w = width, h = height;
		for (GLsizei i = 0; i < mipmap_levels; i++) {
			glTexImage2D(target, i, internal_format, w, h, 0, data_format, GL_UNSIGNED_BYTE, NULL);
			w /= 2;
			h /= 2;
			if (w < 1) w = 1;
			if (h < 1) h = 1;
		}
		
		// Set high quality texture filtering as default for 2D (not rect) textures
		if (target == GL_TEXTURE_2D)
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		
		if (data) {
			if (stride_in_pixels == 0)
				stride_in_pixels = width;
			
			GLint prev_unpack_alignment = 0, prev_row_length = 0;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
			glGetIntegerv(GL_UNPACK_ROW_LENGTH, &prev_row_length);
			
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, stride_in_pixels);
				
				glTexSubImage2D(target, 0, 0, 0, width, height, data_format, GL_UNSIGNED_BYTE, data);
				
			glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, prev_row_length);
			
			if ( target == GL_TEXTURE_2D && !(flags & SGL_SKIP_MIPMAPS) )
				glGenerateMipmap(target);
		}
	glBindTexture(target, prev_bound_texture);
	
	return texture;
}

void sgl_texture_destroy(GLuint texture) {
	glDeleteTextures(1, (const GLuint[]){ texture });
}

void sgl_texture_update(GLuint texture, const void* data, size_t stride_in_pixels, uint32_t flags) {
	sgl_texture_update_sub(texture, 0, 0, 0, 0, data, stride_in_pixels, flags);
}

void sgl_texture_update_sub(GLuint texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void* data, size_t stride_in_pixels, uint32_t flags) {
	GLenum target = 0;
	GLint prev_bound_texture = 0;
	if (flags & SGL_RECT) {
		target = GL_TEXTURE_RECTANGLE;
		glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &prev_bound_texture);
	} else {
		target = GL_TEXTURE_2D;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_bound_texture);
	}
	
	glBindTexture(target, texture);
		GLint width = w, height = h, internal_format = 0;
		if (width == 0) {
			glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &width);
			width -= x;
		}
		if (height == 0) {
			glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &height);
			height -= y;
		}
		if (stride_in_pixels == 0) {
			stride_in_pixels = width;
		}
		glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
		
		GLenum data_format = 0;
		switch(internal_format) {
			case GL_R8:    data_format = GL_RED;  break;
			case GL_RG8:   data_format = GL_RG;   break;
			case GL_RGB8:  data_format = GL_RGB;  break;
			case GL_RGBA8: data_format = GL_RGBA; break;
		}
		
		if (data_format != 0) {
			GLint prev_unpack_alignment = 0, prev_row_length = 0;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack_alignment);
			glGetIntegerv(GL_UNPACK_ROW_LENGTH, &prev_row_length);
			
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, stride_in_pixels);
				
				glTexSubImage2D(target, 0, x, y, width, height, data_format, GL_UNSIGNED_BYTE, data);
				
			glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack_alignment);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, prev_row_length);
			
			if ( target == GL_TEXTURE_2D && !(flags & SGL_SKIP_MIPMAPS) )
				glGenerateMipmap(GL_TEXTURE_2D);
		}
	glBindTexture(target, prev_bound_texture);
}

void sgl_texture_dimensions(GLuint texture, uint32_t* width, uint32_t* height, uint32_t flags) {
	GLenum target = 0;
	GLint prev_bound_texture = 0;
	if (flags & SGL_RECT) {
		target = GL_TEXTURE_RECTANGLE;
		glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE, &prev_bound_texture);
	} else {
		target = GL_TEXTURE_2D;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_bound_texture);
	}
	
	glBindTexture(target, texture);
	// TODO: error checking
		if (width)
			glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, (int32_t*)width);
		if (height)
			glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, (int32_t*)height);
	glBindTexture(target, prev_bound_texture);
}


//
// Frame buffer functions
//

GLuint sgl_framebuffer_new(GLuint color_buffer_texture, uint32_t flags) {
	GLint prev_draw_fb = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fb);
	
	GLenum texture_target = (flags & SGL_RECT) ? GL_TEXTURE_RECTANGLE : GL_TEXTURE_2D;
	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, color_buffer_texture, 0);
	sgl_error("Failed to bind color buffer to framebuffer. glFramebufferTexture2D()");
	
	if ( glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		glDeleteFramebuffers(1, &framebuffer);
		framebuffer = 0;
	}
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_draw_fb);
	return framebuffer;
}

void sgl_framebuffer_destroy(GLuint framebuffer) {
	glDeleteFramebuffers(1, &framebuffer);
}

void sgl_framebuffer_bind(GLuint framebuffer, GLsizei width, GLsizei height) {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
	if ( !sgl_error("Failed to bind framebuffer %u. glBindFramebuffer()", framebuffer) && width != 0 && height != 0 )
		glViewport(0, 0, width, height);
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

int sgl_draw(GLenum primitive, GLuint program, const char* bindings, ...) {
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
	
	// Make sure no previous error code messes up our state
	glGetError();
	
	GLuint vertex_array_object = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vertex_array_object);
	if (vertex_array_object == 0) {
		glGenVertexArrays(1, &vertex_array_object);
		glBindVertexArray(vertex_array_object);
		sgl_error("Failed to generate and bind a new vertex array object. glBindVertexArray()");
	}
	
	glUseProgram(program);
	if ( sgl_error("Can't use OpenGL program for drawing. glUseProgram()") )
		return -1;
	
	va_start(args, bindings);
	const char* setup_pass_bindings = bindings;
	while( next_directive(&setup_pass_bindings, &d) ) {
		if (d.type == ';') {
			// User no longer wants to use the current buffer for attributes. So reset all the buffer
			// stuff. If a new attribute directive comes around it will consume the next buffer.
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			sgl_error("Failed to unbind vertex buffer. glBindBuffer(GL_ARRAY_BUFFER)");
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
			if ( ! sgl_error("Unable to bind index buffer. glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)") ) {
				int index_buffer_size = 0;
				glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &index_buffer_size);
				if ( !sgl_error("Unable to determine size of index buffer. glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE)") )
					indices_to_render = index_buffer_size / index_type_size;
			}
			
			use_index_buffer = true;
			continue;
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
			if ( sgl_error("Error on looking up uniform %s. glGetUniformLocation()", d.name) ) {
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
				if ( sgl_error("Error on looking up attribute %s. glGetAttribLocation()", d.name) ) {
					// All glGetAttribLocation errors are caused by invalid programs. So no point in
					// trying anything else since the program is broken.
					return -1;
				} else if (location == -1) {
					fprintf(stderr, "Program has no attribute \"%s\", attribute unused and it's space will be skipped in the buffer.\n", d.name);
				}
			}
			
			// We don't know the stride of the attribute we're going to bind. So sum the size of the
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
					// Skip global parameters and uniforms
					if (ld.type > 256 || isupper(ld.type))
						continue;
					
					if ( parse_attribute_directive(&ld, &ai) )
						current_buffer_stride += ai.type_size * ai.components;
				}
				
				// Consume an argument, bind it as array buffer and determine the number of vertecies in
				// there (only needed when we don't use an index buffer for rendering).
				glBindBuffer(GL_ARRAY_BUFFER, va_arg(args, GLuint));
				if ( ! sgl_error("Unable to bind vertex buffer at attribute %s. glBindBuffer(GL_ARRAY_BUFFER)", d.name) && !use_index_buffer ) {
					glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_buffer_size);
					// See how many vertecies are in that buffer. In the end we want to render as many
					// vertecies as are present in all buffers.
					if ( !sgl_error("Unable to determine buffer size at attribute %s. glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE)", d.name) && current_buffer_size > 0 && current_buffer_stride > 0) {
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
					case '1': glUniform1fv(location, 1, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniform1fv()", d.name); continue;
					case '2': glUniform2fv(location, 1, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniform2fv()", d.name); continue;
					case '3': glUniform3fv(location, 1, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniform3fv()", d.name); continue;
					case '4': glUniform4fv(location, 1, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniform4fv()", d.name); continue;
				}
				break;
			case 'I':
				if (d.modifier_length != 1) break;
				switch(d.modifiers[0]) {
					case '1': glUniform1iv(location, 1, va_arg(args, GLint*)); sgl_error("Failed to set uniform %s. glUniform1iv()", d.name); continue;
					case '2': glUniform2iv(location, 1, va_arg(args, GLint*)); sgl_error("Failed to set uniform %s. glUniform2iv()", d.name); continue;
					case '3': glUniform3iv(location, 1, va_arg(args, GLint*)); sgl_error("Failed to set uniform %s. glUniform3iv()", d.name); continue;
					case '4': glUniform4iv(location, 1, va_arg(args, GLint*)); sgl_error("Failed to set uniform %s. glUniform4iv()", d.name); continue;
				}
				break;
			case 'U':
				if (d.modifier_length != 1) break;
				switch(d.modifiers[0]) {
					case '1': glUniform1uiv(location, 1, va_arg(args, GLuint*)); sgl_error("Failed to set uniform %s. glUniform1uiv()", d.name); continue;
					case '2': glUniform2uiv(location, 1, va_arg(args, GLuint*)); sgl_error("Failed to set uniform %s. glUniform2uiv()", d.name); continue;
					case '3': glUniform3uiv(location, 1, va_arg(args, GLuint*)); sgl_error("Failed to set uniform %s. glUniform3uiv()", d.name); continue;
					case '4': glUniform4uiv(location, 1, va_arg(args, GLuint*)); sgl_error("Failed to set uniform %s. glUniform4uiv()", d.name); continue;
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
							if (d.modifiers[2] == '3')      { glUniformMatrix2x3fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix2x3fv()", d.name); continue; }
							else if (d.modifiers[2] == '4') { glUniformMatrix2x4fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix2x4fv()", d.name); continue; }
						} else {
							glUniformMatrix2fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix2fv()", d.name); continue;
						}
						break;
					case '3':
						if (d.modifiers[1] == 'x') {
							if (d.modifiers[2] == '2')      { glUniformMatrix3x2fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix3x2fv()", d.name); continue; }
							else if (d.modifiers[2] == '4') { glUniformMatrix3x4fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix3x4fv()", d.name); continue; }
						} else {
							glUniformMatrix3fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix3fv()", d.name); continue;
						}
						break;
					case '4':
						if (d.modifiers[1] == 'x') {
							if (d.modifiers[2] == '2')      { glUniformMatrix4x2fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix4x2fv()", d.name); continue; }
							else if (d.modifiers[2] == '3') { glUniformMatrix4x3fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix4x3fv()", d.name); continue; }
						} else {
							glUniformMatrix4fv(location, 1, transpose, va_arg(args, GLfloat*)); sgl_error("Failed to set uniform %s. glUniformMatrix4fv()", d.name); continue;
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
				size_t array_length = -1;
				
				for(char* m = d.modifiers; *m != '\0'; m++) {
					switch(*m) {
						case 'r': target = GL_TEXTURE_RECTANGLE; break;
						case '*': array_length = va_arg(args, size_t); break;
						default: goto invalid_texture_modifier;
					}
				}
				
				if (array_length == (size_t)-1) {
					// Just one simple texture
					glActiveTexture(GL_TEXTURE0 + active_textures);
					if ( sgl_error("Failed to activate texture image unit %d for texture %s. Probably to many textures. glActiveTexture()", active_textures, d.name) )
						continue;
					glBindTexture(target, va_arg(args, GLint));
					if ( sgl_error("Failed to bind texture for %s to %s. glBindTexture()", d.name, (target == GL_TEXTURE_2D) ? "GL_TEXTURE_2D" : "GL_TEXTURE_RECTANGLE") )
						continue;
					glUniform1i(location, active_textures);
					if ( sgl_error("Failed to set uniform for texture %s. glUniform1i()", d.name) ) {
						glBindTexture(target, 0);
						continue;
					}
				
					active_textures++;
				} else {
					// A texture array (possibly empty but we need to consume the args anyway)
					GLuint* textures = va_arg(args, GLuint*);
					GLint image_unit_indices[array_length];
					for(size_t i = 0; i < array_length; i++) {
						glActiveTexture(GL_TEXTURE0 + active_textures);
						if ( sgl_error("Failed to activate texture image unit %d for texture array %s. Probably to many textures. glActiveTexture()", active_textures, d.name) )
							break;
						image_unit_indices[i] = active_textures;
						active_textures++;
						
						glBindTexture(target, textures[i]);
						if ( sgl_error("Failed to bind texture for %s[%zu] to %s. glBindTexture()", d.name, i, (target == GL_TEXTURE_2D) ? "GL_TEXTURE_2D" : "GL_TEXTURE_RECTANGLE") )
							continue;
					}
					
					if (array_length > 0) {
						glUniform1iv(location, array_length, image_unit_indices);
						if ( sgl_error("Failed to set uniform for texture array %s. glUniform1iv()", d.name) )
							continue;
					}
				}
				continue;
				
				}
				invalid_texture_modifier:
				break;
			
			// Attributes
			default:
				if ( parse_attribute_directive(&d, &ai) ) {
					// Make sure that we count the attributes size in future offsets. So the buffer layout is the
					// same even if we fail to use some attributes.
					size_t offset = current_buffer_offset;
					current_buffer_offset += ai.type_size * ai.components;
					
					// Don't process unknown attributes (would just lead to errors) and ignore padding attributes
					if ( location == -1 )
						continue;
					
					glEnableVertexAttribArray(location);
					if (sgl_error("Failed to enable vertex attribute %s. glEnableVertexAttribArray()", d.name))
						continue;
					
					if (ai.upload_as_int)
						glVertexAttribIPointer(location, ai.components, ai.opengl_type, current_buffer_stride, (GLvoid*)offset);
					else
						glVertexAttribPointer(location, ai.components, ai.opengl_type, ai.normalized, current_buffer_stride, (GLvoid*)offset);
					
					sgl_error("Failed to setup buffer layout for attribute %s. %s()", d.name, ai.upload_as_int ? "glVertexAttribIPointer" : "glVertexAttribPointer");
					continue;
				}
				break;
		}
		
		// We'll only arrive down here if the type is invalid so print out an error message
		fprintf(stderr, "Invalid type %%%s%c for %s\n", d.modifiers, d.type, d.name);
	}
	va_end(args);
	
	
	// Draw stuff, start of the fireworks... finally
	if ( ! use_index_buffer ) {
		glDrawArrays(primitive, 0, vertecies_to_render);
		sgl_error("Drawcall failed. glDrawArrays()");
	} else {
		glDrawElements(primitive, indices_to_render, index_buffer_type, 0);
		sgl_error("Drawcall failed. glDrawElements()");
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
 * Helper function for `sgl_draw()`. Parses the next directive in `*bindings`. Returns the type of
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
#ifdef				GL_VERSION_4_1
					case 'f': ai->opengl_type = GL_FIXED;      ai->type_size = sizeof(GLfixed); break;
#endif
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



//
// Utility functions
//

GLenum sgl_error(const char* description, ...) {
	GLenum error = glGetError();
	if (error == GL_NO_ERROR)
		return GL_NO_ERROR;
	
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

void* sgl_fload(const char* filename, size_t* size) {
	long filesize = 0;
	char* data = NULL;
	int error = -1;
	
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	
	if ( fseek(f, 0, SEEK_END)              == -1       ) goto fail;
	if ( (filesize = ftell(f))              == -1       ) goto fail;
	if ( fseek(f, 0, SEEK_SET)              == -1       ) goto fail;
	if ( (data = malloc(filesize + 1))      == NULL     ) goto fail;
	// TODO: proper error detection for fread and get proper error code with ferror
	if ( (long)fread(data, 1, filesize, f)  != filesize ) goto free_and_fail;
	fclose(f);
	
	data[filesize] = '\0';
	if (size)
		*size = filesize;
	return (void*)data;
	
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

char* sgl_strappendf(char** dest, const char* format, ...) {
	va_list args;
	int append_size = 0;
	
	va_start(args, format);
		append_size = vsnprintf(NULL, 0, format, args);
	va_end(args);
	
	if (append_size == -1)
		return NULL;
	
	size_t old_size = 0;
	char* str = NULL;
	if (dest && *dest) {
		old_size = strlen(*dest);
		str = *dest;
	} else {
		old_size = 0;
		str = NULL;
	}
	
	size_t size = old_size + append_size + 1;
	str = realloc(str, size);
	va_start(args, format);
		vsnprintf(str + old_size, size - old_size, format, args);
	va_end(args);
	
	if (dest)
		*dest = str;
	return str;
}

#endif // SLIM_GL_IMPLEMENTATION