
#ifndef __MESH_H_
#define __MESH_H_

#include "Angel.h"
#include <algorithm>

using std::string;
using std::cout;
using std::endl;

class BoundingBox {
	private:
		vec3 min, max;
		vec4* vertices;
		vec4* points; // points describing triangles of bounding box
		unsigned numPoints;
		unsigned pointsIndex;
		bool dirty;

	public:
		BoundingBox(vec4 initialPoint) {
			vertices = new vec4[8]; // cube has 8 corners
			numPoints = 3 * 2 * 6; // 3 points per tri, 2 tri per face, 6 faces
			points = new vec4[numPoints];
			for(int i = 0; i < 3; i++) {
				max[i] = min[i] = initialPoint[i];
			}
		}

		void addContainedVertex(vec4 vert) {
			for(int i = 0; i < 3; i++) {
				if(vert[i] < min[i]) {
					min[i] = vert[i];
					dirty = true;
				}
				if(vert[i] > max[i]) {
					max[i] = vert[i];
					dirty = true;
				}
			}
		}

		void quad(int a, int b, int c, int d) {
			points[pointsIndex] = vertices[a]; pointsIndex++;
			points[pointsIndex] = vertices[b]; pointsIndex++;
			points[pointsIndex] = vertices[c]; pointsIndex++;
			points[pointsIndex] = vertices[a]; pointsIndex++;
			points[pointsIndex] = vertices[c]; pointsIndex++;
			points[pointsIndex] = vertices[d]; pointsIndex++;
		}

		vec4* getPoints() {
			if(!dirty) {
				return points;
			}

			// create vertices based on min and max
			vertices[0] = vec4(min.x, min.y, max.z, 1);
			vertices[1] = vec4(min.x, max.y, max.z, 1);
			vertices[2] = vec4(max.x, max.y, max.z, 1);
			vertices[3] = vec4(max.x, min.y, max.z, 1);
			vertices[4] = vec4(min.x, min.y, min.z, 1);
			vertices[5] = vec4(min.x, max.y, min.z, 1);
			vertices[6] = vec4(max.x, max.y, min.z, 1);
			vertices[7] = vec4(max.x, min.y, min.z, 1);

			// add to points to make faces
			pointsIndex = 0;
			quad(1, 0, 3, 2);
			quad(2, 3, 7, 6);
			quad(3, 0, 4, 7);
			quad(6, 5, 1, 2);
			quad(4, 5, 6, 7);
			quad(5, 4, 0, 1);

			dirty = false;
			return points;
		}

		unsigned getNumPoints() {
			return numPoints;
		}

		vec3 getSize() {
			return max - min;
		}

		float getMaxSize() {
			vec3 size = getSize();
			return std::max(std::max(size[0], size[1]), size[2]);
		}

		vec3 getMin() {
			return vec3(min);
		}

		vec3 getMax() {
			return vec3(max);
		}

		vec4 getCenter() {
			return min + getSize()/2;
		}

		~BoundingBox() {
			delete vertices;
			delete points;
		}
};

// holds vertex list and point data to be sent to GPU
class Mesh {
	private:
		vec4* vertices;
		vec4* points;
		unsigned vertIndex;
		unsigned pointIndex;
		unsigned lineIndex;
		unsigned numPoints;
		unsigned numNormalLinePoints;
		unsigned drawOffset; // for external use
		vec4* normals;
		string name;
		BoundingBox* box;
		vec4* normalLines;
		float maxSize;

		// add 3 identical normal vectors to normals array using newell method
		// also add line segments to normalLines
		void addNormal(vec4 a, vec4 b, vec4 c) {
			vec4 normal(0, 0, 0, 0);
			// newell method
			vec4 verts[3] = {a, b, c};
			for(int i = 0; i < 3; i++) {
				vec4 current = verts[i];
				vec4 next = verts[(i + 1) % 3];
				normal.x += (current.y - next.y)*(current.z + next.z);
				normal.y += (current.z - next.z)*(current.x + next.x);
				normal.z += (current.x - next.x)*(current.y + next.y);
			}
			normal = normalize(normal);
			normals[pointIndex] = normals[pointIndex + 1] = normals[pointIndex + 2] = normal;
			
			// add a line to normalLines by finding center of face,
			// adding a line through center along normal extending out by maxSize
			if(maxSize == 0) {
				maxSize = box->getMaxSize();
			}
			vec4 center = (a + b + c) / 3;
			normalLines[lineIndex] = center;
			normalLines[++lineIndex] = center + (maxSize/20 * normal);
			lineIndex++;
		}
	
	public:
		Mesh(string _name, unsigned numVertices) {
			name = _name;
			vertices = new vec4[numVertices];
			vertIndex = 0;
			maxSize = 0;
			box = NULL;
			normals = points = normalLines = NULL;
		}

		string getName() {
			return name;
		}

		void addVertex(vec4 vert) {
			vertices[vertIndex] = vert;
			vertIndex++;
			if(box == NULL) {
				box = new BoundingBox(vert);
			}
			box->addContainedVertex(vert);
		}

		void startTriangles(unsigned numTriangles) {
			numPoints = numTriangles * 3;
			points = new vec4[numPoints];
			normals = new vec4[numPoints];
			numNormalLinePoints = numTriangles * 2; // two points per face
			normalLines = new vec4[numNormalLinePoints];
			pointIndex = 0;
			lineIndex = 0;
		}

		void addTriangle(unsigned a, unsigned b, unsigned c) {
			unsigned origIndex = pointIndex;
			points[pointIndex] = vertices[a]; pointIndex++;
			points[pointIndex] = vertices[b]; pointIndex++;
			points[pointIndex] = vertices[c];
			pointIndex = origIndex;
			addNormal(vertices[a], vertices[b], vertices[c]);
			pointIndex = origIndex + 3;
		}

		unsigned getNumPoints() {
			return numPoints;
		}

		unsigned getNumNormalLinePoints() {
			return numNormalLinePoints;
		}

		GLsizeiptr getNumBytes() {
			return sizeof(points[0]) * numPoints;
		}

		vec4* getPoints() {
			return points;
		}

		vec4* getNormals() {
			return normals;
		}

		vec4* getNormalLines() {
			return normalLines;
		}

		unsigned getDrawOffset() {
			return drawOffset;
		}

		void setDrawOffset(unsigned offset) {
			drawOffset = offset;
		}

		BoundingBox* getBoundingBox() {
			return box;
		}

		~Mesh() {
			delete vertices;
			if(points != NULL) {
				delete points;
			}
			if(normals != NULL) {
				delete normals;
			}
			if(box != NULL) {
				delete box;
			}
		}

};

#endif

