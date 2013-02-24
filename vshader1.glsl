#version 150

uniform mat4 projection_matrix;
uniform mat4 model_matrix;
uniform mat4 shadow_matrix;

in vec4 vPosition;
in vec4 vTexCoord;
out vec2 texCoord;

void main() {
	texCoord = vTexCoord.xz; // want x/z to map to s/t tex coords
	gl_Position = projection_matrix * shadow_matrix * model_matrix * vPosition;
}
