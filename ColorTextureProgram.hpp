#pragma once

#include "GL.hpp"
#include "Load.hpp"

//Shader program that draws transformed, vertices tinted with vertex colors:
struct ColorTextureProgram {
	ColorTextureProgram();
	~ColorTextureProgram();

	GLuint program = 0;
	//Attribute (per-vertex variable) locations:
	GLuint Color_vec3 = -1U;
	//Uniform (per-invocation variable) locations:
	GLuint OBJECT_TO_CLIP_mat4 = -1U;
	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< ColorTextureProgram > color_texture_program;
