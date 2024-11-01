#ifndef __OBS_H__
#define __OBS_H__

#include "main.h"
#include "obj.h"

class obstacle {
public:
	objRead objReader;
	GLint Object;
	GLuint ObstaclePosVbo;
	GLuint ObstacleNomalVbo;

	int Obstacle_initbuffer(GLuint);

};

#endif