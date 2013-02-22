#version 150

uniform mat4 projection_matrix;
uniform mat4 model_matrix;

in vec4 vPosition;

void main() {
	gl_Position = projection_matrix*model_matrix*vPosition;
}
