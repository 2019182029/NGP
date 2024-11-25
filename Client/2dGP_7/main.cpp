#include "main.h"
#include "obj.h"
#include "class.h"
#include "packet.h"
#include "Common.h"
#include "resource.h"

#include <unordered_map>

#pragma comment(lib, "winmm")
#include <mmsystem.h>

#include <Commctrl.h> // ���� ��Ʈ�� ��� �߰�

#pragma comment(lib, "Comctl32.lib") // ���� ��Ʈ�� ���̺귯�� ��ũ

#define SERVERPORT 9000
#define BUFSIZE    512

#define TESTNUM    0

// ��ȭ���� ���ν���
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ����Ʈ ��Ʈ�� ��� �Լ�
void DisplayText(const char* fmt, ...);
// ���� �Լ� ���� ���
void DisplayError(const char* msg);
// GUI ó�� �� ���� ����� ���� ������ �Լ�
DWORD WINAPI GuiAndClientMain(LPVOID arg);

SOCKET sock;
char buf[BUFSIZE + 1];
HWND hReadyButton, hCancelButton, hExitButton, hEditIP, hConnectButton; // Added hConnectButton for connect functionality
HINSTANCE g_inst;

//�غ�Ϸ� �غ� ��� 
HANDLE hReceiveThread = NULL;
bool gameReady = false;


GLuint vao;

Packet packetclient;

std::array<Packet, 4> gamePacket;

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

std::unordered_map<char, bool> keyStates = {
    {'w', false},
    {'a', false},
    {'d', false},
    {'c', false}
};

std::default_random_engine engine2(std::random_device{}());
std::uniform_real_distribution<double> random_model(1, 6);

obs wall;

std::array<obss, 4> gameCharacters = {
    obss(cubePosVbo2, cubeNomalVbo2),  // 1st element
    obss(cubePosVbo2, cubeNomalVbo2),  // 2nd element
    obss(cubePosVbo2, cubeNomalVbo2),  // 3rd element
    obss(cubePosVbo2, cubeNomalVbo2)   // 4th element
};

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
    // ���� ��� ��� ����
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // ���� ����Ʈ ��������
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // ���� ���� ��ȯ
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // ���� ���� ��� ����
    gluOrtho2D(0, viewport[2], 0, viewport[3]);

    // �𵨺� ���� �ٽ� ��ȯ
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
            // ���� ���� ��Ʈ�� Ȯ���ϰ� GUI ���� �� ������ ����
            if (recvPacket.GetStartBit()) {
                EndDialog(hDlg, IDCANCEL); // GUI ��ȭ ���ڸ� ����
                ExitThread(0);             // ������ ����
            }
        }
        else if (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            DisplayError("recv()");
            ExitThread(0); // ���� �� ������ ����
        }
        Sleep(10); // Small delay to avoid high CPU usage
    }

    return 0;
}

// GUI �� ���� ����� ó���ϴ� ������ �Լ�
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
            DisplayText("�غ� �Ϸ�Ǿ����ϴ�. ������ ����Ǿ����ϴ�. IP �ּ�: %s\n", buf);
            EnableWindow(hReadyButton, TRUE);
            EnableWindow(hConnectButton, FALSE);  // Disable "Connect" button after successful connection
            return TRUE;
        }

        // Ready ������ �� ���� �����带 ����
        case ID_READY:
            packetclient.SetReadyBit(true);
            send(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);

            EnableWindow(hReadyButton, FALSE);
            EnableWindow(hCancelButton, TRUE);
            DisplayText("Ready status confirmed.\n");

            // hDlg �ڵ��� ���ڷ� �����Ͽ� �����带 ����
            gameReady = true;
            hReceiveThread = CreateThread(NULL, 0, ReceiveDataThread, hDlg, 0, NULL);
            return TRUE;

        case ID_CANCEL:
            // Packet���� �غ� ��Ʈ�� ����
            packetclient.SetReadyBit(false);

            // ������ ����� Packet�� ����
            send(sock, reinterpret_cast<char*>(&packetclient), sizeof(packetclient), 0);

            // ������ ������ �����ϰ� �����带 ����
            gameReady = false;
            if (hReceiveThread) {
                WaitForSingleObject(hReceiveThread, INFINITE);
                CloseHandle(hReceiveThread);
                hReceiveThread = NULL;
            }

            // ��ư ���� ����
            EnableWindow(hReadyButton, TRUE);
            EnableWindow(hCancelButton, FALSE);
            DisplayText("�غ� ���°� ��ҵǾ����ϴ�.\n");

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
    g_inst = GetModuleHandle(NULL); // �ν��Ͻ� �ڵ��� main���� ���� ������

    // ���� ��Ʈ�� �ʱ�ȭ
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_INTERNET_CLASSES;
    InitCommonControlsEx(&icex);

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // GUI �� ���� ��� ������ ����
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

    for (int i = 0; i < 4; ++i) {
        gameCharacters[i].init(cubePosVbo2, cubeNomalVbo2);  // Initialize with the appropriate VBOs
        gameCharacters[i].Object = CubeObject;  // Set the Object for each gameCharacter

        switch (i) {
        case 0:  // 0�� �÷��̾�
            gamePacket[i].SetPosition(0.0f, 0.0f);
            gamePacket[i].SetCurrentSurface(0);  // �Ʒ���
            break;

        case 1:  // 1�� �÷��̾�
            gamePacket[i].SetPosition(2.0f, 2.0f);
            gamePacket[i].SetCurrentSurface(1);  // �Ʒ���
            break;

        case 2:  // 2�� �÷��̾�
            gamePacket[i].SetPosition(0.0f, 4.0f);
            gamePacket[i].SetCurrentSurface(2);  // �Ʒ���
            break;

        case 3:  // 3�� �÷��̾�
            gamePacket[i].SetPosition(-2.0f, 2.0f);
            gamePacket[i].SetCurrentSurface(3);  // �Ʒ���
            break;

        default:
            break;
        }
    }

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

    int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos"); //--- lightPos �� ����: (0.0, 0.0, 5.0);
    glUniform3f(lightPosLocation, light.light_x, light.light_y, light.light_z);
    int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor"); //--- lightColor �� ����: (1.0, 1.0, 1.0) ���
    glUniform3f(lightColorLocation, light.light_r, light.light_g, light.light_b);
    unsigned int lighton = glGetUniformLocation(shaderProgramID, "light");
    glUniform1i(lighton, 1);

    unsigned int ambiont = glGetUniformLocation(shaderProgramID, "amb");

    glUniform3f(ambiont, 0.0,0.0,0.0);

    int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor"); //--- object Color�� ����: (1.0, 0.5, 0.3)�� ��

    int modelMatrixLocation = glGetUniformLocation(shaderProgramID, "modelMatrix");
    int viewLoc = glGetUniformLocation(shaderProgramID, "view"); //--- ���ؽ� ���̴����� ���� ��ȯ ��� �������� �޾ƿ´�.
    int projLoc = glGetUniformLocation(shaderProgramID, "projection"); //--- ���ؽ� ���̴����� ���� ��ȯ ��� �������� �޾ƿ´�.

    glm::vec3 cameraPos = glm::vec3(light.camera_x, light.camera_y, light.camera_z); //--- ī�޶� ��ġ
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -200.0f); //--- ī�޶� �ٶ󺸴� ����
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 vTransform(1.0f);
    glm::mat4 pTransform(1.0f);

    vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);

    vTransform = glm::rotate(vTransform, glm::radians(light.cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // z������ ȸ��

    pTransform = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 190.0f); //--- ���� ���� ����: fovy, aspect, near, far

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

    int viewPosLocation = glGetUniformLocation(shaderProgramID, "viewPos"); //--- viewPos �� ����: ī�޶� ��ġ
    glUniform3f(viewPosLocation, cameraPos.x, cameraPos.y, cameraPos.z);

    //�����¿� �� �׸��� �ݺ���
    for (int i = 0; i < 4; i++) {
        // �� ��� �ʱ�ȭ
        glm::mat4 modelMatrix(1.0f);
        // �� ����� ���̴��� ����
        if (light.cameraRotation == 180) {
            if (i == 0) {

                modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 4.0f, 0)); // �̵�
            }
            else if (i == 1)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 2.0f, 0)); // �̵�
                modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
            }
            else if (i == 2)
            {
                modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
            }
            else if (i == 3)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 2.0f, 0.0f)); // �̵�
                modelMatrix = glm::rotate(modelMatrix, glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
            }
        }
        else
        {
            if (i == 0) {

            }
            else if (i == 1)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 2.0f, 0.0f)); // �̵�
                modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
            }
            else if (i == 2)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 4.0f, 0)); // �̵�
                modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
            }
            else if (i == 3)
            {
                modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 2.0f, 0)); // �̵�
                modelMatrix = glm::rotate(modelMatrix, glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
            }
        }
        modelMatrix = glm::translate(modelMatrix, glm::vec3(wall.x, wall.y, wall.z)); // �̵�
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

    // ���� ĳ����
    for (int i = 0; i < 4; ++i) {
        // �� ��� �ʱ�ȭ
        glm::mat4 modelMatrix(1.0f);

        // �� ����� ���̴��� ����
        modelMatrix = glm::translate(modelMatrix, glm::vec3(gameCharacters[i].x, gameCharacters[i].y, gameCharacters[i].z)); // �̵�
        modelMatrix = glm::scale(modelMatrix, glm::vec3(gameCharacters[i].x_scale, gameCharacters[i].y_scale, gameCharacters[i].z_scale));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(-light.cameraRotation), glm::vec3(0.0f, 0.0f, 1.0f)); // z������ ȸ��

        // ���� ����
        glUniform4f(objColorLocation, gameCharacters[i].r, gameCharacters[i].g, gameCharacters[i].b, 1.0f);
        if (i == TESTNUM)
        {
            glUniform4f(objColorLocation, 0.0f, gameCharacters[i].g, gameCharacters[i].b, 1.0f);
        }
        // �� ����� ���̴��� ����
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

        // ���� ���ε�
        glBindBuffer(GL_ARRAY_BUFFER, gameCharacters[i].vvbo);
        glVertexAttribPointer(PosLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(PosLocation);

        glBindBuffer(GL_ARRAY_BUFFER, gameCharacters[i].nvbo);
        glVertexAttribPointer(NomalLocation, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glEnableVertexAttribArray(NomalLocation);

        // ������Ʈ �׸���
        glDrawArrays(GL_TRIANGLES, 0, gameCharacters[i].Object); // gameCharacters[i].Object �� �׸� �ﰢ���� ����
    }

    //��ֹ��� �׸��� �ݺ���
    for (int i = 0; i < objects.size(); i++){
        // �� ��� �ʱ�ȭ
        glm::mat4 modelMatrix(1.0f);
        // �� ����� ���̴��� ����
        modelMatrix = glm::translate(modelMatrix, glm::vec3(objects[i].x, objects[i].y, objects[i].z)); // �̵�

        modelMatrix = glm::rotate(modelMatrix, glm::radians(objects[i].rotate), glm::vec3(0.0f, 0.0f, 1.0f)); // X �� ȸ��
        modelMatrix = glm::rotate(modelMatrix, glm::radians(objects[i].rotate), glm::vec3(1.0f, 0.0f, 0.0f)); // X �� ȸ��
        modelMatrix = glm::rotate(modelMatrix, glm::radians(objects[i].rotate), glm::vec3(0.0f, 1.0f, 0.0f)); // X �� ȸ��
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

    // ���� ����Ʈ ũ�⸦ ����ϴ�.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int windowWidth = viewport[2];
    int windowHeight = viewport[3];

    // ���� ���� ����
    setOrthographicProjection();

    glUseProgram(0);

    // �ؽ�Ʈ ������
    glPushMatrix();
    glLoadIdentity();

    // �ؽ�Ʈ�� ��ġ�� ȭ�� ���� ������� ����
    float x = windowWidth - 100; // ȭ�� �ʺ񿡼� 100px ������ ��ġ
    float y = windowHeight - 30; // ȭ�� ���̿��� 30px ������ ��ġ
    renderBitmapString(x, y, GLUT_BITMAP_HELVETICA_18, ("HP: " + std::to_string(gameCharacters[1].hp)).c_str());

    glPopMatrix();

    // ���� �������� ����
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
        std::cout << "ERROR: vertex shader ������ ����\n" << errorLog << std::endl;
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
        std::cout << "ERROR: fragment shader ������ ����\n" << errorLog << std::endl;
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
    for (int i = 0; i < objects.size(); ++i)
    {
        objects[i].move();
    }

    for (int i = 0; i < 4; ++i)
    {
        switch (gamePacket[i].GetCurrentSurface())
        {
        case 0:
            gameCharacters[i].y = gamePacket[i].getY() + gameCharacters[i].y_scale ;
            gameCharacters[i].x = gamePacket[i].getX();
            if (i == TESTNUM)
            {
                light.cameraRotation = 0;
                light.camera_x = 0.0f;
                light.camera_y = 2.0f;
            }
            break;
        case 1:
            gameCharacters[i].x = gamePacket[i].getX() - gameCharacters[i].x_scale;
            gameCharacters[i].y = gamePacket[i].getY();
            if (i == TESTNUM)
            {
                light.cameraRotation = 270.0f;
                light.camera_x = 2.0f;
                light.camera_y = 0;
            }
            break;
        case 2:
            gameCharacters[i].y = gamePacket[i].getY() - gameCharacters[i].y_scale;
            gameCharacters[i].x = gamePacket[i].getX();
            if (i == TESTNUM)
            {
                light.cameraRotation = 180.0f;
                light.camera_x = 0.0f;
                light.camera_y = -2.0f;
            }
            break;
        case 3:
            gameCharacters[i].x = gamePacket[i].getX() + gameCharacters[i].x_scale;
            gameCharacters[i].y = gamePacket[i].getY();
            if (i == TESTNUM)
            {
                light.cameraRotation = 90.0f;
                light.camera_x = -2.0f;
                light.camera_y = 0.0f;
            }
            break;
        default:
            break;
        }
        std::cout << light.cameraRotation << std::endl;
    }
    InitBuffer();
    glutPostRedisplay();

    if (game_check) {
        glutTimerFunc(30, update, 1);
    }
}

void updateKeyState(char key, bool isPressed) {
    // �ش� Ű�� ��ȿ���� Ȯ��
    if (keyStates.find(key) == keyStates.end()) return;

    // ���� Ű ���¿� �����ϸ� ����
    if (keyStates[key] == isPressed) return;

    // Ű ���� ������Ʈ
    keyStates[key] = isPressed;

    // Packet�� ���� ����
    packetclient.setKeyState(key, isPressed);

    // W, A, D, C�� ��Ʈ ���� ���
    std::cout << "W: " << packetclient.getKeyState('w') << ", ";
    std::cout << "A: " << packetclient.getKeyState('a') << ", ";
    std::cout << "D: " << packetclient.getKeyState('d') << ", ";
    std::cout << "C: " << packetclient.getKeyState('c') << "\n";
}

// Ű �Է� �̺�Ʈ ó��
void keyboard(unsigned char key, int x, int y) {
    updateKeyState(key, true); // Ű ���� ó��
}

void keyUp(unsigned char key, int x, int y) {
    updateKeyState(key, false); // Ű ���� ó��
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

            // ������ �߽�(400)���� ���콺 ��ġ�� �Ÿ� ���
            GLfloat distanceFromCenter = mouseX2; // ������ �߽��� �������� �Ÿ� ���
            GLfloat rotationChange = distanceFromCenter * 0.005f; // �ʿ信 ���� ���� ����

            
            if (light.cameraRotation == 0)
            {
                light.light_y = 8.0f;
            }
            else if (light.cameraRotation == 270)
            {
                light.light_y = 8.0f;

            }
            else if (light.cameraRotation == 180)
            {
                light.light_y = -4.0f;
            }
            else if (light.cameraRotation == 90)
            {
                light.light_y = 8.0f;
            }


            if(sever_level > 3)
            {
                if (gameCharacters[1].x + gameCharacters[1].x_scale > 2.0f || gameCharacters[1].x - gameCharacters[1].x_scale < -2.0f)
                {
                    gameCharacters[1].x -= rotationChange;
                }
            }
        }
        
        
        InitBuffer();
        glutPostRedisplay();
    }
}


bool checkCollision(object_won& sphere, obss& wall) {
    // AABB - �� �浹
    float closestX = std::max(wall.x - wall.x_scale, std::min(sphere.x, wall.x + wall.x_scale));
    float closestY = std::max(wall.y - wall.y_scale, std::min(sphere.y, wall.y + wall.y_scale));
    float closestZ = std::max(wall.z - wall.z_scale, std::min(sphere.z, wall.z + wall.z_scale));

    // ���� �߽ɰ� ���� ����� �� ������ �Ÿ��� ���
    float distanceX = sphere.x - closestX;
    float distanceY = sphere.y - closestY;
    float distanceZ = sphere.z - closestZ;

    // �Ÿ��� ���� ���������� ������ ����������
    float radius = sphere.x_scale;
    return (distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ) < (radius * radius);
}



GLvoid object_ok(int value) {

    if (objects.size() < 10) {
        object_won new_object;
        if (sever_level > 1) {
            int model = random_model(engine2);
            if (model == 1) {
                new_object.init(RockPosVbo, RockNomalVbo); // ��ü �ʱ�ȭ
                new_object.object_num = RockObject;
            }
            else if (model == 2)
            {
                new_object.init(cubePosVbo2, cubeNomalVbo2); // ��ü �ʱ�ȭ
                new_object.object_num = CubeObject;
            }
            else if( model == 3)
            {
                new_object.init(hpPosVbo, hpNomalVbo); // ��ü �ʱ�ȭ
                new_object.object_num = sphereObject;
            }
            else
            {
                new_object.init(teapotPosVbo, teapotNomalVbo); // ��ü �ʱ�ȭ
                new_object.object_num = teapotObject;
            }
        }
        else {
            new_object.init(cubePosVbo2, cubeNomalVbo2); // ��ü �ʱ�ȭ
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

            gameCharacters[1].jump_scale = 0;
            gameCharacters[1].x = 0;
            gameCharacters[1].y = 0.25f;
            gameCharacters[1].z = -1.0f;
            objects.clear();
            glutTimerFunc(900, object_ok, 1);
        }

        glutTimerFunc(30000, next_stage, 1);
        InitBuffer();
        glutPostRedisplay();
    }
}

