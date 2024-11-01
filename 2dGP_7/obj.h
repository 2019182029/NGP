#ifndef __OBJ_H__
#define __OBJ_H__

#include "main.h"

class obs {
public:
    GLfloat x{}, y{}, z{ -45.0f };

    GLfloat x_scale{ 2.0f }, y_scale{ 0.0001f }, z_scale{ 50.0f };

    GLfloat r{ 1 }, g{ 0 }, b{ 0 }, a{ 1.0 };

    objRead objReader;
    GLint Object = objReader.loadObj_normalize_center("cube.obj");

};

class obss {
public:
    GLfloat x{}, y{ 0.25f }, z{ -1.0f };
    GLfloat x_scale{ 0.25f }, y_scale{ 0.25f }, z_scale{ 0.25f };

    GLfloat r{ 1 }, g{ 0 }, b{ 0 }, a{ 1.0 };
    int jump_scale{};
    int hp = 100;

    GLuint vvbo{ cubePosVbo2 }, nvbo{ cubeNomalVbo2 };
    GLint Object{};

    void change_color(float r, float g, float b) {
        this->r = r;
        this->g = g;
        this->b = b;
    }

    void init(int PosVbo, int NomalVbo) {

        this->vvbo = PosVbo;
        this->nvbo = NomalVbo;
    }

};

class object_won {
public:
    GLfloat x{}, y{ 0.25f }, z{ -100.0f };
    GLfloat x_scale{ 0.25f }, y_scale{ 0.25f }, z_scale{ 0.25f };
    GLfloat x_move{}, y_move{};
    GLfloat r{}, g{}, b{}, a{ 1.0 };
    GLuint vvbo{}, nvbo{};
    GLfloat rotate{};
    GLint object_num{};
    int rotate_move{};


    void init(int PosVbo, int NomalVbo) {
        this->z = -100.0f;
        this->x = 0.0f;
        this->y = 2.0f;
        this->rotate_move = random_rotate(engine);

        this->r = random_color(engine);
        this->g = random_color(engine);
        this->b = random_color(engine);

        float size = random_scale(engine);

        this->x_scale = size;
        this->y_scale = size;
        this->z_scale = size;

        this->x_move = random_move(engine);
        this->y_move = random_move(engine);

        this->vvbo = PosVbo;
        this->nvbo = NomalVbo;
    }

    void move() {
        this->x += this->x_move;
        this->y += this->y_move;

        this->z += 1.0f;

        if (this->z > 5.0f)
        {
            this->init(this->vvbo, this->nvbo);
        }

        if (this->x + this->x_scale + this->x_move > 2.0f || this->x - this->x_scale + this->x_move < -2.0f)
        {
            this->x_move *= -1;
            this->rotate_move *= -1;
        }
        if (this->y + this->y_scale + this->y_move > 4.0f || this->y - this->y_scale + this->y_move < 0.0f)
        {
            this->y_move *= -1;
            this->rotate_move *= -1;
        }

        this->rotate += this->rotate_move;
    }

};

class light_set {
public:
    float rotate_light = 0;

    float light_x = 0;
    float light_y = 8.0f;
    float light_z = 6.0f;

    float rotate_cube = 0;
    float cameraRotation = 0;

    float light_r = 1.0f;
    float light_g = 1.0f;
    float light_b = 1.0f;

    float camera_x = 0;
    float camera_y = 2.0f;
    float camera_z = 5.0f;
};

class bullet {
public:
    GLfloat x{}, y{  }, z{  };
    GLfloat scale{ 0.5f };
    GLfloat z_move{ 1.0f };
    GLuint vvbo{}, nvbo{};


    void init(float x, float y, float z) {
        this->z = z;
        this->x = x;
        this->y = y;

        this->vvbo = cubePosVbo2;
        this->nvbo = cubeNomalVbo2;
    }

    void move() {
        this->z -= this->z_move;
    }

};

class snow {
public:
    GLfloat x{}, y{  }, z{  };
    GLfloat scale{ 0.025 };
    GLfloat x_move{}, y_move{};
    GLuint vvbo{}, nvbo{};



    void init() {
        this->z = random_snow_pos_z(engine);
        this->x = random_snow_pos_x(engine);
        this->y = 4.0f;
        this->y_move = random_snow_pos_y_move(engine);
    }

    void move() {
        this->y += this->y_move;
        if (this->y <= 0)
        {
            this->init();
        }
    }

};

struct objRead {

    std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
    std::vector< glm::vec3 > temp_vertices;
    std::vector< glm::vec2 > temp_uvs;
    std::vector< glm::vec3 > temp_normals;
    std::vector< glm::vec3 > outvertex, outnormal;
    std::vector< glm::vec2 > outuv;

    float sumX = 0.0, sumY = 0.0, sumZ = 0.0;
    float aveX, aveY, aveZ;
    float scaleX, scaleY, scaleZ;
    float minX = 0.0, minY = 0.0, minZ = 0.0;
    float maxX = 0.0, maxY = 0.0, maxZ = 0.0;
    float scaleAll{};

    float sizeX{}, sizeY{}, sizeZ{};

    int loadObj_normalize_center(const char* filename);

};

#endif