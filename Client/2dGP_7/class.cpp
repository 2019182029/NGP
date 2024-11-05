#include "class.h"

std::default_random_engine engine(std::random_device{}());
std::uniform_real_distribution<GLfloat> random_scale(0.25f, 0.5f);
std::uniform_real_distribution<GLfloat> random_move(-0.1f, 0.1f);
std::uniform_real_distribution<GLfloat> random_color(0.0f, 1.0f);
std::uniform_real_distribution<double> random_rotate(-10.0f, 10.0f);

std::uniform_real_distribution<GLfloat> random_snow_pos_z(-30.0f, 2.0f);
std::uniform_real_distribution<GLfloat> random_snow_pos_x(-2.0f, 2.0f);
std::uniform_real_distribution<GLfloat> random_snow_pos_y_move(-0.2f, -0.05f);

obs::obs()
    : x(0), y(0), z(-45.0f),
    x_scale(2.0f), y_scale(0.0001f), z_scale(50.0f),
    r(1), g(0), b(0), a(1.0f) {
    Object = objReader.loadObj_normalize_center("cube.obj");
}
//--------------------------------------------------------------------------------------------------
obss::obss(GLuint cubePosVbo2, GLuint cubeNomalVbo2)
    : x(0), y(0.25f), z(-1.0f),
    x_scale(0.25f), y_scale(0.25f), z_scale(0.25f),
    r(1), g(0), b(0), a(1.0f),
    jump_scale(0),
    hp(100),
    vvbo(cubePosVbo2), nvbo(cubeNomalVbo2),
    Object(0) { }

void obss::change_color(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

void obss::init(int PosVbo, int NomalVbo) {
    this->vvbo = PosVbo;
    this->nvbo = NomalVbo;
}
//---------------------------------------------------------------------------------------------------

object_won::object_won()
    : x(0), y(0.25f), z(-100.0f),
    x_scale(0.25f), y_scale(0.25f), z_scale(0.25f),
    r(0), g(0), b(0), a(1.0f),
    vvbo(), nvbo(),
    rotate(0), object_num(0), rotate_move(0) {
}

void object_won::init(int PosVbo, int NomalVbo) {
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

void object_won::move() {
    this->x += this->x_move;
    this->y += this->y_move;
    this->z += 1.0f;

    if (this->z > 5.0f) {
        this->init(this->vvbo, this->nvbo);
    }

    if (this->x + this->x_scale + this->x_move > 2.0f || this->x - this->x_scale + this->x_move < -2.0f) {
        this->x_move *= -1;
        this->rotate_move *= -1;
    }
    if (this->y + this->y_scale + this->y_move > 4.0f || this->y - this->y_scale + this->y_move < 0.0f) {
        this->y_move *= -1;
        this->rotate_move *= -1;
    }

    this->rotate += this->rotate_move;
}
//---------------------------------------------------------------------------------------------------------------
light_set::light_set()
    : rotate_light(0),
    light_x(0), light_y(8.0f), light_z(6.0f),
    rotate_cube(0), cameraRotation(0),
    light_r(1.0f), light_g(1.0f), light_b(1.0f),
    camera_x(0), camera_y(2.0f), camera_z(5.0f) { }