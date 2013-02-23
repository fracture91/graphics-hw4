#version 150

uniform vec4 inColor;
uniform sampler2D texture;
uniform bool useTexture;
in vec2 texCoord;
out vec4 fColor;


void main() {
	if(useTexture) {
		fColor = texture2D(texture, texCoord);
	} else {
		fColor = inColor;
	}
}

