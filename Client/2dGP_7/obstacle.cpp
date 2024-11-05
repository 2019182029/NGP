#include "obstacle.h"

class obstacle {
public:
	objRead objReader;
	GLint Object;
	GLuint ObstaclePosVbo;
	GLuint ObstacleNomalVbo;

	int Obstacle_initbuffer(GLuint);

}; 

int obstacle::Obstacle_initbuffer(GLuint vao) {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ObstaclePosVbo);
	glBindBuffer(GL_ARRAY_BUFFER, ObstaclePosVbo);
	glBufferData(GL_ARRAY_BUFFER, objReader.outvertex.size() * sizeof(glm::vec3), &objReader.outvertex[0], GL_STATIC_DRAW);

	glGenBuffers(1, &ObstacleNomalVbo);
	glBindBuffer(GL_ARRAY_BUFFER, ObstacleNomalVbo);
	glBufferData(GL_ARRAY_BUFFER, objReader.outnormal.size() * sizeof(glm::vec3), &objReader.outnormal[0], GL_STATIC_DRAW);
}