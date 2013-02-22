
#ifndef __PLYREADER_H_
#define __PLYREADER_H_

#include <sstream>

#include "Mesh.hpp"
#include "textfile.cpp"
#include "ReaderException.hpp"

using std::string;
using std::stringstream;
using std::endl;
using std::cout;

// reads a PLY file
class PLYReader {
	private:
		string content;
		Mesh* mesh;
		int verticesLeft;
		int trianglesLeft;
		const char* filename;

	public:
		PLYReader(const char* _filename) {
			content = string(textFileRead(_filename));
			filename = _filename;
		}

		// returns a Mesh containing data from ply file
		// caller is responsible for deleting Mesh when done
		Mesh* read() {
			verticesLeft = -1;
			trianglesLeft = -1;
			stringstream stream(content, stringstream::in);
			string line;
			unsigned lineNum = 0;
			while(getline(stream, line)) {
				parseLine(line, lineNum);
				lineNum++;
			}
			if(verticesLeft != 0) {
				throw ReaderException("Not enough vertices");
			}
			if(trianglesLeft != 0) {
				throw ReaderException("Not enough triangles");
			}
			return mesh;
		}

		void parseLine(string line, unsigned lineNum) {
			if(lineNum == 0) {
				if(!startsWith(line, "ply")) {
					throw ReaderException("Line 0 doesn't start with Ply");
				}
				return;
			}
			if(startsWith(line, "format ascii 1.0")
					|| startsWith(line, "property")
					|| startsWith(line, "end_header")) {
				return;
			}

			stringstream ss(stringstream::in);
			ss.str(line);
			string garbage;
			
			if(startsWith(line, "element vertex")) {
				ss >> garbage >> garbage >> verticesLeft;
				mesh = new Mesh(filename, verticesLeft);
				return;
			}

			if(startsWith(line, "element face")) {
				ss >> garbage >> garbage >> trianglesLeft;
				mesh->startTriangles(trianglesLeft);
				return;
			}

			if(verticesLeft > 0) {
				vec4 vertex;
				ss >> vertex.x >> vertex.y >> vertex.z;
				vertex.w = 1;
				mesh->addVertex(vertex);
				verticesLeft--;
				return;
			}

			if(trianglesLeft > 0) {
				unsigned faces, a, b, c;
				ss >> faces >> a >> b >> c;
				mesh->addTriangle(a, b, c);
				trianglesLeft--;
				return;
			}

			throw ReaderException("Something went wrong");

		}

		static bool startsWith(string str, string prefix) {
			return str.substr(0, prefix.size()) == prefix;
		}

};

#endif

