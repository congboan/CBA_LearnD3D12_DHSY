//
// Created by 77361 on 25-5-7.
//
#include <windows.h>

LPCTSTR gWindowClassName = L"BattleFire";

//@param inWParam 里面会存储案件信息
LRESULT CALLBACK WindowProc(HWND inHWND, UINT inMSG, WPARAM inWParam, LPARAM inLParam)
{
    switch (inMSG)
    {
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        PostQuitMessage(0);//会向消息队列中添加WM_QUIT
        break;
    }
    return DefWindowProc(inHWND, inMSG, inWParam, inLParam);//默认的系统处理消息
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //注册窗口
    WNDCLASSEX wndClassEx;
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
    wndClassEx.cbClsExtra = NULL;
    wndClassEx.cbWndExtra = NULL;
    wndClassEx.hInstance = hInstance;
    wndClassEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClassEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClassEx.hbrBackground = NULL;
    wndClassEx.lpszMenuName = NULL;
    wndClassEx.lpszClassName = gWindowClassName;
    if (!RegisterClassEx(&wndClassEx))
    {
        MessageBox(nullptr, L"RegisterClassEx failed!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }


    return 0;
}
