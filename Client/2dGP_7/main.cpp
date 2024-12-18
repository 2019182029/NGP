#include "main.h"
#include "obj.h"
#include "class.h"
#include "packet.h"
#include "Common.h"
#include "resource.h"

#include <unordered_map>

#pragma comment(lib, "winmm")
#include <mmsystem.h>

#include <Commctrl.h> // 공용 컨트롤 헤더 추가

#pragma comment(lib, "Comctl32.lib") // 공용 컨트롤 라이브러리 링크

#define SERVERPORT 9000
#define BUFSIZE    512

#define TESTNUM    0

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

std::array<Packet, 4> gamePacket;

std::array<Vertex, 10> objectPacket;

GLchar* vertexSource, * fragmentSource;
GLuint vertexShader, fragmentShader;
GLuint shaderProgramID;

GLuint WallPosVbo;
GLuint WallNomalVbo;

GLuint cubePosVbo2;
GLuint cubeNomalVbo2;

GLuint RockPosVbo;
GLuint RockNomalVbo;

GLuint spherePosVbo;
GLuint sphereNomalVbo;

GLuint teapotPosVbo;
GLuint teapotNomalVbo;


std::unordered_map<char, bool> keyStates = {
    {'w', false},
    {'a', false},
    {'d', false},
    {'c', false}
};

bool alive[4] = { false,false ,false ,false };

std::default_random_engine engine2(0);
std::uniform_int_distribution<int> random_model(1, 3);

obs wall;

std::array<obss, 4> gameCharacters = {
    obss(cubePosVbo2, cubeNomalVbo2),  // 1st element
    obss(cubePosVbo2, cubeNomalVbo2),  // 2nd element
    obss(cubePosVbo2, cubeNomalVbo2),  // 3rd element
    obss(cubePosVbo2, cubeNomalVbo2)   // 4th element
};


std::vector<object> objects;

GLvoid object_test();

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

bool checkCollision(object&, obss&);

int move_check{};
int jump_check = 3;
int sever_level = 0;
bool game_check = true;
bool left_button = false;
int playerHP = 100;

DWORD WINAPI ReceiveDataThread(LPVOID arg) {
    HWND hDlg = reinterpret_cast<HWND>(arg);
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode); // Set non-blocking mode
    Packet test;
    while (gameReady) {
        int bytesReceived = recv(sock, reinterpret_cast<char*>(&test), sizeof(test), 0);

        if (bytesReceived > 0) {

            // 게임 시작 비트를 확인하고 GUI 종료 및 스레드 종료
            std::cout << "플레이어 번호 :" << packetclient.GetPlayerNumber() << std::endl;
            if (test.GetStartBit()) {
                packetclient.SetStartBit(test.GetStartBit());
                mode = 0;
                ioctlsocket(sock, FIONBIO, &mode); // Set blocking mode
                EndDialog(hDlg, IDCANCEL); // GUI 대화 상자를 종료
                ExitThread(0);             // 스레드 종료5
            }
        }
        else if (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            DisplayError("recv()");
            ExitThread(0); // 에러 시 스레드 종료
        }
        Sleep(100); // Small delay to avoid high CPU usage
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
            recv(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);
            if (!packetclient.GetValidBit())
            {
                DisplayText("자리가 모두 차있습니다. 다른 서버에 연결해주세요\n", buf);
                Packet a;
                packetclient = a;
                closesocket(sock);
                EnableWindow(hCancelButton, FALSE);
                EnableWindow(hReadyButton, FALSE);
                EnableWindow(hConnectButton, TRUE);
            }
            else {
                // Display connection success message and enable "Ready" button
                DisplayText("준비 완료되었습니다. 서버에 연결되었습니다. IP 주소: %s\n", buf);
                EnableWindow(hReadyButton, TRUE);
                EnableWindow(hConnectButton, FALSE);  // Disable "Connect" button after successful connection
            }
            return TRUE;
        }

                       // Ready 상태일 때 수신 스레드를 생성
        case ID_READY:
            packetclient.SetReadyBit(true);
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
            packetclient.SetReadyBit(false);

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
            // 소켓이 열려있으면 닫음
            packetclient.SetValidBit(0);
            send(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);

            if (sock != INVALID_SOCKET) {
                closesocket(sock);
                sock = INVALID_SOCKET;
            }

            // 버튼 상태 초기화
            EnableWindow(hConnectButton, TRUE);
            EnableWindow(hReadyButton, FALSE);
            EnableWindow(hCancelButton, FALSE);

            // IP 입력창을 활성화
            SendMessage(hEditIP, IPM_CLEARADDRESS, 0, 0); // IP 입력 필드를 초기화

            // 대화 상자 초기화 메시지 표시
            DisplayText("연결이 종료되었습니다. 새 IP를 입력하세요.\n");

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

    //packetclient.SetPlayerNumber(1);

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

    object_test();
    //glutTimerFunc(1000, next_stage, 1);
    glutTimerFunc(16, update, 1);

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

    glUniform3f(ambiont, 0.0, 0.0, 0.0);

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
        glUniform4f(objColorLocation, wall.r, wall.g, wall.b, wall.a);

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, WallPosVbo);
        glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(PosLocation);

        glBindBuffer(GL_ARRAY_BUFFER, WallNomalVbo);
        glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(NomalLocation);

        glDrawArrays(GL_TRIANGLES, 0, wall.Object);

    }

    // 게임 캐릭터
    for (int i = 0; i < 4; ++i) {
        // 모델 행렬 초기화
        if (!gameCharacters[i].m_bIsBlowingUp && gamePacket[i].GetSurvivingBit()) {
            glm::mat4 modelMatrix(1.0f);

            // 모델 행렬을 셰이더에 전달
            modelMatrix = glm::translate(modelMatrix, glm::vec3(gameCharacters[i].x, gameCharacters[i].y, gameCharacters[i].z)); // 이동
            modelMatrix = glm::scale(modelMatrix, glm::vec3(gameCharacters[i].x_scale, gameCharacters[i].y_scale, gameCharacters[i].z_scale));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(-light.cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // z축으로 회전

            // 색상 설정
            glUniform4f(objColorLocation, gameCharacters[i].r, gameCharacters[i].g, gameCharacters[i].b, 1.0f);
            if (gamePacket[i].GetPlayerNumber() == packetclient.GetPlayerNumber())
            {
                glUniform4f(objColorLocation, 0.1f, 0.5f, 0.1f, 1.0f);
            }
            // 모델 행렬을 셰이더에 전달
            glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

            // 버퍼 바인딩
            glBindBuffer(GL_ARRAY_BUFFER, gameCharacters[i].vvbo);
            glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
            glEnableVertexAttribArray(PosLocation);

            glBindBuffer(GL_ARRAY_BUFFER, gameCharacters[i].nvbo);
            glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
            glEnableVertexAttribArray(NomalLocation);

            // 오브젝트 그리기
            glDrawArrays(GL_TRIANGLES, 0, gameCharacters[i].Object); // gameCharacters[i].Object 가 그릴 삼각형의 개수
        }
        else if (gameCharacters[i].m_bIsBlowingUp)
        {
            for (int j = 0; j < 64; ++j)
            {
                glm::mat4 modelMatrix(1.0f);

                // m_pvf3Vectors의 값을 이용하여 위치 벡터 설정
                glm::vec3 currentPos(gameCharacters[i].x, gameCharacters[i].y, gameCharacters[i].z);
                glm::vec3 translationVec = currentPos + gameCharacters[i].m_pvf3Vectors[j] * gameCharacters[i].m_fElapsedTime;
                modelMatrix = glm::translate(modelMatrix, translationVec);

                // 스케일 변환 적용
                modelMatrix = glm::scale(modelMatrix, glm::vec3(gameCharacters[i].m_pExplosionMesh->x_scale,
                    gameCharacters[i].m_pExplosionMesh->y_scale,
                    gameCharacters[i].m_pExplosionMesh->z_scale));
                // 카메라 회전에 따른 회전 변환 적용
                modelMatrix = glm::rotate(modelMatrix, glm::radians(-light.cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f));

                // 색상 설정
                glUniform4f(objColorLocation, 0.5f, 0.5f, 0.5f, 1.0f);

                // 모델 행렬을 셰이더에 전달
                glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

                // 법선 변환 행렬 계산 (월드 변환 행렬의 역행렬의 전치 행렬)
                glm::mat4 normalTransform = glm::inverse(glm::transpose(modelMatrix));
                unsigned int normalTransformLocation = glGetUniformLocation(shaderProgramID, "normalTransform");
                glUniformMatrix4fv(normalTransformLocation, 1, GL_FALSE, &normalTransform[0][0]);

                // 버퍼 바인딩
                glBindBuffer(GL_ARRAY_BUFFER, gameCharacters[i].vvbo);
                glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
                glEnableVertexAttribArray(PosLocation);

                glBindBuffer(GL_ARRAY_BUFFER, gameCharacters[i].nvbo);
                glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
                glEnableVertexAttribArray(NomalLocation);

                // 오브젝트 그리기
                glDrawArrays(GL_TRIANGLES, 0, gameCharacters[i].Object); // gameCharacters[i].Object 가 그릴 삼각형의 개수
            }


        }
    }

    //장애물들 그리는 반복문
    for (int i = 0; i < objects.size(); ++i) {
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

        if (objects[i].a == 0.1f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDrawArrays(GL_TRIANGLES, 0, objects[i].object_num);

            glDisable(GL_BLEND);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, objects[i].object_num);
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
    float x = windowWidth - 200; // 화면 너비에서 100px 떨어진 위치
    float y = windowHeight - 30; // 화면 높이에서 30px 떨어진 위치
    renderBitmapString(x, y, GLUT_BITMAP_HELVETICA_18, ("player: " + std::to_string(packetclient.GetPlayerNumber()) + " , Item: " + std::to_string(gamePacket[packetclient.GetPlayerNumber()].GetItemBit())).c_str());

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
    glBufferData(GL_ARRAY_BUFFER, CubeReader.outvertex.size() * sizeof(glm::vec3), &CubeReader.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &cubeNomalVbo2);
    glBindBuffer(GL_ARRAY_BUFFER, cubeNomalVbo2);
    glBufferData(GL_ARRAY_BUFFER, CubeReader.outnormal.size() * sizeof(glm::vec3), &CubeReader.outnormal[0], GL_STATIC_DRAW);

    glGenBuffers(1, &RockPosVbo);
    glBindBuffer(GL_ARRAY_BUFFER, RockPosVbo);
    glBufferData(GL_ARRAY_BUFFER, RockReader.outvertex.size() * sizeof(glm::vec3), &RockReader.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &RockNomalVbo);
    glBindBuffer(GL_ARRAY_BUFFER, RockNomalVbo);
    glBufferData(GL_ARRAY_BUFFER, RockReader.outnormal.size() * sizeof(glm::vec3), &RockReader.outnormal[0], GL_STATIC_DRAW);


    glGenBuffers(1, &spherePosVbo);
    glBindBuffer(GL_ARRAY_BUFFER, spherePosVbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.outvertex.size() * sizeof(glm::vec3), &sphere.outvertex[0], GL_STATIC_DRAW);

    glGenBuffers(1, &sphereNomalVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sphereNomalVbo);
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

bool alive_check = false;
bool gamefinish = false;
int winner = -1;

GLvoid update(int value) {

    recv(sock, (char*)&gamePacket, sizeof(gamePacket), 0);
    recv(sock, (char*)&objectPacket, sizeof(objectPacket), 0);

    if (!alive_check)
    {
        for (int i = 0; i < 4; ++i)
        {
            alive[i] = gamePacket[i].GetSurvivingBit();
            gameCharacters[i].init(cubePosVbo2, cubeNomalVbo2, CubeObject);  // Initialize with the appropriate VBOs
            std::cout << "생존 여부 " << i << "번쨰 " << alive[i]<< std::endl;
        }
        alive_check = true;
    }
    else
    {
        for (int i = 0; i < 4; ++i)
        {
            if (alive[i] && !gamePacket[i].GetSurvivingBit())
            {
                alive[i] = false;
                winner = i;
                gameCharacters[i].m_bIsBlowingUp = true;

            }
            else if(gameCharacters[i].m_bIsBlowingUp)
            {
                //시간에 따른 이동
                gameCharacters[i].m_fElapsedTime += 0.016f;

                for (int j = 0; j < 64; ++j) {
                    glm::vec3 updatedPosition = gameCharacters[i].m_pvf3Vectors[i] * gameCharacters[i].m_fElapsedTime;
                }
                if (gameCharacters[i].m_fElapsedTime > 0.96f)
                {
                    gameCharacters[i].m_bIsBlowingUp = false;
                }
            }
        }
    }

    for (int i = 0; i < objects.size(); ++i)
    {
        objects[i].x = objectPacket[i].GetXPosition();
        objects[i].y = objectPacket[i].GetYPosition();
        objects[i].z = objectPacket[i].GetZPosition();

        if (gamePacket[packetclient.GetPlayerNumber()].GetAppliedBit())
        {
            objects[i].a = 0.1f;
        }
        else
        {
            objects[i].a = 1.0f;
        }

        if (objectPacket[i].GetItem())
        {
            objects[i].vvbo = cubePosVbo2;
            objects[i].nvbo = cubeNomalVbo2;
            objects[i].object_num = CubeObject;
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        
        switch (gamePacket[i].GetCurrentSurface())
        {
        case 0:
            gameCharacters[i].y = gamePacket[i].getY() + gameCharacters[i].y_scale;
            gameCharacters[i].x = gamePacket[i].getX();
            if (i == packetclient.GetPlayerNumber())
            {
                light.cameraRotation = 0;
                light.camera_x = 0.0f;
                light.camera_y = 2.0f;
                light.light_y = 8.0f;
            }
            break;
        case 1:
            gameCharacters[i].x = gamePacket[i].getX() - gameCharacters[i].x_scale;
            gameCharacters[i].y = gamePacket[i].getY();
            if (i == packetclient.GetPlayerNumber())
            {
                light.cameraRotation = 270.0f;
                light.camera_x = 2.0f;
                light.camera_y = 0;
                light.light_y = 8.0f;
            }
            break;
        case 2:
            gameCharacters[i].y = gamePacket[i].getY() - gameCharacters[i].y_scale;
            gameCharacters[i].x = gamePacket[i].getX();
            if (i == packetclient.GetPlayerNumber())
            {
                light.cameraRotation = 180.0f;
                light.camera_x = 0.0f;
                light.camera_y = -2.0f;
                light.light_y = -4.0f;
            }
            break;
        case 3:
            gameCharacters[i].x = gamePacket[i].getX() + gameCharacters[i].x_scale;
            gameCharacters[i].y = gamePacket[i].getY();
            if (i == packetclient.GetPlayerNumber())
            {
                light.cameraRotation = 90.0f;
                light.camera_x = -2.0f;
                light.camera_y = 0.0f;
                light.light_y = 8.0f;
            }
            break;
        default:
            break;
        }

    }

    bool allDead = true;
    bool anyBlowingUp = false;

    for (int i = 0; i < 4; ++i) {
        if (alive[i]) {
            allDead = false;
            break;
        }
        if (gameCharacters[i].m_bIsBlowingUp) {
            anyBlowingUp = true;
        }
    }

    if (allDead && !anyBlowingUp) {
        std::cout << "게임 종료" << std::endl;
        std::cout << winner << "번째 플레이어" << "승리" << std::endl;
        gamefinish = true; // 게임 종료 상태 설정
    }

    InitBuffer();
    glutPostRedisplay();

    if (game_check && !gamefinish) {
        glutTimerFunc(10, update, 1);
    }
}

void updateKeyState(char key, bool isPressed) {
    if (!gamePacket[packetclient.GetPlayerNumber()].GetSurvivingBit()) return;
    // 해당 키가 유효한지 확인
    if (keyStates.find(key) == keyStates.end()) return;

    // 현재 키 상태와 동일하면 무시
    if (keyStates[key] == isPressed) return;

    // 키 상태 업데이트
    keyStates[key] = isPressed;

    // Packet의 상태 갱신
    packetclient.setKeyState(key, isPressed);

    // W, A, D, C의 비트 상태 출력
    std::cout << "W: " << packetclient.getKeyState('w') << ", ";
    std::cout << "A: " << packetclient.getKeyState('a') << ", ";
    std::cout << "D: " << packetclient.getKeyState('d') << ", ";
    std::cout << "C: " << packetclient.getKeyState('c') << "\n";

    std::cout << "플레이어 번호 : " << packetclient.GetPlayerNumber() << "Valid Bit : " << packetclient.GetValidBit() << std::endl;

    send(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);

}

// 키 입력 이벤트 처리
void keyboard(unsigned char key, int x, int y) {
    updateKeyState(key, true); // 키 눌림 처리
}

void keyUp(unsigned char key, int x, int y) {
    updateKeyState(key, false); // 키 떼짐 처리
}

GLvoid MousePoint(int button, int state, int x, int y) {
}

GLvoid Motion(int x, int y) {

}

GLvoid object_test() {

    for (int i = 0; i < 10; ++i)
    {
        object new_object;

        int model = random_model(engine2);


        if (model == 1) {
            new_object.init(RockPosVbo, RockNomalVbo);
            new_object.object_num = RockObject;
        }
        else if (model == 2)
        {
            new_object.init(teapotPosVbo, teapotNomalVbo);
            new_object.object_num = teapotObject;
        }
        else
        {
            new_object.init(spherePosVbo, sphereNomalVbo);
            new_object.object_num = sphereObject;
        }

        objects.push_back(new_object);
    }

    InitBuffer();
    glutPostRedisplay();
}