#version 150

uniform vec4 inColor;
uniform sampler2D texture;
uniform bool useTexture;
uniform bool useExponentialFog;
in vec2 texCoord;
out vec4 fColor;


void main() {
	if(useTexture) {
		fColor = texture2D(texture, texCoord);
	} else {
		fColor = inColor;
	}
	float dist = abs(gl_FragCoord.z / gl_FragCoord.w);
	float fogFactor = 1;
	if(useExponentialFog) {
		fogFactor = exp(-0.05 * dist);
	} else { // linear
		float maxFogDist = 100;
		float minFogDist = 0;
		fogFactor = (maxFogDist - dist) / (maxFogDist - minFogDist);
	}
	fogFactor = clamp(fogFactor, 0, 1);
	fColor = mix(vec4(0,0,0,1), fColor, fogFactor);
}

