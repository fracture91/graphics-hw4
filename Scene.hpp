
#ifndef __SCENE_H_
#define __SCENE_H_

#include "LSystemRenderer.hpp"
#include "bmpread.c"

// defines a camera whose coordinate system is along u/v/n axes
// (rather than x/y/z) at eye position
class Camera {
	private:
		vec3 eye;
		vec3 u, v, n;
		mat4 viewMatrix;

		// should be called after eye, u, v, n change
		void updateViewMatrix() {
			viewMatrix = mat4();
			viewMatrix[0] = vec4(u, -dot(eye, u));
			viewMatrix[1] = vec4(v, -dot(eye, v));
			viewMatrix[2] = vec4(n, -dot(eye, n));
		}

		void updateUVN() {
			u = vec3(viewMatrix[0].x, viewMatrix[0].y, viewMatrix[0].z);
			v = vec3(viewMatrix[1].x, viewMatrix[1].y, viewMatrix[1].z);
			n = vec3(viewMatrix[2].x, viewMatrix[2].y, viewMatrix[2].z);
		}

	public:
		enum Axis { U, V, N };
		Camera() {
			viewMatrix = mat4();
			eye = vec3(0);
			u = vec3(1, 0, 0);
			v = vec3(0, 1, 0);
			n = vec3(0, 0, 1);
		}

		void lookAt(vec3 eye, vec3 at, vec3 up) {
			this->eye = eye;
			viewMatrix = LookAt(eye, at, up);
			updateUVN();
		}

		mat4 getViewMatrix() {
			return viewMatrix;
		}

		void slide(vec3 delta) {
			mat3 uvn = transpose(mat3(u, v, n));
			eye += uvn * delta;
			updateViewMatrix();
		}

		void rotate(Axis axis, float degrees) {
			float rads = DegreesToRadians * degrees;
			float c = cos(rads);
			float s = sin(rads);
			mat3 newUVN(u, v, n);
			switch(axis) {
				case U:
					newUVN[1] = vec3(c*v + s*n);
					newUVN[2] = vec3(-s*v + c*n);
					break;
				case V:
					newUVN[0] = vec3(c*u - s*n);
					newUVN[2] = vec3(s*u + c*n);
					break;
				case N:
					newUVN[0] = vec3(c*u + s*v);
					newUVN[1] = vec3(-s*u + c*v);
					break;
			}
			u = newUVN[0];
			v = newUVN[1];
			n = newUVN[2];
			updateViewMatrix();
		}

		void pitch(float degrees) {
			rotate(U, degrees);
		}

		void yaw(float degrees) {
			rotate(V, degrees);
		}

		void roll(float degrees) {
			rotate(N, degrees);
		}
};

class Scene {
	private:
		int screenWidth;
		int screenHeight;
		mat4 perspective;
		mat4 shadow;
		bool showShadows;
		bool useExponentialFog;
		Camera camera;
		GLuint program;
		vector<Mesh*> meshes;
		Mesh* cow;
		Mesh* car;
		Mesh* ground;
		GLuint textures[2];
		bool showGrass;

		void updatePerspective() {
			if(screenHeight == 0) {
				perspective = mat4(); // don't want to divide by zero...
				return;
			}
			perspective = mat4()
				* Perspective(90, (float)screenWidth/screenHeight, 0.0000001, 100000);
		}

		// returns next empty space in buffer
		GLuint bufferMeshes(GLuint bufferStart, vector<Mesh*>* meshes) {
			for (vector<Mesh*>::const_iterator i = meshes->begin(); i != meshes->end(); ++i) {
				Mesh* mesh = *i;
				GLsizeiptr bytes = mesh->getNumBytes();
				mesh->setDrawOffset(bufferStart / sizeof(mesh->getPoints()[0]));
				glBufferSubData(GL_ARRAY_BUFFER, bufferStart, bytes, mesh->getPoints());
				bufferStart += bytes;
			}
			return bufferStart;
		}

		void assignTexture(string path, GLuint texName) {
			bmpread_t bitmap;
			if(!bmpread(path.c_str(), 0, &bitmap)) {
				throw runtime_error("failed to load texture: " + path);
			}
			glBindTexture(GL_TEXTURE_2D, texName);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap.width, bitmap.height, 0,
					GL_RGB, GL_UNSIGNED_BYTE, bitmap.rgb_data);
			bmpread_free(&bitmap);
		}

		void setUpTextures() {
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(2, textures);

			glUniform1i(glGetUniformLocation(program, "texture"), 0);

			// just reuse the vertex as a texture coord
			GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
			glEnableVertexAttribArray(vTexCoord);
			glVertexAttribPointer(vTexCoord, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

			assignTexture("textures/grass.bmp", textures[0]);
			assignTexture("textures/stones.bmp", textures[1]);
			showGrass = false;
			toggleGrass();
		}

		// use the shadow matrix in the shader, or not (ignored if !showShadows)
		void setUseShadow(bool use) {
			static vec4 oldColor;
			static bool oldUseState = false;
			// need to backup/restore old color being used
			if(use != oldUseState) {
				GLuint colorLoc = glGetUniformLocationARB(program, "inColor");
				if(use) {
					glGetUniformfv(program, colorLoc, oldColor);
					glUniform4fv(colorLoc, 1, vec4(0, 0, 0, 1)); // render shadow as black
				} else {
					glUniform4fv(colorLoc, 1, oldColor);
				}
				oldUseState = use;
			}
			GLuint shadowLoc = glGetUniformLocationARB(program, "shadow_matrix");
			glUniformMatrix4fv(shadowLoc, 1, GL_TRUE, use && showShadows ? shadow : mat4());
		}

	public:
		LSystemRenderer& lsysRenderer;

		Scene(GLuint program, LSystemRenderer& lr):lsysRenderer(lr) {
			this->program = program;

			PLYReader cowReader("meshes/cow.ply");
			cow = cowReader.read();
			meshes.push_back(cow);

			PLYReader carReader("meshes/big_porsche.ply");
			car = carReader.read();
			meshes.push_back(car);

			// our randomly placed trees and floor plane will be in this volume
			vec3 max(10, 0, 10);
			vec3 min(-30, 0, -30);
			lsysRenderer.showAllSystemsRandomly(min, max);

			// use the bounding box to generate a cube, then make a Mesh out of it
			BoundingBox* box = new BoundingBox(min);
			box->addContainedVertex(max);

			// less efficient since we'll have duplicate vertices... oh well
			ground = new Mesh("ground", box->getNumPoints());
			int numTriangles = box->getNumPoints() / 3;
			ground->startTriangles(numTriangles);
			for(int i = 0; i < numTriangles; i++) {
				int pointIndex = i * 3;
				for(int j = 0; j < 3; j++) {
					ground->addVertex(box->getPoints()[pointIndex + j]);
				}
				ground->addTriangle(pointIndex, pointIndex + 1, pointIndex + 2);
			}
			meshes.push_back(ground);

			setUpTextures();
			camera.lookAt(vec3(20, 50, 20), vec3(-20, 20, -20), vec3(0, 1, 0));
			updatePerspective();

			useExponentialFog = true;
			toggleExponentialFog(); // set to linear by default

			showShadows = true;
			vec3 light(5000, 10000, 0);
			mat4 m;
			// put shadow .01 above ground to avoid z fighting
			m[3].y = -1.0 / (light.y - 0.01);
			m[3].w = 0;
			shadow = Translate(light) * m * Translate(-light);
		}

		void bufferPoints() {
			GLsizeiptr totalBytes = lsysRenderer.getTotalBytes();
			for (vector<Mesh*>::const_iterator i = meshes.begin(); i != meshes.end(); ++i) {
				totalBytes += (*i)->getNumBytes();
			}

			glBufferData(GL_ARRAY_BUFFER, totalBytes, NULL, GL_STATIC_DRAW);

			GLuint bufferStart = bufferMeshes(0, &meshes);
			bufferStart = bufferMeshes(bufferStart, lsysRenderer.getMeshes());

			GLuint posLoc = glGetAttribLocation(program, "vPosition");
			glEnableVertexAttribArray(posLoc);
			glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
		}

		void display() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_DEPTH_TEST);
			GLuint projLoc = glGetUniformLocationARB(program, "projection_matrix");
			glUniformMatrix4fv(projLoc, 1, GL_TRUE, perspective * camera.getViewMatrix());
			setUseShadow(false);

			if(lsysRenderer.forestMode()) {
				GLuint colorLoc = glGetUniformLocationARB(program, "inColor");
				GLuint modelLoc = glGetUniformLocationARB(program, "model_matrix");
				GLuint useTextureLoc = glGetUniformLocationARB(program, "useTexture");

				glUniform1i(useTextureLoc, true);
				glUniform4fv(colorLoc, 1, vec4(0.5, 1, 0.5, 1));
				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, mat4());
				glDrawArrays(GL_TRIANGLES, ground->getDrawOffset(), ground->getNumPoints());
				glUniform1i(useTextureLoc, false);

				glUniform4fv(colorLoc, 1, vec4(1, 1, 1, 1));

				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, Scale(3));
				glDrawArrays(GL_TRIANGLES, cow->getDrawOffset(), cow->getNumPoints());
				setUseShadow(true); // draw again with shadows
				glDrawArrays(GL_TRIANGLES, cow->getDrawOffset(), cow->getNumPoints());
				setUseShadow(false);


				float yAdjust = -1 * car->getBoundingBox()->getMin().y;
				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, RotateY(-60) * Translate(-25, yAdjust, 0));
				glDrawArrays(GL_TRIANGLES, car->getDrawOffset(), car->getNumPoints());
				setUseShadow(true);
				glDrawArrays(GL_TRIANGLES, car->getDrawOffset(), car->getNumPoints());
				setUseShadow(false);
			}

			lsysRenderer.display();
			setUseShadow(true);
			lsysRenderer.display(false); // make sure it doesn't override shadow color
			setUseShadow(false);


			glDisable(GL_DEPTH_TEST); 

			// output to hardware, double buffered
			glFlush();
			glutSwapBuffers();
		}

		void reshape(int screenWidth, int screenHeight) {
			this->screenWidth = screenWidth;
			this->screenHeight = screenHeight;
			glViewport(0, 0, screenWidth, screenHeight);
			updatePerspective();
		}

		Camera& getCamera() {
			return camera;
		}

		void toggleGrass() {
			showGrass = !showGrass;
			glBindTexture(GL_TEXTURE_2D, showGrass ? textures[0] : textures[1]);
		}

		void toggleShadows() {
			showShadows = !showShadows;
		}

		void toggleExponentialFog() {
			useExponentialFog = !useExponentialFog;
			glUniform1i(glGetUniformLocationARB(program, "useExponentialFog"), useExponentialFog);
		}
};

#endif

