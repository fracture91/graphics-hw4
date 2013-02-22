
#ifndef __LSYSTEM_H_
#define __LSYSTEM_H_

#include <string>
#include <map>
#include <stdexcept>
#include <stack>

#include "Angel.h"

using std::string;
using std::map;
using std::pair;
using std::runtime_error;
using std::cout;
using std::endl;
using std::stack;

// contains basic drawing parameters
// modifies a given transform matrix stack according to commands
class Turtle {
	private:
		void ensureCtm() {
			if(ctm == NULL || ctm->empty()) {
				throw runtime_error("Turtle ctm must be non-null, non-empty");
			}
		}

	public:
		unsigned segmentLength;
		float thickness;
		const float defaultThickness;
		vec3 rotations;
		stack<mat4>* ctm;
		enum Axis { X, Y, Z };

		Turtle():defaultThickness(0.25) {
			segmentLength = 0;
			thickness = defaultThickness;
			rotations = vec3(0, 0, 0);
			ctm = NULL;
		}

		Turtle(const Turtle& other):defaultThickness(0.25) {
			segmentLength = other.segmentLength;
			thickness = other.thickness;
			rotations = other.rotations;
			ctm = other.ctm;
		}

		void rotate(Axis axis, bool positive) {
			ensureCtm();
			mat4 operand;
			float theta = (positive ? 1 : -1) * rotations[axis];
			if(axis == X) {
				operand = RotateX(theta);
			} else if(axis == Y) {
				operand = RotateY(theta);
			} else if(axis == Z) {
				operand = RotateZ(theta);
			}
			ctm->top() *= operand;
		}

		void turnAround() {
			ensureCtm();
			ctm->top() *= RotateY(180);
		}

		void forward() {
			ensureCtm();
			ctm->top() *= Translate(0, 0, segmentLength);
		}

		void push() {
			ensureCtm();
			ctm->push(ctm->top());
		}

		void pop() {
			ensureCtm();
			ctm->pop();
		}
};


class LSystem {
	private:
		string name;
		map<char, char> replacements;
		map<char, string> grammar;
		string turtleString;

		// apply rules to turtleString one time
		void iterateTurtleString() {
			string nextTurtle = "";
			for(string::iterator it = turtleString.begin(); it != turtleString.end(); ++it) {
				char currentChar = *it;
				map<char, string>::iterator rule = grammar.find(currentChar);
				if(rule != grammar.end()) {
					// if there is a rule for this character, replace with rhs
					nextTurtle += rule->second;
				} else {
					// no rule, char is same in newTurtle
					nextTurtle += currentChar;
				}
			}
			turtleString = nextTurtle;
		}

		// replace characters in turtleString according to replacements map
		void applyReplacements() {
			string repTurtle = "";
			for(string::iterator it = turtleString.begin(); it != turtleString.end(); ++it) {
				char currentChar = *it;
				map<char, char>::iterator rep = replacements.find(currentChar);
				if(rep != replacements.end()) {
					// if there is a replacement for this character, replace
					if(rep->second == ' ') {
						// just remove character from string
					} else {
						repTurtle += rep->second;
					}
				} else {
					// no replacement, char is same in repTurtle
					repTurtle += currentChar;
				}
			}
			turtleString = repTurtle;
		}

	public:
		Turtle protoTurtle;
		unsigned iterations;
		string start;

		LSystem(string name) {
			this->name = name;
			turtleString = "";
			start = "";
			iterations = 0;
		}

		string getName() {
			return name;
		}

		// add a replacement rule - use space for empty replacements
		void addReplacement(char target, char replacement) {
			replacements.insert(pair<char, char>(target, replacement));
		}

		// add a rule to this lsystem's grammar
		void addRule(char lhs, string rhs) {
			grammar.insert(pair<char, string>(lhs, rhs));
		}

		// get the generated turtle string
		string getTurtleString() {
			if(turtleString != "") { // already computed
				return turtleString;
			}
			if(start == "") {
				throw runtime_error("Empty start string");
			}

			turtleString = start;
			string lastTurtle;
			unsigned i = 0;
			while(i < iterations) {
				lastTurtle = turtleString;
				iterateTurtleString();
				if(turtleString == lastTurtle) {
					break; // no longer changing
				}
				i++;
			}
			applyReplacements();
			return turtleString;
		}

		Turtle* getTurtleCopy() {
			return new Turtle(protoTurtle);
		}

		void print() {
			cout << "LSystem " << name << ": " << endl <<
				"len=" << protoTurtle.segmentLength << ", " << endl <<
				"iter=" << iterations << ", " << endl <<
				"rot=" << protoTurtle.rotations << ", " << endl <<
				"reps=(";
			for (map<char, char>::const_iterator i = replacements.begin(); i != replacements.end(); ++i) {
				cout << i->first << "->" << i->second << ", ";
			}
			cout << "), " << endl <<
				"start=" << start << ", " << endl <<
				"rules=(";
			for (map<char, string>::const_iterator i = grammar.begin(); i != grammar.end(); ++i) {
				cout << i->first << " -> " << i->second << ", ";
			}
			cout << "), " << endl <<
				"turtleString=" << getTurtleString() << endl;
		}

};

#endif

