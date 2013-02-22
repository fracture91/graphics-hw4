
#ifndef __SCENE_H_
#define __SCENE_H_

#include "LSystemRenderer.hpp"

class Scene {
	private:
		int screenWidth;
		int screenHeight;
		mat4 projection;
		GLuint program;
		vector<Mesh*> meshes;
		Mesh* cow;
		Mesh* car;
		
		void resetProjection() {
			if(screenHeight == 0) {
				projection = mat4(); // don't want to divide by zero...
				return;
			}
			projection = mat4()
				* Perspective(90, (float)screenWidth/screenHeight, 0.0000001, 100000)
				* LookAt(vec3(20, 50, 20), vec3(-20, 20, -20), vec3(0, 1, 0));
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

			PLYReader carReader("meshes/big_porsche.ply");
			car = carReader.read();

			meshes.push_back(car);
			meshes.push_back(cow);

			resetProjection();
		}

		void bufferPoints() {
			GLsizeiptr totalBytes = lsysRenderer.getTotalBytes();
			totalBytes += cow->getNumBytes();
			totalBytes += car->getNumBytes();
			glBufferData(GL_ARRAY_BUFFER, totalBytes, NULL, GL_STATIC_DRAW);

			GLuint bufferStart = bufferMeshes(0, &meshes);
			bufferStart = bufferMeshes(bufferStart, lsysRenderer.getMeshes());
			
			GLuint posLoc = glGetAttribLocation(program, "vPosition");
			glEnableVertexAttribArray(posLoc);
			glVertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
		}

		void display() {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glEnable(GL_DEPTH_TEST);
			GLuint projLoc = glGetUniformLocationARB(program, "projection_matrix");
			glUniformMatrix4fv(projLoc, 1, GL_TRUE, projection);
			
			if(lsysRenderer.forestMode()) {
				GLuint colorLoc = glGetUniformLocationARB(program, "inColor");
				glUniform4fv(colorLoc, 1, vec4(1,1,1,1));

				GLuint modelLoc = glGetUniformLocationARB(program, "model_matrix");
				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, Scale(3));
				glDrawArrays(GL_TRIANGLES, cow->getDrawOffset(), cow->getNumPoints());

				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, Translate(-20, 0, -10) * RotateY(-60));
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
			resetProjection();
		}
};

#endif

