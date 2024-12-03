#pragma once

#ifndef __MAIN_H__
#define __MAIN_H__
#define _CRT_SECURE_NO_WARNINGS

#define GAME_BGM "gamebgm.wav"

#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <random>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>
#include <math.h>
#include <vector>
#include <string>
#include <array>

#include <chrono>
void make_shaderProgram();
void make_vertexShaders();
void make_fragmentShaders();
void drawScene();
void Reshape(int w, int h);
void InitBuffer();
char* filetobuf(const char*);

GLvoid keyboard(unsigned char key, int x, int y);
GLvoid keyUp(unsigned char, int, int);
GLvoid handleEvent(unsigned char key, bool state);
GLvoid Motion(int x, int y);
GLvoid MousePoint(int button, int state, int x, int y);
GLvoid object_ok(int value);
GLvoid next_stage(int value);
GLvoid update(int value);


#endif