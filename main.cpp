//
// Created by 77361 on 25-5-7.
//

#include "BattleFireDirect.h"
#include "Scene.h"
#include "StaticMeshComponent.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"winmm.lib")

LPCTSTR gWindowClassName = L"BattleFire";
//@param inWParam 里面会存储案件信息
//@param inLParam createwindowex最后一个参数
LRESULT CALLBACK WindowProc(HWND inHWND, UINT inMSG, WPARAM inWParam, LPARAM inLParam)
{
    switch (inMSG)
    {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        PostQuitMessage(0); //会向消息队列中添加WM_QUIT
        break;
    }
    return DefWindowProc(inHWND, inMSG, inWParam, inLParam); //默认的系统处理消息
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int inCmdShow)
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
    wndClassEx.lpfnWndProc = WindowProc;
    if (!RegisterClassEx(&wndClassEx))
    {
        MessageBox(nullptr, L"RegisterClassEx failed!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    //创建窗口
    int viewportWidgth = 1280;
    int viewportHeigth = 720;
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = viewportWidgth;
    rect.bottom = viewportHeigth;
    AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE); //此处会修改rect的值 会包含窗口的边框（比如title  bar）

    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    HWND hwnd = CreateWindowEx(NULL, gWindowClassName, L"My Render Window", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,CW_USEDEFAULT, windowWidth, windowHeight, NULL, NULL,
                               hInstance, NULL);
    if (!hwnd)
    {
        MessageBox(nullptr, L"Create Window failed!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    InitD3D12(hwnd, viewportWidgth, viewportHeigth);
    InitScene(viewportWidgth, viewportHeigth);


    ShowWindow(hwnd, inCmdShow);
    UpdateWindow(hwnd);
    
    DWORD last_time = timeGetTime();
    DWORD appStartTime = last_time;

    MSG msg;
    while (true)
    {
        ZeroMemory(&msg, sizeof(MSG)); //memset C语言风格
        //peekmessage
        //@param1 过滤窗口 null代表接收所有窗口消息
        //@param2 消息id 0代表不过滤 和后面的参数一个大一个小
        //@param4 从消息队列中拿到后移除消息
        if (PeekMessage(&msg, NULL, 0,
                        0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg); //从系统消息转换为应用程序能识别的消息
            DispatchMessage(&msg); //调用dispatchmessage WindowProc就会被调用
        }
        else
        {
            //render
            WaitForCompletionOfCommandList();
            DWORD current_time = timeGetTime(); //ms
            DWORD frameTime = current_time - last_time;
            DWORD timeSinceAppStartInMS = current_time - appStartTime;
            last_time = current_time;
            float frameTimeInSecond = float(frameTime) / 1000.0f;
            float timeSinceAppStartInSecond = float(timeSinceAppStartInMS) / 1000.0f;

            RenderOneFrame(frameTimeInSecond, timeSinceAppStartInSecond);
            SwapD3D12Buffers();
        }
    }
    return 0;
}
