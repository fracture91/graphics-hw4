#ifdef _WIN32
	#define NOMINMAX // otherwise windows.h redefines min and max...
	#include "win_dirent.h"
#else
	#include "unix_dirent.h"
#endif
#include <vector>
#include <stdlib.h>
#include <time.h>

#include "Angel.h"
#include "Mesh.hpp"
#include "PLYReader.hpp"
#include "MeshRenderer.hpp"
#include "LSystem.hpp"
#include "LSystemReader.hpp"
#include "LSystemRenderer.hpp"
#include "Scene.hpp"

// remember to prototype
void display(void);
void keyboard(unsigned char key, int x, int y);

LSystemRenderer* lsysRenderer;
Scene* scene;

using namespace std;

GLuint setUpShaders(void) {	
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	// Load shaders and use the resulting shader program
	GLuint program = InitShader("vshader1.glsl", "fshader1.glsl");
	glUseProgram(program);

	// sets the default color to clear screen
	glClearColor(0,0,0, 1.0); // black background
	return program;
}

//----------------------------------------------------------------------------
// this is where the drawing should happen
void display(void) {
	scene->display();
}

void reshape(int screenWidth, int screenHeight) {
	scene->reshape(screenWidth, screenHeight);
}

//----------------------------------------------------------------------------

// keyboard handler
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
		case 27: // ESC
			exit(EXIT_SUCCESS);
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
			lsysRenderer->showOneSystem(key - 'a');
			break;
		case 'f':
			vec3 max(10, 0, 10);
			vec3 min(-30, 0, -30);
			lsysRenderer->showAllSystemsRandomly(min, max);
			break;
	}
	glutPostRedisplay();
}

vector<string>* getFileNames(const char* path) {
	vector<string>* names = new vector<string>();
	DIR* directory;
	dirent* entry;
	if((directory = opendir(path)) != NULL) {
		while((entry = readdir(directory)) != NULL) {
			if(entry->d_name[0] == '.') {
				continue;
			}
			names->push_back(string(path) + "/" + entry->d_name);
		}
		closedir(directory);
	} else {
		throw "Couldn't open directory";
	}
	return names;
}


//----------------------------------------------------------------------------
// entry point
int main(int argc, char **argv) {
	// init glut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(512, 512);

	// get a list of all the mesh data in meshes directory
	vector<string>* names = getFileNames("lsystems");
	std::sort(names->begin(), names->end());
	vector<LSystem*> lsystems = vector<LSystem*>();
	for(vector<string>::const_iterator i = names->begin(); i != names->end(); ++i) {
		LSystemReader reader((*i).c_str());
		lsystems.push_back(reader.read());
		//lsystems[lsystems.size() - 1]->print();
	}


	// If you are using freeglut, the next two lines will check if 
	// the code is truly 3.2. Otherwise, comment them out

	// should run a test here 
	// with different cases
	// this is a good time to find out information about
	// your graphics hardware before you allocate any memory
	glutInitContextVersion(3, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	// create window
	// opengl can be incorperated into other packages like wxwidgets, fltoolkit, etc.
	glutCreateWindow("L-System Renderer");

	// init glew
	glewInit();

	GLuint program = setUpShaders();

	srand(time(NULL));
	lsystems[0]->print();
	lsysRenderer = new LSystemRenderer(program, lsystems);
	scene = new Scene(program, *lsysRenderer);
	scene->bufferPoints();
	// assign handlers
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	// should add menus
	// add mouse handler
	// add resize window functionality (should probably try to preserve aspect ratio)

	// enter the drawing loop
	// frame rate can be controlled with 
	glutMainLoop();
	return 0;
}

