#include <windows.h>
#include <commctrl.h>
#include <string>
#include "pvzclass/pvzclass.h"

#pragma comment(lib, "comctl32.lib")

#define IDC_EDIT_TEXT 1001
#define IDC_BTN_ACTION 1002
#define IDC_STATUS_BAR 1003

HINSTANCE g_hInst = nullptr;
HWND g_hEdit = nullptr;
HWND g_hBtn = nullptr;
HWND g_hStatus = nullptr;
HFONT g_hFont = nullptr;
int g_currentMultiplier = 1;
bool g_pvzFound = false;

// 检测系统版本，Win8+使用微软雅黑UI，Win7使用普通微软雅黑
bool IsWin8OrLater()
{
    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    DWORDLONG dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 2;
    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != FALSE;
}

// 创建字体（根据系统版本选择）
HFONT CreateModernFont(int nHeight = 15)
{
    const wchar_t* fontName = IsWin8OrLater() ? L"Microsoft YaHei UI" : L"Microsoft YaHei";
    return CreateFontW(
        -nHeight,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        fontName
    );
}

// 更新状态栏显示
void UpdateStatusBar()
{
    std::wstring status;
    if (g_pvzFound)
    {
        status = L"\u5f53\u524d\u500d\u7387: \u00d7" + std::to_wstring(g_currentMultiplier);
    }
    else
    {
        status = L"\u672a\u627e\u5230PVZ";
    }
    SendMessage(g_hStatus, SB_SETTEXT, 2, (LPARAM)status.c_str());
}

// 应用加速设置
void ApplySpeedHack(int multiplier)
{
    if (!g_pvzFound) return;
    
    // 启用加速
    PVZ::Memory::WriteMemory<uint8_t>(0x6A9EAB, 1);
    // 设置倍率
    PVZ::Memory::WriteMemory<uint32_t>(0x4526D3, static_cast<uint32_t>(multiplier));
}

// 查找PVZ进程
bool FindPVZ()
{
    DWORD pid = ProcessOpener::Open();
    if (pid)
    {
        PVZ::InitPVZ(pid);
        g_pvzFound = true;
        return true;
    }
    g_pvzFound = false;
    return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_hFont = CreateModernFont(15);

        // 创建EditText - 限制整数输入
        g_hEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
            10, 10, 200, 25,
            hWnd,
            (HMENU)IDC_EDIT_TEXT,
            g_hInst,
            nullptr
        );
        SendMessage(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // 设置提示文字（通过EM_SETCUEBANNER）
        SendMessage(g_hEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"\u8f93\u5165\u6b63\u6574\u6570\u500d\u7387");

        // 创建Button - 加速
        g_hBtn = CreateWindowExW(
            0,
            L"BUTTON",
            L"\u52a0\u901f",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            220, 10, 80, 25,
            hWnd,
            (HMENU)IDC_BTN_ACTION,
            g_hInst,
            nullptr
        );
        SendMessage(g_hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // 创建StatusBar
        g_hStatus = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            L"",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hWnd,
            (HMENU)IDC_STATUS_BAR,
            g_hInst,
            nullptr
        );
        SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // 设置StatusBar三部分：左下角版本、中间标题、右下角倍率
        int parts[] = { 80, 200, -1 };
        SendMessage(g_hStatus, SB_SETPARTS, 3, (LPARAM)parts);
        SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)L"v0.0.1");
        SendMessage(g_hStatus, SB_SETTEXT, 1, (LPARAM)L"PVZ Accelerator");
        
        // 尝试查找PVZ
        FindPVZ();
        UpdateStatusBar();

        // 设置窗口标题
        SetWindowTextW(hWnd, L"PVZ Accelerator v0.0.1");

        // 设置定时器，每2秒检测一次PVZ
        SetTimer(hWnd, 1, 2000, nullptr);

        return 0;
    }

    case WM_TIMER:
    {
        // 定时检测PVZ状态
        bool wasFound = g_pvzFound;
        FindPVZ();
        if (wasFound != g_pvzFound)
        {
            UpdateStatusBar();
        }
        return 0;
    }

    case WM_SIZE:
    {
        SendMessage(g_hStatus, WM_SIZE, 0, 0);
        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_BTN_ACTION)
        {
            if (!g_pvzFound)
            {
                // 尝试重新查找
                if (FindPVZ())
                {
                    UpdateStatusBar();
                }
                else
                {
                    MessageBoxW(hWnd, L"\u672a\u627e\u5230PVZ\u6e38\u620f\u8fdb\u7a0b", L"\u9519\u8bef", MB_OK | MB_ICONERROR);
                }
                return 0;
            }
            
            wchar_t buffer[256];
            GetWindowTextW(g_hEdit, buffer, 256);
            
            // 解析输入的整数
            int value = 0;
            if (buffer[0] != L'\0')
            {
                value = _wtoi(buffer);
                if (value < 1) value = 1;
            }
            else
            {
                value = 1;
            }
            
            g_currentMultiplier = value;
            ApplySpeedHack(g_currentMultiplier);
            UpdateStatusBar();
        }
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        if (g_pvzFound)
        {
            PVZ::QuitPVZ();
        }
        if (g_hFont)
        {
            DeleteObject(g_hFont);
            g_hFont = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&iccex);

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = L"PVZAcceleratorClass";
    wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex))
        return 1;

    HWND hWnd = CreateWindowExW(
        0,
        L"PVZAcceleratorClass",
        L"PVZ Accelerator",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 150,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
        return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
