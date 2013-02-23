
#ifndef __SCENE_H_
#define __SCENE_H_

#include "LSystemRenderer.hpp"

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
		Camera camera;
		GLuint program;
		vector<Mesh*> meshes;
		Mesh* cow;
		Mesh* car;
		Mesh* ground;
		
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

			camera.lookAt(vec3(20, 50, 20), vec3(-20, 20, -20), vec3(0, 1, 0));
			updatePerspective();
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
			
			if(lsysRenderer.forestMode()) {
				GLuint colorLoc = glGetUniformLocationARB(program, "inColor");
				GLuint modelLoc = glGetUniformLocationARB(program, "model_matrix");


				glUniform4fv(colorLoc, 1, vec4(0.5, 1, 0.5, 1));
				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, mat4());
				glDrawArrays(GL_TRIANGLES, ground->getDrawOffset(), ground->getNumPoints());

				glUniform4fv(colorLoc, 1, vec4(1, 1, 1, 1));

				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, Scale(3));
				glDrawArrays(GL_TRIANGLES, cow->getDrawOffset(), cow->getNumPoints());

				float yAdjust = -1 * car->getBoundingBox()->getMin().y;
				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, RotateY(-60) * Translate(-25, yAdjust, 0));
				glDrawArrays(GL_TRIANGLES, car->getDrawOffset(), car->getNumPoints());
			}

			lsysRenderer.display();


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
};

#endif

