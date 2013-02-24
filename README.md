Andrew Hurle

CS 4731 - Computer Graphics

Homework 4

Displays two lindenmayer systems, two meshes, and a ground plane.  Can
change ground plane texture with 'A', toggle shadows with 'D', toggle
fog algorithm with 'F', and randomize tree positions with 'R'.  Camera
controls are TVU for slide and JKL for rotation.

The program is linked against whatever files are present on the machine.
The Zoo Lab machines contain the needed library files, so they're not
included here.

Running
=====

To compile and run on Linux:

1. `make && ./hw4`


To compile and run on Windows (Zoo Lab machines, tested on FLA21-02):

1. Open "Visual Studio Command Prompt (2010)" from the start menu
2. `cd C:\wherever\these\files\are`
3. `nmake /f Win-Makefile`
4. `hw4.exe`

Notes
=====

LSystemReader is responsible for pulling the system data out of files
and putting it into an LSystem instance.  This instance will iterate the
start string based on the grammar it is given.  The LSystem provides a
Turtle instance.  This Turtle can be given all of the commands in the
turtle string, and will modify a given transform matrix stack.
LSystemRenderer will actually give the commands to the turtle and draw
its progress to the screen as a PolyCylinder.  hw3.cpp hooks everything
up with the standard GLUT callbacks.  The Scene object from Scene.hpp
is capable of displaying the LSystems and arbitrary meshes
simultaneously.  The Camera class in Scene.hpp handles camera controls.

The camera position is in the +x/+y/+z octant, looking to -x/-y/-z, so x
and z axes both point out of the screen.

