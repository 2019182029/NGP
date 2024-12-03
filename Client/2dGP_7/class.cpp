#include "class.h"


std::default_random_engine dre(0);

std::default_random_engine engine(1);
std::uniform_int_distribution<int> uid(-15, 15);
std::uniform_int_distribution<int> uidZDir(1, 10);

std::uniform_real_distribution<GLfloat> random_scale(0.25f, 0.5f);
std::uniform_real_distribution<GLfloat> random_color(0.0f, 1.0f);
std::uniform_real_distribution<double> random_rotate(-10.0f, 10.0f);


std::default_random_engine dreObstacle{ std::random_device{}() };
std::uniform_int_distribution<int> uidObstacle(0, 2);

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

void obss::init(int PosVbo, int NomalVbo, int num) {
    this->vvbo = PosVbo;
    this->nvbo = NomalVbo;
    this->Object = num;


    m_bIsBlowingUp = false;
    m_fElapsedTime = 0.0f;

    m_pExplosionMesh = new object();
    m_pExplosionMesh->init(PosVbo, NomalVbo);
    m_pExplosionMesh->object_num = num;
    m_pExplosionMesh->x_scale = 0.025f;
    m_pExplosionMesh->y_scale = 0.025f;
    m_pExplosionMesh->z_scale = 0.025f;

    for (int i = 0; i < 64; ++i) {
        // 랜덤 이동을 위한 theta와 phi 값을 move와 유사한 방식으로 계산
        float theta = glm::linearRand(0.0f, glm::two_pi<float>());
        float phi = glm::linearRand(0.0f, glm::pi<float>());

        float x = std::cos(theta) * std::sin(phi);
        float y = std::sin(theta) * std::sin(phi);
        float z = std::cos(phi);

        float magnitude = glm::linearRand(2.5f, 5.0f);

        // 방사형 벡터의 방향을 move 함수의 결과로 설정
        m_pvf3Vectors[i] = glm::vec3(x, y, z) * magnitude;
    }

}
//---------------------------------------------------------------------------------------------------

object::object()
    : x(0), y(0.25f), z(-100.0f),
    x_scale(0.25f), y_scale(0.25f), z_scale(0.25f),
    r(0), g(0), b(0), a(1.0f),
    vvbo(), nvbo(),
    rotate(0), object_num(0), rotate_move(0) {
}

void object::init(int PosVbo, int NomalVbo) {
    this->z = -100.0f;
    this->x = uid(dre) / 10.0f;
    this->y = uid(dre) / 10.0f;

    this->x_move = uid(dre);
    this->y_move = uid(dre);
    this->z_move = uidZDir(dre);

    this->rotate_move = random_rotate(engine);

    this->r = random_color(engine);
    this->g = random_color(engine);
    this->b = random_color(engine);

    float size = 0.25;
    this->x_scale = size;
    this->y_scale = size;   
    this->z_scale = size;


    this->vvbo = PosVbo;
    this->nvbo = NomalVbo;
}

void object::move(double elapsedTime) {
    if (this->x + this->x_scale + this->x_move*elapsedTime > 2.0f || this->x - this->x_scale + this->x_move*elapsedTime < -2.0f) {
        this->x_move *= -1;
        this->rotate_move *= -1;
    }
    if (this->y + this->y_scale + this->y_move > 4.0f || this->y - this->y_scale + this->y_move < 0.0f) {
        this->y_move *= -1;
        this->rotate_move *= -1;
    }

    this->rotate += this->rotate_move;

    this->x += this->x_move * elapsedTime;
    this->y += this->y_move * elapsedTime;
    this->z += this->z_move * elapsedTime;

    //서버한테 5로 수정요청

    if (this->z > 5.0f) {
        this->z = -100.0f + this->z;
    }
}

//---------------------------------------------------------------------------------------------------------------
light_set::light_set()
    : rotate_light(0),
    light_x(0), light_y(8.0f), light_z(6.0f),
    rotate_cube(0), cameraRotation(0),
    light_r(1.0f), light_g(1.0f), light_b(1.0f),
    camera_x(0), camera_y(2.0f), camera_z(5.0f) { }