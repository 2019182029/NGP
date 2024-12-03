#include "main.h"
#include "obj.h"

#ifndef __CLS_H__
#define __CLS_H__

class obs {
public:
    GLfloat x{}, y{}, z{  };

    GLfloat x_scale{  }, y_scale{  }, z_scale{  };

    GLfloat r{  }, g{  }, b{  }, a{  };

    objRead objReader;
    GLint Object ;
    
    obs(); // 持失切 識情
};

class obss {
public:
    GLfloat x{}, y{ 0.25f }, z{ -1.0f };
    GLfloat x_scale{ 0.25f }, y_scale{ 0.25f }, z_scale{ 0.25f };

    GLfloat r{ 1 }, g{ 0 }, b{ 0 }, a{ 1.0 };
    int jump_scale{};
    int hp;

    GLuint vvbo;
    GLuint nvbo;
    GLint Object;

    obss(GLuint cubePosVbo2,GLuint cubeNomalVbo2); // 持失切 識情

    void change_color(float r, float g, float b);
    void init(int PosVbo, int NomalVbo);
};

class object {
public:
    GLfloat x{}, y{ 0.25f }, z{ -100.0f };
    GLfloat x_scale{ 0.25f }, y_scale{ 0.25f }, z_scale{ 0.25f };
    GLfloat x_move{}, y_move{}, z_move{};
    GLfloat r{}, g{}, b{}, a{ 1.0 };
    GLuint vvbo{}, nvbo{};
    GLfloat rotate{};
    GLint object_num{};
    int rotate_move{};

    object(); // 持失切 識情

    void init(int PosVbo, int NomalVbo);
    void move(double elapsedTime);
};
//------------------------------------------------------------------------------------------------
class light_set {
public:
    float rotate_light;

    float light_x;
    float light_y;
    float light_z;

    float rotate_cube;
    float cameraRotation;

    float light_r;
    float light_g;
    float light_b;

    float camera_x;
    float camera_y;
    float camera_z;

    light_set(); // 持失切 識情
};

#endif