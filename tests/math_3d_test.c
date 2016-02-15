// Needed for open_memstream() to test m4_fprintp()
#define _POSIX_C_SOURCE 200809L

#define MATH_3D_IMPLEMENTATION
#include "../math_3d.h"
#define SLIM_TEST_IMPLEMENTATION
#include "../slim_test.h"


//
// Additional check macros
//

#define st_check_matrix(actual, expected) st_check_msg(              \
	fabs((actual).m00 - (expected).m00) < 0.0001 &&                  \
	fabs((actual).m01 - (expected).m01) < 0.0001 &&                  \
	fabs((actual).m02 - (expected).m02) < 0.0001 &&                  \
	fabs((actual).m03 - (expected).m03) < 0.0001 &&                  \
	                                                                 \
	fabs((actual).m10 - (expected).m10) < 0.0001 &&                  \
	fabs((actual).m11 - (expected).m11) < 0.0001 &&                  \
	fabs((actual).m12 - (expected).m12) < 0.0001 &&                  \
	fabs((actual).m13 - (expected).m13) < 0.0001 &&                  \
	                                                                 \
	fabs((actual).m20 - (expected).m20) < 0.0001 &&                  \
	fabs((actual).m21 - (expected).m21) < 0.0001 &&                  \
	fabs((actual).m22 - (expected).m22) < 0.0001 &&                  \
	fabs((actual).m23 - (expected).m23) < 0.0001 &&                  \
	                                                                 \
	fabs((actual).m30 - (expected).m30) < 0.0001 &&                  \
	fabs((actual).m31 - (expected).m31) < 0.0001 &&                  \
	fabs((actual).m32 - (expected).m32) < 0.0001 &&                  \
	fabs((actual).m33 - (expected).m33) < 0.0001                     \
,                                                                    \
	#actual " == " #expected " failed, got:\n"                       \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  expected:\n"                                                  \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  | %6.2f %6.2f %6.2f %6.2f |\n"                                \
	"  | %6.2f %6.2f %6.2f %6.2f |"                                  \
,                                                                    \
	(actual).m00, (actual).m10, (actual).m20, (actual).m30,          \
	(actual).m01, (actual).m11, (actual).m21, (actual).m31,          \
	(actual).m02, (actual).m12, (actual).m22, (actual).m32,          \
	(actual).m03, (actual).m13, (actual).m23, (actual).m33,          \
	                                                                 \
	(expected).m00, (expected).m10, (expected).m20, (expected).m30,  \
	(expected).m01, (expected).m11, (expected).m21, (expected).m31,  \
	(expected).m02, (expected).m12, (expected).m22, (expected).m32,  \
	(expected).m03, (expected).m13, (expected).m23, (expected).m33   \
)

#define st_check_vec3(actual, expected, epsilon) st_check_msg(                            \
	fabs((actual).x - (expected).x) < (epsilon) &&                                        \
	fabs((actual).y - (expected).y) < (epsilon) &&                                        \
	fabs((actual).z - (expected).z) < (epsilon)                                           \
,                                                                                         \
	#actual " == " #expected " failed, got (%.2f %.2f %.2f), expected (%.2f %.2f %.2f)",  \
	(actual).x, (actual).y, (actual).z,                                                   \
	(expected).x, (expected).y, (expected).z                                              \
)


//
// Test cases
//

void test_matrix_memory_layout() {
	// Check that the indexed and named members actually refere to the same
	// values of the matrix.
	mat4_t mat = (mat4_t){
		.m[0][0] =  1,  .m[1][0] =  2,  .m[2][0] =  3,  .m[3][0] =  4,
		.m[0][1] =  5,  .m[1][1] =  6,  .m[2][1] =  7,  .m[3][1] =  8,
		.m[0][2] =  9,  .m[1][2] = 10,  .m[2][2] = 11,  .m[3][2] = 12,
		.m[0][3] = 13,  .m[1][3] = 14,  .m[2][3] = 15,  .m[3][3] = 16
	};
	
	float epsilon = 0.0001;
	st_check_float(mat.m[0][0], mat.m00, epsilon);
	st_check_float(mat.m[0][1], mat.m01, epsilon);
	st_check_float(mat.m[0][2], mat.m02, epsilon);
	st_check_float(mat.m[0][3], mat.m03, epsilon);
	
	st_check_float(mat.m[1][0], mat.m10, epsilon);
	st_check_float(mat.m[1][1], mat.m11, epsilon);
	st_check_float(mat.m[1][2], mat.m12, epsilon);
	st_check_float(mat.m[1][3], mat.m13, epsilon);
	
	st_check_float(mat.m[2][0], mat.m20, epsilon);
	st_check_float(mat.m[2][1], mat.m21, epsilon);
	st_check_float(mat.m[2][2], mat.m22, epsilon);
	st_check_float(mat.m[2][3], mat.m23, epsilon);
	
	st_check_float(mat.m[3][0], mat.m30, epsilon);
	st_check_float(mat.m[3][1], mat.m31, epsilon);
	st_check_float(mat.m[3][2], mat.m32, epsilon);
	st_check_float(mat.m[3][3], mat.m33, epsilon);
}

void test_mat4() {
	// Make sure that the values end up where they belong. They're transposed by
	// the compiler during initialization.
	mat4_t mat = mat4(
		 1,  2,  3,  4,
		 5,  6,  7,  8,
		 9, 10, 11, 12,
		13, 14, 15, 16
	);
	
	float epsilon = 0.0001;
	st_check_float(mat.m00,  1, epsilon);
	st_check_float(mat.m01,  5, epsilon);
	st_check_float(mat.m02,  9, epsilon);
	st_check_float(mat.m03, 13, epsilon);
	
	st_check_float(mat.m10,  2, epsilon);
	st_check_float(mat.m11,  6, epsilon);
	st_check_float(mat.m12, 10, epsilon);
	st_check_float(mat.m13, 14, epsilon);
	
	st_check_float(mat.m20,  3, epsilon);
	st_check_float(mat.m21,  7, epsilon);
	st_check_float(mat.m22, 11, epsilon);
	st_check_float(mat.m23, 15, epsilon);
	
	st_check_float(mat.m30,  4, epsilon);
	st_check_float(mat.m31,  8, epsilon);
	st_check_float(mat.m32, 12, epsilon);
	st_check_float(mat.m33, 16, epsilon);
}

void test_m4_identity() {
	mat4_t mat = m4_identity();
	
	st_check_matrix(mat, mat4(
		1,  0,  0,  0,
		0,  1,  0,  0,
		0,  0,  1,  0,
		0,  0,  0,  1
	));
}

void test_m4_translation() {
	mat4_t mat = m4_translation(vec3(7, 5, 3));
	
	st_check_matrix(mat, mat4(
		1,  0,  0,  7,
		0,  1,  0,  5,
		0,  0,  1,  3,
		0,  0,  0,  1
	));
}

void test_m4_scaling() {
	mat4_t mat = m4_scaling(vec3(7, 5, 3));
	
	st_check_matrix(mat, mat4(
		7,  0,  0,  0,
		0,  5,  0,  0,
		0,  0,  3,  0,
		0,  0,  0,  1
	));
}

void test_m4_rotation_x() {
	mat4_t mat = m4_rotation_x(M_PI * 0.5);
	st_check_matrix(mat, mat4(
		1,  0,  0,  0,
		0,  0, -1,  0,
		0,  1,  0,  0,
		0,  0,  0,  1
	));
}

void test_m4_rotation_y() {
	mat4_t mat = m4_rotation_y(M_PI * 0.5);
	st_check_matrix(mat, mat4(
		 0,  0,  1,  0,
		 0,  1,  0,  0,
		-1,  0,  0,  0,
		 0,  0,  0,  1
	));
}

void test_m4_rotation_z() {
	mat4_t mat = m4_rotation_z(M_PI * 0.5);
	st_check_matrix(mat, mat4(
		 0, -1,  0,  0,
		 1,  0,  0,  0,
		 0,  0,  1,  0,
		 0,  0,  0,  1
	));
}

void test_m4_mul() {
	mat4_t a = m4_translation(vec3(3, 7, 5));
	mat4_t b = m4_translation(vec3(2, 6, 4));
	mat4_t mat = m4_mul(a, b);
	
	st_check_matrix(mat, mat4(
		1,  0,  0,  3 + 2,
		0,  1,  0,  7 + 6,
		0,  0,  1,  5 + 4,
		0,  0,  0,    1
	));
	
	// Combinations covered by test_m4_invert_affine()
}

void test_m4_mul_dir() {
	// Rotate a vector by an angle and check if that's the angle between the
	// original and rotated vector.
	float rad = M_PI * 0.5;
	mat4_t mat = m4_rotation_x(rad);
	vec3_t a = vec3(0, 1, 0);
	vec3_t b = m4_mul_dir(mat, a);
	
	float rad_after_rotation = acosf( v3_dot(a, b) );
	st_check_float(rad_after_rotation, rad, 0.001);
}

void test_m4_mul_pos() {
	// Tested by test_m4_lookat() and test_m4_perspective()
	// (including division by w).
}

void test_m4_rotation() {
	// Rotate a vector by an angle and check if that's the angle between the
	// original and rotated vector. vec3(2, 0, 0) also tests normalization of
	// axis vector.
	
	// Rotate y-axis around the x-axis
	float rad = M_PI * 0.5;
	mat4_t mat = m4_rotation(rad, vec3(2, 0, 0));
	vec3_t a = vec3(0, 1, 0);
	vec3_t b = m4_mul_dir(mat, a);
	
	float rad_after_rotation = v3_angle_between(a, b);
	st_check_float(rad_after_rotation, rad, 0.001);
	st_check_float(b.x, 0, 0.0001);
	st_check_float(b.y, 0, 0.0001);
	st_check_float(b.z, 1, 0.0001);
	
	// Rotate x-axis around the y-axis
	mat = m4_rotation(rad, vec3(0, 1, 0));
	a = vec3(1, 0, 0);
	b = m4_mul_dir(mat, a);
	rad_after_rotation = v3_angle_between(a, b);
	st_check_float(rad_after_rotation, rad, 0.001);
	st_check_float(b.x,  0, 0.0001);
	st_check_float(b.y,  0, 0.0001);
	st_check_float(b.z, -1, 0.0001);
	
	// Rotate a point around the x-axis
	vec3_t axis = vec3(1, 0, 0);
	mat = m4_rotation(rad, axis);
	a = vec3(1, 1, 1);
	b = m4_mul_dir(mat, a);
	// Project a and rotated vector onto rotation axis and see if they're the same
	vec3_t a_proj = v3_proj(a, axis);
	vec3_t b_proj = v3_proj(b, axis);
	st_check_float(a_proj.x, b_proj.x, 0.0001);
	st_check_float(a_proj.y, b_proj.y, 0.0001);
	st_check_float(a_proj.z, b_proj.z, 0.0001);
	// Calculate vectors perpendicular to our roation axis and check the angle
	// between those two.
	vec3_t a_perp = v3_sub(a, a_proj);
	vec3_t b_perp = v3_sub(b, b_proj);
	rad_after_rotation = v3_angle_between(a_perp, b_perp);
	st_check_float(rad_after_rotation, rad, 0.001);
	
	// Do the same but calculate the angle between the original and rotated vector
	// using cross products. Calculate the cross products between the vectors and
	// the axis to get vectors orthogonal to the rotation axis. These vectors are
	// on a circle perpendicular to the rotation axis and we can determine the
	// angle between them on that circle. Simpler than explicit projection.
	vec3_t a_cross = v3_cross(a, axis);
	vec3_t b_cross = v3_cross(b, axis);
	rad_after_rotation = v3_angle_between(a_cross, b_cross);
	st_check_float(rad_after_rotation, rad, 0.001);
	
	// Rotate a point on the rotation axis itself. It should be rotated onto itself.
	mat = m4_rotation(rad, axis);
	a = vec3(0.5, 0, 0);
	b = m4_mul_dir(mat, a);
	st_check_float(a.x, b.x, 0.0001);
	st_check_float(a.y, b.y, 0.0001);
	st_check_float(a.z, b.z, 0.0001);
}

void test_m4_transpose() {
	mat4_t mat = m4_transpose(mat4(
		 1,  2,  3,  4,
		 5,  6,  7,  8,
		 9, 10, 11, 12,
		13, 14, 15, 16
	));
	st_check_matrix(mat, mat4(
		 1,  5,  9, 13,
		 2,  6, 10, 14,
		 3,  7, 11, 15,
		 4,  8, 12, 16
	));
}

void test_m4_fprintp() {
	char* text_ptr = NULL;
	size_t text_size = 0;
	FILE* output = open_memstream(&text_ptr, &text_size);
	
	mat4_t mat = mat4(
		 1,  2,  3,  4.333,
		 5,  6,  7,  8.777777,
		 9, 10, 11, 12,
		13, 14, 15, 16
	);
	m4_fprintp(output, mat, 10, 4);
	fclose(output);
	
	char* expected = ""
		"|     1.0000     2.0000     3.0000     4.3330 |\n"
		"|     5.0000     6.0000     7.0000     8.7778 |\n"
		"|     9.0000    10.0000    11.0000    12.0000 |\n"
		"|    13.0000    14.0000    15.0000    16.0000 |\n";
	st_check_str(text_ptr, expected);
}

void test_m4_ortho() {
	mat4_t projection = m4_ortho(3, 6, 5, 7, -100, 50);
	vec3_t a = vec3(4.5, 6, 0);
	vec3_t a_expected = vec3(0, 0, -1.0f/3);
	vec3_t b = vec3(4, 6.5, 10);
	vec3_t b_expected = vec3(-1.0f/3, 0.5, -0.466666);
	vec3_t c = vec3(5, 5, -80);
	vec3_t c_expected = vec3(1.0f/3, -1, 0.733333);
	
	vec3_t a_proj = m4_mul_pos(projection, a);
	st_check_vec3(a_proj, a_expected, 0.0001);
	vec3_t b_proj = m4_mul_pos(projection, b);
	st_check_vec3(b_proj, b_expected, 0.0001);
	vec3_t c_proj = m4_mul_pos(projection, c);
	st_check_vec3(c_proj, c_expected, 0.0001);
}

void test_m4_perspective() {
	mat4_t projection = m4_perspective(60, 4.0/3.0, 1, 10);
	// Point in the center and right at the front
	vec3_t a = vec3(0, 0, -1);
	vec3_t a_expected = vec3(0, 0, -1);
	// Point upwards and almost at the back
	vec3_t b = vec3(0, 4, -9);
	vec3_t b_expected = vec3(0, 0.76, 0.97);
	// Point to the right and at the back
	vec3_t c = vec3( 7, 0, -10);
	vec3_t c_expected = vec3(0.91, 0, 1);
	// Point in the middle of the lower left quadrant and more than halfway to the back
	vec3_t d = vec3(-3, -2, -5);
	vec3_t d_expected = vec3(-0.78, -0.7, 0.78);
	
	vec3_t a_proj = m4_mul_pos(projection, a);
	st_check_vec3(a_proj, a_expected, 0.01);
	vec3_t b_proj = m4_mul_pos(projection, b);
	st_check_vec3(b_proj, b_expected, 0.01);
	vec3_t c_proj = m4_mul_pos(projection, c);
	st_check_vec3(c_proj, c_expected, 0.01);
	vec3_t d_proj = m4_mul_pos(projection, d);
	st_check_vec3(d_proj, d_expected, 0.01);
}

/**
 * This test takes a 1x1x1 box at the origin and looks at it from a different
 * point. The center and several corners are computed and compared with manual
 * calculations.
 * 
 * See P1020781.JPG
 */
void test_m4_look_at() {
	vec3_t from = vec3(0, 5, 5), to = vec3(0, 0, 0), up = vec3(0, 1, 0);
	vec3_t a = vec3(   0,    0,    0), a_expected = vec3(   0,             0, -sqrtf(50)               );
	vec3_t b = vec3( 0.5, -0.5,  0.5), b_expected = vec3( 0.5, -sqrtf(2) / 2, -sqrtf(50)               );
	vec3_t c = vec3(-0.5,  0.5,  0.5), c_expected = vec3(-0.5,             0, -sqrtf(50) + sqrtf(2) / 2);
	vec3_t d = vec3(-0.5, -0.5, -0.5), d_expected = vec3(-0.5,             0, -sqrtf(50) - sqrtf(2) / 2);
	mat4_t camera = m4_look_at(from, to, up);
	
	vec3_t a_trans = m4_mul_pos(camera, a);
	st_check_vec3(a_trans, a_expected, 0.01);
	vec3_t b_trans = m4_mul_pos(camera, b);
	st_check_vec3(b_trans, b_expected, 0.01);
	vec3_t c_trans = m4_mul_pos(camera, c);
	st_check_vec3(c_trans, c_expected, 0.01);
	vec3_t d_trans = m4_mul_pos(camera, d);
	st_check_vec3(d_trans, d_expected, 0.01);
}

void test_m4_invert_affine() {
	mat4_t translation = m4_translation(vec3(3, 5, 7));
	mat4_t inv_translation = m4_invert_affine(translation);
	st_check_matrix(inv_translation, mat4(
		 1,  0,  0, -3,
		 0,  1,  0, -5,
		 0,  0,  1, -7,
		 0,  0,  0,  1
	));
	
	mat4_t scale = m4_scaling(vec3(0.5, 2, 0.5));
	mat4_t inv_scale = m4_invert_affine(scale);
	st_check_matrix(inv_scale, mat4(
		 2,  0,  0,  0,
		 0, 0.5, 0,  0,
		 0,  0,  2,  0,
		 0,  0,  0,  1
	));
	
	mat4_t rotation = m4_rotation(M_PI/2, vec3(1, 0, 0));
	mat4_t inv_rotation = m4_invert_affine(rotation);
	st_check_matrix(inv_rotation, m4_rotation(-M_PI/2, vec3(1, 0, 0)));
	
	vec3_t p = vec3(1, 2, 3);
	mat4_t combined = m4_mul( m4_translation(vec3(5, 5, 5)), m4_rotation(M_PI*1.0f/4, vec3(1, 0, 5)) );
	combined = m4_mul( combined, m4_scaling(vec3(0.5, 2, 0.5)) );
	mat4_t inv_combined = m4_invert_affine(combined);
	
	vec3_t transformed_p = m4_mul_pos(combined, p);
	vec3_t back_transformed_p = m4_mul_pos(inv_combined, transformed_p);
	
	st_check_vec3(back_transformed_p, p, 0.00001);
}


int main() {
	st_run(test_matrix_memory_layout);
	st_run(test_mat4);
	st_run(test_m4_identity);
	st_run(test_m4_translation);
	st_run(test_m4_scaling);
	st_run(test_m4_rotation_x);
	st_run(test_m4_rotation_y);
	st_run(test_m4_rotation_z);
	st_run(test_m4_mul);
	st_run(test_m4_mul_dir);
	st_run(test_m4_mul_pos);
	st_run(test_m4_rotation);
	st_run(test_m4_transpose);
	st_run(test_m4_fprintp);
	st_run(test_m4_ortho);
	st_run(test_m4_perspective);
	st_run(test_m4_look_at);
	st_run(test_m4_invert_affine);
	return st_show_report();
}