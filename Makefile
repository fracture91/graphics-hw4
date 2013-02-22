hw4: hw4.cpp vshader1.glsl fshader1.glsl Angel.h CheckError.h mat.h vec.h\
		textfile.h textfile.cpp InitShader.cpp Mesh.hpp PLYReader.hpp\
		MeshRenderer.hpp LSystemReader.hpp LSystem.hpp ReaderException.hpp\
		LSystemRenderer.hpp Scene.hpp
	g++ hw4.cpp -g -Wall -lglut -lGL -lGLEW -o hw4

clean:
	rm hw4

