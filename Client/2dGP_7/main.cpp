#include "main.h"
#include "obj.h"
#include "class.h"
#include "packet.h"
#include "Common.h"
#include "resource.h"
#pragma comment(lib, "winmm")
#include <mmsystem.h>

#include <Commctrl.h> // 공용 컨트롤 헤더 추가

#pragma comment(lib, "Comctl32.lib") // 공용 컨트롤 라이브러리 링크

#define SERVERPORT 9000
#define BUFSIZE    512

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);
// 소켓 함수 오류 출력
void DisplayError(const char* msg);
// GUI 처리 및 소켓 통신을 위한 스레드 함수
DWORD WINAPI GuiAndClientMain(LPVOID arg);

SOCKET sock;
char buf[BUFSIZE + 1];
HWND hReadyButton, hCancelButton, hExitButton, hEditIP, hConnectButton; // Added hConnectButton for connect functionality
HINSTANCE g_inst;

//준비완료 준비 취소 
HANDLE hReceiveThread = NULL;
bool gameReady = false;


GLuint vao;

Packet packetclient;

GLchar* vertexSource, * fragmentSource;
GLuint vertexShader, fragmentShader;
GLuint shaderProgramID;

GLuint WallPosVbo;
GLuint WallNomalVbo;

GLuint cubePosVbo2;
GLuint cubeNomalVbo2;

GLuint RockPosVbo;
GLuint RockNomalVbo;

GLuint hpPosVbo;
GLuint hpNomalVbo;

GLuint teapotPosVbo;
GLuint teapotNomalVbo;

std::default_random_engine engine2(std::random_device{}());
std::uniform_real_distribution<double> random_model(1, 6);

obs wall;
obss main_character(cubePosVbo2, cubeNomalVbo2);

std::vector<object_won> objects;

objRead RockReader;
GLint RockObject = RockReader.loadObj_normalize_center("rock.obj");

objRead CubeReader;
GLint CubeObject = CubeReader.loadObj_normalize_center("cube.obj");

objRead sphere;
GLint sphereObject = sphere.loadObj_normalize_center("sphere.obj");

objRead teapotReader;
GLint teapotObject = teapotReader.loadObj_normalize_center("teapot.obj");

GLfloat Color[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

light_set light;

void setOrthographicProjection() {
    // 현재 행렬 모드 저장
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 현재 뷰포트 가져오기
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // 투영 모드로 전환
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // 직교 투영 행렬 설정
    gluOrtho2D(0, viewport[2], 0, viewport[3]);

    // 모델뷰 모드로 다시 전환
    glMatrixMode(GL_MODELVIEW);
}

void resetPerspectiveProjection() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void renderBitmapString(float x, float y, void* font, const char* string) {
    const char* c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

bool checkCollision(object_won& , obss& );

int move_check{};
int jump_check = 3;
int sever_level = 0;
bool game_check = true;
bool left_button = false;
int playerHP = 100;

DWORD WINAPI ReceiveDataThread(LPVOID arg) {
    HWND hDlg = reinterpret_cast<HWND>(arg);
    Packet recvPacket;
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode); // Set non-blocking mode

    while (gameReady) {
        int bytesReceived = recv(sock, reinterpret_cast<char*>(&recvPacket), sizeof(recvPacket), 0);
        if (bytesReceived > 0) {
            // 게임 시작 비트를 확인하고 GUI 종료 및 스레드 종료
            if (recvPacket.isGameStarted()) {
                EndDialog(hDlg, IDCANCEL); // GUI 대화 상자를 종료
                ExitThread(0);             // 스레드 종료
            }
        }
        else if (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            DisplayError("recv()");
            ExitThread(0); // 에러 시 스레드 종료
        }
        Sleep(10); // Small delay to avoid high CPU usage
    }

    return 0;
}

// GUI 및 소켓 통신을 처리하는 스레드 함수
DWORD WINAPI GuiAndClientMain(LPVOID arg)
{
    DialogBox(g_inst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
    return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        hEditIP = GetDlgItem(hDlg, IDC_IP);
        hConnectButton = GetDlgItem(hDlg, ID_CONNECT);  // Connect button
        hReadyButton = GetDlgItem(hDlg, ID_READY);
        hCancelButton = GetDlgItem(hDlg, ID_CANCEL);
        hExitButton = GetDlgItem(hDlg, ID_EXIT);

        EnableWindow(hReadyButton, FALSE); // Disable "Ready" initially
        EnableWindow(hCancelButton, FALSE); // Disable "Cancel" initially

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            DisplayError("socket()");
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CONNECT: {  // Connect button clicked
            // Retrieve IP address from input field
            DWORD ip;
            SendMessage(hEditIP, IPM_GETADDRESS, 0, (LPARAM)&ip);
            BYTE part1 = FIRST_IPADDRESS(ip);
            BYTE part2 = SECOND_IPADDRESS(ip);
            BYTE part3 = THIRD_IPADDRESS(ip);
            BYTE part4 = FOURTH_IPADDRESS(ip);
            snprintf(buf, BUFSIZE, "%d.%d.%d.%d", part1, part2, part3, part4);

            // Set up server address structure
            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(SERVERPORT);
            serverAddr.sin_addr.s_addr = inet_addr(buf);

            // Create socket
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == INVALID_SOCKET) {
                DisplayError("socket()"); // Display error if socket creation fails
                return TRUE;
            }

            // Attempt to connect to the server
            if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                DisplayError("connect()");
                closesocket(sock);
                sock = INVALID_SOCKET;
                return TRUE;
            }

            // Display connection success message and enable "Ready" button
            DisplayText("준비 완료되었습니다. 서버에 연결되었습니다. IP 주소: %s\n", buf);
            EnableWindow(hReadyButton, TRUE);
            EnableWindow(hConnectButton, FALSE);  // Disable "Connect" button after successful connection
            return TRUE;
        }

        // Ready 상태일 때 수신 스레드를 생성
        case ID_READY:
            packetclient.setReady(true);
            send(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);

            EnableWindow(hReadyButton, FALSE);
            EnableWindow(hCancelButton, TRUE);
            DisplayText("Ready status confirmed.\n");

            // hDlg 핸들을 인자로 전달하여 스레드를 생성
            gameReady = true;
            hReceiveThread = CreateThread(NULL, 0, ReceiveDataThread, hDlg, 0, NULL);
            return TRUE;

        case ID_CANCEL:
            // Packet에서 준비 비트를 해제
            packetclient.setReady(false);

            // 서버에 변경된 Packet을 전송
            send(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);

            // 데이터 수신을 중지하고 스레드를 종료
            gameReady = false;
            if (hReceiveThread) {
                WaitForSingleObject(hReceiveThread, INFINITE);
                CloseHandle(hReceiveThread);
                hReceiveThread = NULL;
            }

            // 버튼 상태 변경
            EnableWindow(hReadyButton, TRUE);
            EnableWindow(hCancelButton, FALSE);
            DisplayText("준비 상태가 취소되었습니다.\n");

            return TRUE;

        case ID_EXIT:
            EndDialog(hDlg, IDCANCEL);
            if (sock != INVALID_SOCKET) {
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

void DisplayText(const char* fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    char cbuf[BUFSIZE * 2];
    vsprintf(cbuf, fmt, arg);
    va_end(arg);

    int nLength = GetWindowTextLength(GetDlgItem(GetParent(hReadyButton), IDC_EDIT2));
    SendMessage(GetDlgItem(GetParent(hReadyButton), IDC_EDIT2), EM_SETSEL, nLength, nLength);
    SendMessageA(GetDlgItem(GetParent(hReadyButton), IDC_EDIT2), EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}

void DisplayError(const char* msg) {
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char*)&lpMsgBuf, 0, NULL);
    DisplayText("[%s] %s\r\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}



int main(int argc, char** argv)
{
    g_inst = GetModuleHandle(NULL); // 인스턴스 핸들을 main에서 직접 가져옴

    // 공용 컨트롤 초기화
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_INTERNET_CLASSES;
    InitCommonControlsEx(&icex);

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // GUI 및 소켓 통신 스레드 생성
    HANDLE hThread = CreateThread(NULL, 0, GuiAndClientMain, NULL, 0, NULL);
    if (hThread == NULL) {
        DisplayError("Failed to create hThread");
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Example1");

    glewExperimental = GL_TRUE;
    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);
    glewInit();

    make_shaderProgram();
    InitBuffer();
    glutWarpPointer(800 / 2, 800 / 2);
    glutTimerFunc(1000, next_stage, 1);
    glutTimerFunc(60, update, 1);

    main_character.init(cubePosVbo2, cubeNomalVbo2);
    main_character.Object = CubeObject;

    PlaySound(TEXT(GAME_BGM), NULL, SND_ASYNC | SND_LOOP);

    glutKeyboardFunc(keyboard);

    glutDisplayFunc(drawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardUpFunc(keyUp);
    glutMouseFunc(MousePoint);
    glutMotionFunc(Motion);

    glutMainLoop();

    return 0;
}

void drawScene()
{
    glClearColor(Color[0], Color[1], Color[2], Color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);

    int PosLocation = glGetAttribLocation(shaderProgramID, "vPos");
    int NomalLocation = glGetAttribLocation(shaderProgramID, "vNormal");

    glEnableVertexAttribArray(PosLocation);
    glEnableVertexAttribArray(NomalLocation);

    int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos"); //--- lightPos 값 전달: (0.0, 0.0, 5.0);
    glUniform3f(lightPosLocation, light.light_x, light.light_y, light.light_z);
    int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor"); //--- lightColor 값 전달: (1.0, 1.0, 1.0) 백색
    glUniform3f(lightColorLocation, light.light_r, light.light_g, light.light_b);
    unsigned int lighton = glGetUniformLocation(shaderProgramID, "light");
    glUniform1i(lighton, 1);

    unsigned int ambiont = glGetUniformLocation(shaderProgramID, "amb");

    glUniform3f(ambiont, 0.0,0.0,0.0);

    int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor"); //--- object Color값 전달: (1.0, 0.5, 0.3)의 색

    int modelMatrixLocation = glGetUniformLocation(shaderProgramID, "modelMatrix");
    int viewLoc = glGetUniformLocation(shaderProgramID, "view"); //--- 버텍스 세이더에서 뷰잉 변환 행렬 변수값을 받아온다.
    int projLoc = glGetUniformLocation(shaderProgramID, "projection"); //--- 버텍스 세이더에서 투영 변환 행렬 변수값을 받아온다.

    glm::vec3 cameraPos = glm::vec3(light.camera_x, light.camera_y, light.camera_z); //--- 카메라 위치
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -200.0f); //--- 카메라 바라보는 방향
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 vTransform(1.0f);
    glm::mat4 pTransform(1.0f);

    vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);

    vTransform = glm::rotate(vTransform, glm::radians(light.cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // z축으로 회전

    pTransform = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 190.0f); //--- 투영 공간 설정: fovy, aspect, near, far

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

    int viewPosLocation = glGetUniformLocation(shaderProgramID, "viewPos"); //--- viewPos 값 전달: 카메라 위치
    glUniform3f(viewPosLocation, cameraPos.x, cameraPos.y, cameraPos.z);

    //상하좌우 벽 그리는 반복문
    for (int i = 0; i < 4; i++) {
        // 모델 행렬 초기화
        glm::mat4 modelMatrix(1.0f);
        // 모델 행렬을 셰이더에 전달
        if (light.cameraRotation == 180) {
            if (i == 0) {

                modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 4.0f, 0)); // 이동
            }
            else if (i == 1)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 2.0f, 0)); // 이동
                modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
            }
            else if (i == 2)
            {
                modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
            }
            else if (i == 3)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 2.0f, 0.0f)); // 이동
                modelMatrix = glm::rotate(modelMatrix, glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
            }
        }
        else
        {
            if (i == 0) {

            }
            else if (i == 1)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 2.0f, 0.0f)); // 이동
                modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
            }
            else if (i == 2)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 4.0f, 0)); // 이동
                modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
            }
            else if (i == 3)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 2.0f, 0)); // 이동
                modelMatrix = glm::rotate(modelMatrix, glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
            }
        }
        modelMatrix = glm::translate(modelMatrix, glm::vec3(wall.x, wall.y, wall.z)); // 이동
        modelMatrix = glm::scale(modelMatrix, glm::vec3(wall.x_scale, wall.y_scale, wall.z_scale));
        glUniform4f(objColorLocation, wall.r, wall.g, wall.b,wall.a);

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, WallPosVbo);
        glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(PosLocation);

        glBindBuffer(GL_ARRAY_BUFFER, WallNomalVbo);
        glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(NomalLocation);

        glDrawArrays(GL_TRIANGLES, 0, wall.Object);

    }

    //메인 캐릭터 그리는 반복문
    {
        // 모델 행렬 초기화
        glm::mat4 modelMatrix(1.0f);
        // 모델 행렬을 셰이더에 전달
        modelMatrix = glm::translate(modelMatrix, glm::vec3(main_character.x, main_character.y, main_character.z)); // 이동
        modelMatrix = glm::scale(modelMatrix, glm::vec3(main_character.x_scale, main_character.y_scale, main_character.z_scale));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(-light.cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // z축으로 회전
        glUniform4f(objColorLocation, main_character.r, main_character.g, main_character.b, 1.0);

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, main_character.vvbo);
        glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(PosLocation);

        glBindBuffer(GL_ARRAY_BUFFER, main_character.nvbo);
        glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(NomalLocation);

        glDrawArrays(GL_TRIANGLES, 0, main_character.Object);

    }
    //장애물들 그리는 반복문
    for (int i = 0; i < objects.size(); i++){
        // 모델 행렬 초기화
        glm::mat4 modelMatrix(1.0f);
        // 모델 행렬을 셰이더에 전달
        modelMatrix = glm::translate(modelMatrix, glm::vec3(objects[i].x, objects[i].y, objects[i].z)); // 이동

        modelMatrix = glm::rotate(modelMatrix, glm::radians(objects[i].rotate), glm::vec3(0.0f, 0.0f, 1.0f)); // X 축 회전
        modelMatrix = glm::rotate(modelMatrix, glm::radians(objects[i].rotate), glm::vec3(1.0f, 0.0f, 0.0f)); // X 축 회전
        modelMatrix = glm::rotate(modelMatrix, glm::radians(objects[i].rotate), glm::vec3(0.0f, 1.0f, 0.0f)); // X 축 회전
        modelMatrix = glm::scale(modelMatrix, glm::vec3(objects[i].x_scale, objects[i].y_scale, objects[i].z_scale));
        glUniform4f(objColorLocation, objects[i].r, objects[i].g, objects[i].b, objects[i].a);

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, objects[i].vvbo);
        glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(PosLocation);

        glBindBuffer(GL_ARRAY_BUFFER, objects[i].nvbo);
        glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(NomalLocation);

        if (sever_level > 2) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        glDrawArrays(GL_TRIANGLES, 0, objects[i].object_num);

        if (sever_level >2 ) {
            glDisable(GL_BLEND);
        }
    }
    glDisableVertexAttribArray(PosLocation);
    glDisableVertexAttribArray(NomalLocation);

    // 현재 뷰포트 크기를 얻습니다.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int windowWidth = viewport[2];
    int windowHeight = viewport[3];

    // 직교 투영 설정
    setOrthographicProjection();

    glUseProgram(0);

    // 텍스트 렌더링
    glPushMatrix();
    glLoadIdentity();

    // 텍스트의 위치를 화면 우측 상단으로 설정
    float x = windowWidth - 100; // 화면 너비에서 100px 떨어진 위치
    float y = windowHeight - 30; // 화면 높이에서 30px 떨어진 위치
    renderBitmapString(x, y, GLUT_BITMAP_HELVETICA_18, ("HP: " + std::to_string(main_character.hp)).c_str());

    glPopMatrix();

    // 원래 투영으로 복귀
    resetPerspectiveProjection();

    glUseProgram(shaderProgramID);


    glutSwapBuffers();
}

void Reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

void InitBuffer()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &WallPosVbo);
    glBindBuffer(GL_ARRAY_BUFFER, WallPosVbo);
    glBufferData(GL_ARRAY_BUFFER, wall.objReader.outvertex.size() * sizeof(glm::vec3), &wall.objReader.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &WallNomalVbo);
    glBindBuffer(GL_ARRAY_BUFFER, WallNomalVbo);
    glBufferData(GL_ARRAY_BUFFER, wall.objReader.outnormal.size() * sizeof(glm::vec3), &wall.objReader.outnormal[0], GL_STATIC_DRAW);

    glGenBuffers(1, &cubePosVbo2);
    glBindBuffer(GL_ARRAY_BUFFER, cubePosVbo2);
    glBufferData(GL_ARRAY_BUFFER,CubeReader.outvertex.size() * sizeof(glm::vec3), &CubeReader.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &cubeNomalVbo2);
    glBindBuffer(GL_ARRAY_BUFFER, cubeNomalVbo2);
    glBufferData(GL_ARRAY_BUFFER, CubeReader.outnormal.size() * sizeof(glm::vec3), &CubeReader.outnormal[0], GL_STATIC_DRAW);

    glGenBuffers(1, &RockPosVbo);
    glBindBuffer(GL_ARRAY_BUFFER, RockPosVbo);
    glBufferData(GL_ARRAY_BUFFER, RockReader.outvertex.size() * sizeof(glm::vec3), &RockReader.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &RockNomalVbo);
    glBindBuffer(GL_ARRAY_BUFFER, RockNomalVbo);
    glBufferData(GL_ARRAY_BUFFER, RockReader.outnormal.size() * sizeof(glm::vec3), &RockReader.outnormal[0], GL_STATIC_DRAW);


    glGenBuffers(1, &hpPosVbo);
    glBindBuffer(GL_ARRAY_BUFFER, hpPosVbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.outvertex.size() * sizeof(glm::vec3), &sphere.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &hpNomalVbo);
    glBindBuffer(GL_ARRAY_BUFFER, hpNomalVbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.outnormal.size() * sizeof(glm::vec3), &sphere.outnormal[0], GL_STATIC_DRAW);

    glGenBuffers(1, &teapotPosVbo);
    glBindBuffer(GL_ARRAY_BUFFER, teapotPosVbo);
    glBufferData(GL_ARRAY_BUFFER, teapotReader.outvertex.size() * sizeof(glm::vec3), &teapotReader.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &teapotNomalVbo);
    glBindBuffer(GL_ARRAY_BUFFER, teapotNomalVbo);
    glBufferData(GL_ARRAY_BUFFER, teapotReader.outnormal.size() * sizeof(glm::vec3), &teapotReader.outnormal[0], GL_STATIC_DRAW);

}

void make_shaderProgram()
{
    make_vertexShaders();
    make_fragmentShaders();

    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vertexShader);
    glAttachShader(shaderProgramID, fragmentShader);
    glLinkProgram(shaderProgramID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(shaderProgramID);
}

void make_vertexShaders()
{
    vertexSource = filetobuf("vertex.glsl");
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
    glCompileShader(vertexShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cout << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

void make_fragmentShaders()
{
    fragmentSource = filetobuf("fragment.glsl");
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
    glCompileShader(fragmentShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cout << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

char* filetobuf(const char* file)
{
    FILE* fptr;
    long length;
    char* buf;
    fptr = fopen(file, "rb");
    if (!fptr)
        return NULL;
    fseek(fptr, 0, SEEK_END);
    length = ftell(fptr);
    buf = (char*)malloc(length + 1);
    fseek(fptr, 0, SEEK_SET);
    fread(buf, length, 1, fptr);
    fclose(fptr);
    buf[length] = 0;
    return buf;
}



GLvoid update(int value) {

    for (int i = 0; i < objects.size(); i++)
    {
        objects[i].move();

        if (checkCollision(objects[i], main_character)) {
            objects[i].z = -200.0f; 
            main_character.hp -= 10;
            main_character.init(objects[i].vvbo, objects[i].nvbo);
            main_character.Object = objects[i].object_num;

            if (main_character.hp <= 0)
            {
                objects.clear();
               
                sever_level = 0;
                game_check = false;
                exit(0);
            }
            else
            {
                main_character.change_color(objects[i].r, objects[i].g, objects[i].b);
            }
        }
    }

    

    

    if (light.cameraRotation == 0)
    {
        main_character.y = 0.25f + 0.1f * main_character.jump_scale;
        light.light_y = 8.0f;

    }
    else if (light.cameraRotation == 270)
    {
        main_character.x = 2.0f - main_character.x_scale - 0.1f * main_character.jump_scale;
        light.light_y = 8.0f;

    }
    else if (light.cameraRotation == 180)
    {
        main_character.y = 4.0f - main_character.y_scale - 0.1f * main_character.jump_scale;
        light.light_y = -4.0f;
    }
    else if (light.cameraRotation == 90)
    {
        main_character.x = -2.0f + main_character.x_scale + 0.1f * main_character.jump_scale;
        light.light_y = 8.0f;
    }

    InitBuffer();
    glutPostRedisplay();

    if (game_check) {
        glutTimerFunc(30, update, 1);
    }
}

GLvoid keyboard(unsigned char key, int x, int y)
{
    handleEvent(key, true);
    glutPostRedisplay();
}
GLvoid keyUp(unsigned char key, int x, int y)
{
    handleEvent(key, false);
    glutPostRedisplay();
}

GLvoid handleEvent(unsigned char key, bool state)
{
    if (state)
    {
        switch (key) {
        case 'z':
            jump();
            break;
       
        case '2':
            if (sever_level != 2) {
                sever_level = 2;
                glutTimerFunc(90, object_ok, 1);
            }
            break;
        case '3':
            if (sever_level != 3) {
                sever_level = 3;
                objects.clear();
                glutTimerFunc(90, object_ok, 1);
            }
           
            break;
        case 'r':
            game_check = true;
            sever_level = 0;
            objects.clear();

            break;

        }
    }
}

GLvoid MousePoint(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_UP) {
           left_button = false;
            glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

        }
        else {
           left_button = true;
            glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
        }
    }
}

GLvoid Motion(int x, int y) {
    {
        if (left_button) {
            glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
            GLfloat mouseX2 = static_cast<GLfloat>(x) - (glutGet(GLUT_WINDOW_WIDTH) / 2.0f);

            // 윈도우 중심(400)에서 마우스 위치의 거리 계산
            GLfloat distanceFromCenter = mouseX2; // 윈도우 중심을 기준으로 거리 계산
            GLfloat rotationChange = distanceFromCenter * 0.005f; // 필요에 따라 감도 조절

            
            if (light.cameraRotation == 0)
            {
                main_character.y =  0.1f * main_character.jump_scale;
                main_character.x += rotationChange;
                light.light_y = 8.0f;
            }
            else if (light.cameraRotation == 270)
            {
                main_character.x = 2.0f - main_character.x_scale - 0.1f * main_character.jump_scale;
                main_character.y += rotationChange;
                light.light_y = 8.0f;

            }
            else if (light.cameraRotation == 180)
            {
                main_character.y = 4.0f - main_character.y_scale - 0.1f * main_character.jump_scale;
                main_character.x -= rotationChange;
                light.light_y = -4.0f;
            }
            else if (light.cameraRotation == 90)
            {
                main_character.x = -2.0f + main_character.x_scale + 0.1f * main_character.jump_scale;
                main_character.y -= rotationChange;
                light.light_y = 8.0f;
            }


            if(sever_level > 3)
            {
                if (main_character.x + main_character.x_scale > 2.0f || main_character.x - main_character.x_scale < -2.0f)
                {
                    main_character.x -= rotationChange;
                }
            }
            // 마우스를 윈도우 중앙으로 이동
            glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
        }
        if(sever_level < 4){
            if (main_character.x + main_character.x_scale > 2.0f)
            {
                light.cameraRotation = 270.0f;
                light.camera_x = 2.0f;
                light.camera_y = 0;
                jump_check = 3;
                main_character.jump_scale = 0;
            }
            else if (main_character.x - main_character.x_scale < -2.0f)
            {
                light.cameraRotation = 90.0f;
                light.camera_x = -2.0f;
                light.camera_y = 0.0f;
                jump_check = 3;
                main_character.jump_scale = 0;
            }
            else if (main_character.y - main_character.y_scale < 0.0f)
            {
                light.cameraRotation = 0.0f;
                light.camera_x = 0.0f;
                light.camera_y = 2.0f;
                jump_check = 3;
                main_character.jump_scale = 0;
            }
            else if (main_character.y + main_character.y_scale > 4.0f)
            {
                light.cameraRotation = 180.0f;
                light.camera_x = 0.0f;
                light.camera_y = -2.0f;
                main_character.jump_scale = 0;
                jump_check = 3;
            }
        }
        
        InitBuffer();
        glutPostRedisplay();
    }
}

GLvoid jump() {
    if (light.cameraRotation == 0)
    {
        if (jump_check == 3) {
            jump_check = 0;
            main_character.jump_scale = 0;
            glutTimerFunc(60, jump_ok, 1);
        }
    }
    else if (light.cameraRotation == 270)
    {
        if (jump_check == 3) {
            jump_check = 0;
            main_character.jump_scale = 0;
            glutTimerFunc(60, jump_ok, 1);
        }
    }
    else if (light.cameraRotation == 180)
    {
        if (jump_check == 3) {
            jump_check = 0;
            main_character.jump_scale = 0;
            glutTimerFunc(60, jump_ok, 1);
        }
    }
    else if (light.cameraRotation == 90)
    {

        if (jump_check == 3) {
            jump_check = 0;
            main_character.jump_scale = 0;
            glutTimerFunc(60, jump_ok, 1);
        }
    }
}

bool checkCollision(object_won& sphere, obss& wall) {
    // AABB - 원 충돌
    float closestX = std::max(wall.x - wall.x_scale, std::min(sphere.x, wall.x + wall.x_scale));
    float closestY = std::max(wall.y - wall.y_scale, std::min(sphere.y, wall.y + wall.y_scale));
    float closestZ = std::max(wall.z - wall.z_scale, std::min(sphere.z, wall.z + wall.z_scale));

    // 원의 중심과 가장 가까운 점 사이의 거리를 계산
    float distanceX = sphere.x - closestX;
    float distanceY = sphere.y - closestY;
    float distanceZ = sphere.z - closestZ;

    // 거리가 원의 반지름보다 작으면 교차점생김
    float radius = sphere.x_scale;
    return (distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ) < (radius * radius);
}


GLvoid jump_ok(int value) {

    if (jump_check != 3) {
    if (main_character.jump_scale < 15 && jump_check == 0) {
        main_character.jump_scale += 1;
        if (main_character.jump_scale == 15)
        {
            jump_check = 1;
        }
    }
    else if (main_character.jump_scale > 0 && jump_check == 1) {
        main_character.jump_scale -= 1;
        if (main_character.jump_scale == 0)
        {
            jump_check = 3;
        }
    }
    InitBuffer();
    glutPostRedisplay();
    glutTimerFunc(30, jump_ok, 1);
    }
}

GLvoid object_ok(int value) {

    if (objects.size() < 10) {
        object_won new_object;
        if (sever_level > 1) {
            int model = random_model(engine2);
            if (model == 1) {
                new_object.init(RockPosVbo, RockNomalVbo); // 객체 초기화
                new_object.object_num = RockObject;
            }
            else if (model == 2)
            {
                new_object.init(cubePosVbo2, cubeNomalVbo2); // 객체 초기화
                new_object.object_num = CubeObject;
            }
            else if( model == 3)
            {
                new_object.init(hpPosVbo, hpNomalVbo); // 객체 초기화
                new_object.object_num = sphereObject;
            }
            else
            {
                new_object.init(teapotPosVbo, teapotNomalVbo); // 객체 초기화
                new_object.object_num = teapotObject;
            }
        }
        else {
            new_object.init(cubePosVbo2, cubeNomalVbo2); // 객체 초기화
            new_object.object_num = CubeObject;
        }
        if (sever_level > 2) {
            new_object.a = 0.1f;
        }
        else
        {
            new_object.a = 1.0f;
        }
        objects.push_back(new_object);

        InitBuffer();
        glutPostRedisplay();

        if (sever_level >0) {
            glutTimerFunc(480, object_ok, 1);
        }
    }
}


GLvoid next_stage(int value) {
    if (game_check) {


        std::cout << sever_level << std::endl;
        if (sever_level < 5) {
            sever_level++;
        }

        if (sever_level == 1) {
            wall.r = 0.8f;
            wall.g = 0.1f;
            wall.b = 0.1f;
            objects.clear();
            glutTimerFunc(900, object_ok, 1);
        }
        else if (sever_level == 2) {

            wall.r = 0.1f;
            wall.g = 0.5f;
            wall.b = 0.1f;
            objects.clear();
            glutTimerFunc(900, object_ok, 1);
        }
        else if (sever_level == 3) {

            wall.r = 0.1f;
            wall.g = 0.1f;
            wall.b = 1.0f;
            objects.clear();
            glutTimerFunc(900, object_ok, 1);
        }
        else if (sever_level == 4) {

            wall.r = 0.0f;
            wall.g = 0.0;
            wall.b = 0.8f;

            light.cameraRotation = 0.0f;
            light.camera_x = 0.0f;
            light.camera_y = 2.0f;
            jump_check = 3;
            main_character.jump_scale = 0;
            main_character.x = 0;
            main_character.y = 0.25f;
            main_character.z = -1.0f;
            objects.clear();
            glutTimerFunc(900, object_ok, 1);
        }

        glutTimerFunc(30000, next_stage, 1);
        InitBuffer();
        glutPostRedisplay();
    }
}

