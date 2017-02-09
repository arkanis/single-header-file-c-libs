#pragma once

#include <stdio.h>
#include <stdint.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>


GLuint program_new(const char* vertex_shader_file, const char* fragment_shader_file, FILE* compiler_message_stream);
GLuint program_new_from_string(const char* vertex_shader_code, const char* fragment_shader_code, FILE* compiler_message_stream);
void   program_destroy(GLuint program);
void   program_inspect(GLuint program, FILE* output_stream);

GLuint buffer_new(const void* data, size_t size);
void   buffer_destroy(GLuint buffer);
void   buffer_update(GLuint buffer, const void* data, size_t size, GLenum usage);

#define SGL_RECT          (1 << 0)
#define SGL_SKIP_MIPMAPS  (1 << 1)

GLuint texture_new(const void* data, uint32_t width, uint32_t height, uint8_t components, ssize_t stride, uint32_t flags);
void   texture_destroy(GLuint texture);
void   texture_update(GLuint texture, const void* data, ssize_t stride, uint32_t flags);
void   texture_update_sub(GLuint texture, uint32_t x, uint32_t y, int32_t w, int32_t h, const void* data, ssize_t stride, uint32_t flags);
void   texture_dimensions(GLuint texture, int32_t* width, int32_t* height, uint32_t flags);

GLuint framebuffer_new(GLuint color_buffer_texture, uint32_t flags);
void   framebuffer_destroy(GLuint framebuffer);
void   framebuffer_blit(GLuint read_framebuffer, GLint rx, GLint ry, GLint rw, GLint rh, GLuint draw_framebuffer, GLint dx, GLint dy, GLint dw, GLint dh);
void   framebuffer_bind(GLuint framebuffer, GLsizei width, GLsizei height);

int    render(GLenum primitive, GLuint program, const char* bindings, ...);

#ifdef _SGL_UTILS
void* fload(const char* filename, size_t* size);
#endif


/*

Ideas:

- add cube maps
- SGL_DEPTH, SGL_STENCIL flags for textures -> as backend for FBOs

*/