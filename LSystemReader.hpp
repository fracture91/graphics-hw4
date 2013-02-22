
#ifndef __LSYSTEMREADER_H_
#define __LSYSTEMREADER_H_

#include <string>
#include <sstream>

#include "textfile.cpp"
#include "LSystem.hpp"
#include "PLYReader.hpp"

using std::string;
using std::stringstream;

enum ReaderState{LEN, ITER, ROT, REP, RULES};

class LSystemReader {
	private:
		string content;
		const char* filename;
		LSystem* lsys;
		ReaderState state;

		void parseLine(string line) {
			if(PLYReader::startsWith(line, "#")) {
				return; // comment
			}

			stringstream ss(stringstream::in);
			ss.str(line);
			string garbage;

			switch(state) {
				case LEN:
					ss >> garbage >> lsys->protoTurtle.segmentLength;
					if(garbage != "len:") {
						throw ReaderException("len expected");
					}
					state = ITER;
					break;
				case ITER:
					ss >> garbage >> lsys->iterations;
					if(garbage != "iter:") {
						throw ReaderException("iter expected");
					}
					state = ROT;
					break;
				case ROT: {
					vec3* rots = &lsys->protoTurtle.rotations;
					ss >> garbage >> rots->x >> rots->y >> rots->z;
					if(garbage != "rot:") {
						throw ReaderException("rot expected");
					}
					state = REP;
					break;
				}
				case REP:
					ss >> garbage;
					// replacements are optional, may go right to start:
					if(garbage == "start:") {
						ss >> lsys->start;
						state = RULES;
					} else if(garbage == "rep:") {
						char target, comma, replacement;
						ss >> target >> comma >> replacement;
						// char after comma is optional
						if((ss.rdstate() & stringstream::eofbit) != 0) {
							replacement = ' ';
						}
						lsys->addReplacement(target, replacement);
					} else {
						throw ReaderException("start or rep expected");
					}
					break;
				case RULES:
					char lhs;
					string rhs;
					ss >> lhs >> garbage >> rhs;
					if(garbage != ":") {
						throw ReaderException("one char and colon expected");
					}
					lsys->addRule(lhs, rhs);
					break;
			}
		}

	public:
		LSystemReader(const char* filename) {
			content = string(textFileRead(filename));
			this->filename = filename;
		}

		// construct a new LSystem from the given file
		// caller is responsible for freeing memory
		LSystem* read() {
			state = LEN;
			lsys = new LSystem(filename);
			stringstream stream(content, stringstream::in);
			string line;
			while(getline(stream, line)) {
				parseLine(line);
			}
			if(state != RULES) {
				throw ReaderException("File ended prematurely");
			}
			return lsys;
		}
};

#endif

