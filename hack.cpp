#include <windows.h>
#include <scrnsave.h>
#include <gl/gl.h>
#include <gl/glut.h>
#include <stdio.h>
#include <process.h>
#include <vector>
#include <fstream>
#include <cstdio>
#include <random>
#include <iostream>
#include <algorithm>
#include <time.h>
#include <string>
#include <sstream>
#include <deque>
#include <exception>

#define TEXTURES 3
#define WINDOW_LINES 22
#define LINE_CHARS 55

void EnableOpenGL(void);
void DisableOpenGL(HWND);
HDC hDC;
HGLRC hRC;
int wx, wy;
int finish = 0;

bool panicMode = false;
int max_windows = 9;
int max_panic_windows = 150;
int wait_time = 50;

using namespace std;

typedef struct {
    int width;
    int height;
    GLubyte *bits;
} BmpImage;

// random_device seedGen; // https://cpprefjp.github.io/reference/random/random_device.html
// mingw では常に固定の乱数(は？)が返ってくるからだめっぽい
default_random_engine engine((unsigned int)time(NULL));


class WindowInfo {
    public:
        int windowType = -1;
        vector<string> codes;

        WindowInfo(int wtype) {
            windowType = wtype;
        }
};

int freadDWORD(FILE *fp)
{
    int value[4];
    for (int i = 0; i < 4; i++)
        value[i] = fgetc(fp);
    return 256 * 256 * 256 * value[3] + 256 * 256 * value[2] + 256 * value[1] + value[0];
}

BmpImage *loadBmp(string filename)
{
    BmpImage *ret = new BmpImage;
    int offset;
    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "failed to open");
        exit(0);
    }

    fseek(fp, 10, SEEK_SET);
    offset = freadDWORD(fp);
    fseek(fp, 18, SEEK_SET);
    ret->width = freadDWORD(fp);
    ret->height = freadDWORD(fp);

    fseek(fp, offset, SEEK_SET);
    ret->bits = new GLubyte[(ret->width) * (ret->height) * 3];
    if (ret->bits == NULL)
    {
        fprintf(stderr, "allocation failed");
        exit(0);
    }
    for (int i = 0; i < (ret->width) * (ret->height); i++)
    {
        ret->bits[i * 3 + 2] = fgetc(fp);
        ret->bits[i * 3 + 1] = fgetc(fp);
        ret->bits[i * 3 + 0] = fgetc(fp);
    }

    fclose(fp);
    return ret;
}

int parseInt(string str) {
    int ret = 0;
    try {
        ret = stoi(str);
        // cout << "[parseInt] parsed " << ret << endl;
    } catch (exception& e) {
        cout << "[parseInt] Failed to parse int from " << str << endl;
    }

    return ret;
}

void parseFile(WindowInfo* info, string filename) {
    string line;
    ifstream codeFile(filename, ios::in);
    if (!codeFile.is_open()) {
        cout << "[parseFile] Failed to open " << filename << endl;
        return;
    }

    while (getline(codeFile, line))
        info->codes.push_back(line);
}

WindowInfo *getWindowInfo(ifstream *ifs, string type) {
    string line, cmd, arg;
    int windowType = -1;

    if (type == "mac" || type == "macos")
        windowType = 0;
    else if (type == "win" || type == "windows")
        windowType = 1;
    else if (type == "linux" || type == "gnome")
        windowType = 2;

    WindowInfo *ret = new WindowInfo(windowType);
    while (getline(*ifs, line)) {
        cmd = arg = "";
        stringstream ls(line);
        getline(ls, cmd, ' ');
        getline(ls, arg, ' ');

        if (cmd == "!!!end")
            break;
        else if (cmd == "!!!file")
            parseFile(ret, arg);
        else
            ret->codes.push_back(line);
    }
    
    return ret;
}

vector<WindowInfo*> *parseInfo() {
    vector<WindowInfo*> *ret = new vector<WindowInfo*>;
    ifstream windowFile("window.txt", ios::in);
    string line, cmd, arg;
    if (!windowFile.is_open()) {
        cout << "[parseInfo] Failed to open window.txt" << endl;
        exit(0);
    }

    while (getline(windowFile, line)) {
        cmd = arg = "";
        stringstream ls(line);
        getline(ls, cmd, ' ');
        getline(ls, arg, ' ');
        // cout << "cmd: \"" << cmd << "\", arg: \"" << arg << "\"" << endl;

        if (cmd == "!!!code")
            ret->push_back(getWindowInfo(&windowFile, arg));
        else if (cmd == "!!!panic") {
            panicMode = true;
            int parsed = parseInt(arg);
            if (parsed)  {
                max_panic_windows = parsed;
                cout << "[parseInfo] max_panic_windows is set to " << max_panic_windows << endl;
            }
        } else if (cmd == "!!!wait") {
            int parsed = parseInt(arg);
            if (parsed) {
                wait_time = parsed;
                cout << "[parseInfo] wait_time is set to " << wait_time << endl;
            }
        } else if (cmd == "!!!max") {
            int parsed = parseInt(arg);
            if (parsed) {
                max_windows = parsed;
                cout << "[parseInfo] max_windows is set to " << max_windows << endl; 
            }
        }
    }

    // cout << "[parseInfo] parsed below:" << endl;
    // for (auto& r: *ret) {
    //     cout << r->windowType << endl;
    //     for (auto& cl: r->codes)
    //         cout << cl << endl;
    // }

    return ret;
}

void drawChar(char c, int x, int y, float r, float g, float b) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
        glColor3f(r, g, b); // White
        glRasterPos2d(x, y);
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
    glPopAttrib();
}

class CodeWindow
{
private:
    static GLuint textures[TEXTURES];
    static int resX;
    static int resY;

    vector<string> codes;
    int windowType;

    bool isFinished = false;
    int waitWindow = 9;
    int waitCursor = 15;
    int waitFinish = 12;
    int posLine = 0;
    int posChar = 0;
    int posX;
    int posY;
    int topX;
    int topY;
public:
    bool isDead = false;
    bool isPanic = false;

    static void initTextures(int wx, int wy);
    CodeWindow();
    CodeWindow(WindowInfo *info);
    CodeWindow *spawn();
    void drawWindow();
    void drawWindowChar(int pline, int pchar, char c);
    void drawString();
    void render();
};

GLuint CodeWindow::textures[TEXTURES];
int CodeWindow::resX;
int CodeWindow::resY;

void loadTextureFromBmp(GLuint textureId, string filename)
{
    BmpImage *image = loadBmp(filename);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->bits);
    cout << "[loadTextureFromBmp] loaded " << filename << " (" << image->width << "x" << image->height << ") to tex " << textureId << endl;
}

void CodeWindow::initTextures(int wx, int wy)
{
    resX = wx;
    resY = wy;
    glGenTextures(TEXTURES, textures);
    loadTextureFromBmp(textures[0], "mac512.bmp");
    loadTextureFromBmp(textures[1], "win512.bmp");
    loadTextureFromBmp(textures[2], "gnome512.bmp");
    glBindTexture(GL_TEXTURE_2D, 0);
}

CodeWindow::CodeWindow() {
    isPanic = true;
    uniform_int_distribution<> randW(0, TEXTURES-1);
    windowType = randW(engine);
}

CodeWindow::CodeWindow(WindowInfo *info)
{
    codes = info->codes;
    windowType = info->windowType;
    if (windowType == -1) {
        uniform_int_distribution<> randW(0, TEXTURES-1);
        windowType = randW(engine);
    }
}

CodeWindow *CodeWindow::spawn()
{
    // spawnableX: -300 to wx-724 -> 0 to wx-300*2 -300
    //          Y: -150 to wy-570 -> 0 to wy-150*2 - 150
    uniform_int_distribution<> randX(0, resX-212);
    uniform_int_distribution<> randY(0, resY-210);

    posX = randX(engine) - 150;
    posY = randY(engine) - 75;
    topX = posX + 5;
    topY = posY + 360 - 35;

    return this;
}

void CodeWindow::drawWindow()
{
    glPushMatrix(); glPushAttrib(GL_ALL_ATTRIB_BITS);
        glBindTexture(GL_TEXTURE_2D, textures[windowType]); // must bind texture before draw polygon!
        // glScalef(0.5, 0.5, 1);
        glBegin(GL_POLYGON);
            // glBindTexture(GL_TEXTURE_2D, textures[tex]);
            // glTexCoord2f -> テクスチャ座標, glVertex2f -> ポリゴン点座標
            glTexCoord2f(0, 0.2968); glVertex2f(posX, posY);
            glTexCoord2f(0, 1); glVertex2f(posX, posY+360);
            glTexCoord2f(1, 1); glVertex2f(posX+512, posY+360);
            glTexCoord2f(1, 0.2968); glVertex2f(posX+512, posY); // 0.2968 = 1 - height / width
        glEnd();
    glPopMatrix(); glPopAttrib();
}

void CodeWindow::drawWindowChar(int pline, int pchar, char c) {
    // cout << "[drawWindowChar] " << c  << " to " << pline << ", " << pchar << endl;
    drawChar(c, topX+9*pchar, topY-15*pline, 1, 1, 1);
}

void CodeWindow::drawString() {
    // wait window for some ticks...
    if (waitWindow) {
        --waitWindow;
        return;
    }

    // same for cursor
    if (waitCursor) {
        if (waitCursor % 2)
            drawWindowChar(0, 0, '_');
        --waitCursor;
        return;
    }

    if (!codes.size()) {
        isDead = true;
        return;
    }

    int l, c;
    int startLine, endLine;
    int startChar, endChar;
    
    if (posLine < WINDOW_LINES) {
        startLine = 0;
        endLine = posLine;
    } else {
        startLine = posLine - WINDOW_LINES + 1;
        endLine = startLine + WINDOW_LINES - 1;
    }

    // draw all previous lines
    for(l = startLine; l < endLine; l++) {
        int clen = codes[l].length();
        for (c = 0; c < (clen < LINE_CHARS ? clen : LINE_CHARS); c++) {
            drawWindowChar(l-startLine, c, codes[l][c]);
        }
    }

    // then draw latest line...
    if (posChar < LINE_CHARS-1) {
        startChar = 0;
        endChar = posChar;
    } else {
        startChar = posChar - LINE_CHARS + 1;
        endChar = startChar + LINE_CHARS - 1;
    }

    for (c = startChar; c < endChar; c++) {
        drawWindowChar(endLine-startLine, c-startChar, codes[endLine][c]);
    }
    // if (posChar != LINE_CHARS-1)
    //     drawWindowChar(endLine-startLine, endChar-startChar+1, '_');
    drawWindowChar(endLine-startLine, c-startChar, '_');

    if (isFinished) {
        --waitFinish;
        if (!waitFinish)
            isDead = true;
        return;
    } else if (c == codes[endLine].length()) { // FIXME
        ++posLine;
        posChar = 0;
        if (posLine == codes.size())
            isFinished = true;
    } else {
        ++posChar;
    }
}

void CodeWindow::render()
{
    drawWindow();
    drawString();

    if (isPanic)
        isDead = false;
}

unsigned __stdcall disp(void *arg)
{
    EnableOpenGL(); // OpenGL設定

    int argc = 1;
    glutInit(&argc, NULL);

    /* OpenGL初期設定 */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glOrtho(-50.0*wx/wy, 50.0*wx/wy, -50.0, 50.0, -50.0, 50.0); // 座標系設定
    glOrtho(0, wx, 0, wy, -50.0, 50.0); // 0, 0 to bottom left

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(66.0/255, 134.0/255, 244.0/255, 0); // glClearするときの色設定
    // glColor3f(0.9, 0.8, 0.7); // glRectf等で描画するときの色設定
    glViewport(0, 0, wx, wy);

    glEnable(GL_TEXTURE_2D);

    cout << "[disp] screen resolution: " << wx << "x" << wy << endl;
    // one window is 512x360
    max_windows = wx/512 * wy/360;
    cout << "[disp] suggested max windows (auto set): " << max_windows << endl;


    vector<WindowInfo*> *infos = parseInfo();
    uniform_int_distribution <> randI(0, infos->size() - 1);

    vector<CodeWindow*> windows;
    deque<CodeWindow*> panicWindows;
    CodeWindow::initTextures(wx, wy);
    /* 表示関数呼び出しの無限ループ */
    if (panicMode) {
        cout << "[disp] start panic mode" << endl;
        while (1) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto& w: panicWindows)
                w->render();
            // cout << "[disp] rendered " << windows.size() << " windows!" << endl;

            CodeWindow *window = new CodeWindow();
            panicWindows.push_back(window->spawn());

            if (panicWindows.size() > max_panic_windows)
                panicWindows.pop_front();

            glFlush();        // 画面描画強制
            SwapBuffers(hDC); // front bufferとback bufferの入れ替え
            if (finish == 1) // finishが1なら描画スレッドを終了させる
                return 0;
        }
    } else {
        cout << "[disp] start normal mode" << endl;
        while (1)
        {
            Sleep(wait_time); // 0.015秒待つ
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto& w: windows)
                w->render();
            // cout << "[disp] rendered " << windows.size() << " windows!" << endl;

            if (windows.size() < max_windows) {
                CodeWindow *window = new CodeWindow((*infos)[randI(engine)]);
                windows.push_back(window->spawn());
            }

            windows.erase(remove_if(windows.begin(), windows.end(), [](CodeWindow *x){return x->isDead;}), windows.end());

            glFlush();        // 画面描画強制
            SwapBuffers(hDC); // front bufferとback bufferの入れ替え
            if (finish == 1) // finishが1なら描画スレッドを終了させる
                return 0;
        }
    }
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE thread_id1;
    RECT rc;

    switch (msg)
    {
    case WM_CREATE:
        hDC = GetDC(hWnd);
        GetClientRect(hWnd, &rc);
        wx = rc.right - rc.left;
        wy = rc.bottom - rc.top;

        thread_id1 = (HANDLE)_beginthreadex(NULL, 0, disp, NULL, 0, NULL); // 描画用スレッド生成
        if (thread_id1 == 0)
        {
            fprintf(stderr, "pthread_create : %s", strerror(errno));
            exit(0);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_DESTROY:
        finish = 1;                                // 描画スレッドを終了させるために1を代入
        WaitForSingleObject(thread_id1, INFINITE); // 描画スレッドの終了を待つ
        CloseHandle(thread_id1);
        DisableOpenGL(hWnd);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}

void EnableOpenGL(void)
{
    int format;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
        1,                             // version number
        0 | PFD_DRAW_TO_WINDOW         // support window
          | PFD_SUPPORT_OPENGL         // support OpenGL
          | PFD_DOUBLEBUFFER           // double buffered
        ,
        PFD_TYPE_RGBA,    // RGBA type
        24,               // 24-bit color depth
        0, 0, 0, 0, 0, 0, // color bits ignored
        0,                // no alpha buffer
        0,                // shift bit ignored
        0,                // no accumulation buffer
        0, 0, 0, 0,       // accum bits ignored
        32,               // 32-bit z-buffer
        0,                // no stencil buffer
        0,                // no auxiliary buffer
        PFD_MAIN_PLANE,   // main layer
        0,                // reserved
        0, 0, 0           // layer masks ignored
    };

    /* OpenGL設定 */
    format = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, format, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
    /* ここまで */
}

void DisableOpenGL(HWND hWnd)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
}
