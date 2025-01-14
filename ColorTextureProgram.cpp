#include "ColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< ColorTextureProgram > color_texture_program(LoadTagEarly);

// BOTH SHADERS REFERENCED IN: https://learnopengl.com/In-Practice/Text-Rendering
// VERTEX SHADER SOURCE: https://learnopengl.com/code_viewer_gh.php?code=src/7.in_practice/2.text_rendering/text.vs
// FRAGMENT SHADER SOURCE: https://learnopengl.com/code_viewer_gh.php?code=src/7.in_practice/2.text_rendering/text.fs
ColorTextureProgram::ColorTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
        "#version 330 core\n"
        "layout (location = 0) in vec4 vertex;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoords = vertex.zw;\n"
        "}\n"
	,
		//fragment shader:
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "void main() {\n"  
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "    color = vec4(textColor, 1.0) * sampled;\n"
        "}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	// //look up the locations of vertex attributes:
	Color_vec3 = glGetAttribLocation(program, "textColor");

	// //look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "projection");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

ColorTextureProgram::~ColorTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

