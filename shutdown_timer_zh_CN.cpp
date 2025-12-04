#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7及以上版本
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <cstdio>
#include <VersionHelpers.h>
#define IDI_MAIN_ICON 101

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// 颜色定义
#define COLOR_BG 0xFFFFFF   // 白色背景
#define COLOR_TEXT 0x323130 // 文字颜色 - 深灰

// 托盘消息
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002
#define ID_TRAY_CANCEL 1003 // 终止倒计时菜单ID

// 全局变量
HWND g_hTimeEdit, g_hStartBtn, g_hCancelBtn, g_hStatusLabel, g_hTitleLabel;
HWND g_hShutdownRadio, g_hRestartRadio, g_hLogoffRadio; // 添加单选按钮句柄
HFONT g_hTitleFont, g_hNormalFont;
int g_remainingSeconds = 0;
bool g_isShutdownScheduled = false;
int g_dpi = 96;
HBRUSH g_hBgBrush = NULL;
bool g_forceShutdown = true; // 仅保留强制关机
int g_shutdownType = 0;      // 0:关机, 1:重启, 2:注销
HINSTANCE g_hInstance;
bool g_threeMinuteNotified = false; // 3分钟提醒标记
NOTIFYICONDATA g_nid = {0};

// 函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void OnStartShutdown(HWND hwnd);
void OnCancelShutdown();
void UpdateTimerDisplay();
bool EnableShutdownPrivilege();
int ScaleValue(int value, int dpi);
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void ShowTrayContextMenu(HWND hwnd, POINT pt);
bool IsInstanceRunning();
void CheckAndWarnAdminPrivilege();
bool IsSystemLocked();
void ShowThreeMinuteWarning(HWND hwnd);

// 创建字体
HFONT CreateCustomFont(int size, const wchar_t *fontName, int weight = FW_NORMAL)
{
    return CreateFontW(
        -MulDiv(size, g_dpi, 72),
        0, 0, 0, weight,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        fontName);
}

// 显示错误消息
void ShowError(const wchar_t *message)
{
    SetProcessDPIAware();
    MessageBoxW(NULL, message, L"错误", MB_ICONERROR | MB_OK);
}

// DPI缩放
int ScaleValue(int value, int dpi)
{
    return MulDiv(value, dpi, 96);
}

// 检查是否已有实例在运行
bool IsInstanceRunning()
{
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"ModernShutdownTimer_Mutex");
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutex);
        return true;
    }
    return false;
}

// 创建托盘图标
void CreateTrayIcon(HWND hwnd)
{
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    // 使用 LoadIconW 加载图标
    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!g_nid.hIcon)
    {
        // 如果加载失败，使用默认图标
        g_nid.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512)); // 32512 = IDI_APPLICATION
    }

    // 使用 wcscpy_s 拷贝宽字符串
    wcscpy_s(g_nid.szTip, L"定时关机程序");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// 移除托盘图标函数
void RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

// 显示托盘右键菜单
void ShowTrayContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"🔲 显示窗口");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

    // 如果正在倒计时，添加"终止倒计时"选项
    if (g_isShutdownScheduled)
    {
        AppendMenu(hMenu, MF_STRING, ID_TRAY_CANCEL, L"⛔ 终止倒计时");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }

    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"❌ 退出程序");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

// 提升关机权限
bool EnableShutdownPrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
    DWORD lastError = GetLastError();
    CloseHandle(hToken);

    return (result && lastError == ERROR_SUCCESS);
}

// 检查是否以管理员身份运行并给出警告
void CheckAndWarnAdminPrivilege()
{
    BOOL isElevated = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD dwSize = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }

    if (!isElevated)
    {
        MessageBoxW(NULL,
                    L"⚠️ 程序未以管理员身份运行，部分功能可能受限。\n"
                    L"建议以管理员身份重新启动程序以获得完整功能。",
                    L"权限提示", MB_ICONWARNING | MB_OK);
    }
}

// 检查系统是否处于锁定状态
bool IsSystemLocked()
{
    // 尝试获取工作站锁定状态
    HWINSTA hCurrent = GetProcessWindowStation();
    if (hCurrent)
    {
        // 使用替代方法检查工作站锁定状态，兼容更多Windows版本
        DWORD dwFlags;
        if (GetUserObjectInformationW(hCurrent, UOI_FLAGS, &dwFlags, sizeof(dwFlags), NULL))
        {
            // WSF_VISIBLE 标志表示工作站可见，未锁定
            return !(dwFlags & WSF_VISIBLE);
        }
    }
    return false;
}

// 显示3分钟警告
// 在文件顶部添加调试宏
#ifdef _DEBUG
#define DEBUG_PRINT(msg) OutputDebugStringW(msg)
#else
#define DEBUG_PRINT(msg)
#endif

// 修改ShowThreeMinuteWarning函数
void ShowThreeMinuteWarning(HWND hwnd)
{
    DEBUG_PRINT(L"[ShutdownTimer] ShowThreeMinuteWarning called\n");

    // 如果系统已锁定，不显示提醒
    if (IsSystemLocked())
    {
        DEBUG_PRINT(L"[ShutdownTimer] System is locked, skipping notification\n");
        return;
    }

    const wchar_t *actionText = L"关机";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"重启";
        break;
    case 2:
        actionText = L"注销";
        break;
    }

    wchar_t message[256];
    swprintf_s(message, L"⚠️ 距离%s还有3分钟！请做好准备。", actionText);

    DEBUG_PRINT(L"[ShutdownTimer] Message prepared: ");
    DEBUG_PRINT(message);
    DEBUG_PRINT(L"\n");

    // 记录Windows版本
    OSVERSIONINFOEXW osvi = {sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0};
    GetVersionExW((OSVERSIONINFOW *)&osvi);

    wchar_t versionInfo[128];
    swprintf_s(versionInfo, L"[ShutdownTimer] Windows version: %d.%d Build %d\n",
               osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    DEBUG_PRINT(versionInfo);

    // 尝试不同的通知方法
    BOOL notificationSent = FALSE;

    // 方法1：使用Shell_NotifyIconW
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO | NIF_SHOWTIP;
    nid.dwInfoFlags = NIIF_WARNING | NIIF_LARGE_ICON;
    nid.uTimeout = 15000;
    nid.uVersion = NOTIFYICON_VERSION_4;

    wcscpy_s(nid.szInfoTitle, L"定时关机程序");
    wcscpy_s(nid.szInfo, message);

    notificationSent = Shell_NotifyIconW(NIM_MODIFY, &nid);

    wchar_t debugMsg[128];
    swprintf_s(debugMsg, L"[ShutdownTimer] Shell_NotifyIcon returned: %d\n", notificationSent);
    DEBUG_PRINT(debugMsg);

    if (!notificationSent)
    {
        // 如果通知失败，使用消息框
        DEBUG_PRINT(L"[ShutdownTimer] Notification failed, using MessageBox\n");
        MessageBoxW(hwnd, message, L"定时关机程序 - 提醒", MB_ICONWARNING | MB_OK);
    }
    else
    {
        DEBUG_PRINT(L"[ShutdownTimer] Notification sent successfully\n");
    }
}

// 开始关机定时
void OnStartShutdown(HWND hwnd)
{
    if (g_isShutdownScheduled)
    {
        MessageBoxW(hwnd, L"⚠️ 已有定时任务在运行！", L"提示", MB_ICONWARNING);
        return;
    }

    wchar_t buffer[32];
    GetWindowTextW(g_hTimeEdit, buffer, 32);
    int minutes = _wtoi(buffer);

    if (minutes < 0 || minutes > 1440)
    {
        MessageBoxW(hwnd, L"⚠️ 请输入0-1440之间的有效分钟数！\n（最多24小时）", L"错误", MB_ICONERROR);
        return;
    }

    // 重置3分钟提醒标记
    g_threeMinuteNotified = false;

    g_remainingSeconds = minutes * 60;
    g_isShutdownScheduled = true;
    SetTimer(hwnd, 1, 1000, NULL);

    // 禁用所有相关控件
    EnableWindow(g_hStartBtn, FALSE);
    EnableWindow(g_hCancelBtn, TRUE);
    EnableWindow(g_hTimeEdit, FALSE); // 禁用时间输入框

    // 禁用操作类型选择
    EnableWindow(g_hShutdownRadio, FALSE);
    EnableWindow(g_hRestartRadio, FALSE);
    EnableWindow(g_hLogoffRadio, FALSE);

    const wchar_t *actionText = L"关机";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"重启";
        break;
    case 2:
        actionText = L"注销";
        break;
    }

    wchar_t status[128];
    swprintf_s(status, L"🕓 状态：已设定 %d 分钟后%s", minutes, actionText);
    SetWindowTextW(g_hStatusLabel, status);
    UpdateTimerDisplay();
}

// 取消关机
void OnCancelShutdown()
{
    if (!g_isShutdownScheduled)
        return;

    HWND hwnd = GetParent(g_hStartBtn);
    KillTimer(hwnd, 1);
    g_isShutdownScheduled = false;
    g_threeMinuteNotified = false; // 重置提醒标记

    // 重新启用所有相关控件
    EnableWindow(g_hStartBtn, TRUE);
    EnableWindow(g_hCancelBtn, FALSE);
    EnableWindow(g_hTimeEdit, TRUE); // 启用时间输入框

    // 启用操作类型选择
    EnableWindow(g_hShutdownRadio, TRUE);
    EnableWindow(g_hRestartRadio, TRUE);
    EnableWindow(g_hLogoffRadio, TRUE);

    const wchar_t *actionText = L"关机";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"重启";
        break;
    case 2:
        actionText = L"注销";
        break;
    }

    wchar_t status[128];
    swprintf_s(status, L"📋 状态：已取消%s定时", actionText);
    SetWindowTextW(g_hStatusLabel, status);

    HWND hDisplay = GetDlgItem(hwnd, 8);
    SetWindowTextW(hDisplay, L"⏱ 定时已取消");

    wcscpy_s(g_nid.szTip, _countof(g_nid.szTip), L"定时关机程序 - 已取消定时");
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 更新状态显示
void UpdateTimerDisplay()
{
    if (!g_isShutdownScheduled)
        return;

    const wchar_t *actionText = L"关机";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"重启";
        break;
    case 2:
        actionText = L"注销";
        break;
    }

    int hours = g_remainingSeconds / 3600;
    int minutes = (g_remainingSeconds % 3600) / 60;
    int seconds = g_remainingSeconds % 60;

    wchar_t display[256];
    if (hours > 0)
        swprintf_s(display, L"⏱ %s剩余时间：%02d 小时 %02d 分钟 %02d 秒",
                   actionText, hours, minutes, seconds);
    else
        swprintf_s(display, L"⏱ %s剩余时间：%02d 分钟 %02d 秒",
                   actionText, minutes, seconds);

    HWND hDisplay = GetDlgItem(GetParent(g_hStartBtn), 8);
    SetWindowTextW(hDisplay, display);
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // 检查是否已有实例运行
    if (IsInstanceRunning())
    {
        ShowError(L"程序已在运行中！如果找不到，请检查系统托盘");
        return 0;
    }

    g_hInstance = hInstance;
    SetProcessDPIAware();

    // 检查管理员权限并给出警告（不强制）
    CheckAndWarnAdminPrivilege();

    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&icex))
    {
        ShowError(L"初始化通用控件失败");
        return 1;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModernShutdownTimer";
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!wc.hIcon)
    {
        wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512)); // 32512 = IDI_APPLICATION
    }
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc))
    {
        ShowError(L"注册窗口类失败");
        return 1;
    }

    HDC hdc = GetDC(NULL);
    g_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    int windowWidth = ScaleValue(450, g_dpi);
    int windowHeight = ScaleValue(460, g_dpi);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowW(
        L"ModernShutdownTimer", L"定时关机程序",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        (screenWidth - windowWidth) / 2,
        (screenHeight - windowHeight) / 2,
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        ShowError(L"创建窗口失败");
        return 1;
    }

    g_hBgBrush = CreateSolidBrush(COLOR_BG);
    CreateTrayIcon(hwnd);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
    case WM_CREATE:
    {
        g_hTitleFont = CreateCustomFont(16, L"Microsoft YaHei UI", FW_SEMIBOLD);
        g_hNormalFont = CreateCustomFont(10, L"Segoe UI", FW_NORMAL);

        // 标题
        g_hTitleLabel = CreateWindowW(L"STATIC", L"🕓 定时关机程序",
                                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      ScaleValue(20, g_dpi), ScaleValue(20, g_dpi),
                                      ScaleValue(400, g_dpi), ScaleValue(30, g_dpi),
                                      hwnd, NULL, g_hInstance, NULL);

        // 时间输入标签
        CreateWindowW(L"STATIC", L"操作延迟时间（分钟）：",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      ScaleValue(30, g_dpi), ScaleValue(70, g_dpi),
                      ScaleValue(150, g_dpi), ScaleValue(25, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // 时间输入框
        g_hTimeEdit = CreateWindowW(L"EDIT", L"60",
                                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
                                    ScaleValue(200, g_dpi), ScaleValue(70, g_dpi),
                                    ScaleValue(100, g_dpi), ScaleValue(28, g_dpi),
                                    hwnd, NULL, g_hInstance, NULL);

        // 开始按钮
        g_hStartBtn = CreateWindowW(L"BUTTON", L"▶ 开始定时",
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    ScaleValue(100, g_dpi), ScaleValue(120, g_dpi),
                                    ScaleValue(120, g_dpi), ScaleValue(38, g_dpi),
                                    hwnd, (HMENU)1, g_hInstance, NULL);

        // 取消按钮
        g_hCancelBtn = CreateWindowW(L"BUTTON", L"⏹ 取消操作",
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     ScaleValue(230, g_dpi), ScaleValue(120, g_dpi),
                                     ScaleValue(120, g_dpi), ScaleValue(38, g_dpi),
                                     hwnd, (HMENU)2, g_hInstance, NULL);
        EnableWindow(g_hCancelBtn, FALSE);

        // 操作类型选择
        CreateWindowW(L"BUTTON", L"操作类型",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      ScaleValue(30, g_dpi), ScaleValue(170, g_dpi),
                      ScaleValue(390, g_dpi), ScaleValue(80, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // 关机选项
        g_hShutdownRadio = CreateWindowW(L"BUTTON", L"关机",
                                         WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                                         ScaleValue(50, g_dpi), ScaleValue(190, g_dpi),
                                         ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                         hwnd, (HMENU)5, g_hInstance, NULL);

        // 重启选项
        g_hRestartRadio = CreateWindowW(L"BUTTON", L"重启",
                                        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                        ScaleValue(160, g_dpi), ScaleValue(190, g_dpi),
                                        ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                        hwnd, (HMENU)6, g_hInstance, NULL);

        // 注销选项
        g_hLogoffRadio = CreateWindowW(L"BUTTON", L"注销",
                                       WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                       ScaleValue(270, g_dpi), ScaleValue(190, g_dpi),
                                       ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                       hwnd, (HMENU)7, g_hInstance, NULL);

        // 默认选中关机
        SendMessage(g_hShutdownRadio, BM_SETCHECK, BST_CHECKED, 0);

        // 强制操作提示
        CreateWindowW(L"STATIC", L"⚠️ 操作将强制进行，不等待应用程序关闭",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      ScaleValue(30, g_dpi), ScaleValue(260, g_dpi),
                      ScaleValue(390, g_dpi), ScaleValue(40, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // 状态标签
        g_hStatusLabel = CreateWindowW(L"STATIC", L"📋 状态：等待设置定时时间",
                                       WS_CHILD | WS_VISIBLE | SS_LEFT,
                                       ScaleValue(20, g_dpi), ScaleValue(310, g_dpi),
                                       ScaleValue(410, g_dpi), ScaleValue(40, g_dpi),
                                       hwnd, NULL, g_hInstance, NULL);

        // 时间显示标签
        CreateWindowW(L"STATIC", L"⏱ 剩余时间将在此显示",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(350, g_dpi),
                      ScaleValue(410, g_dpi), ScaleValue(60, g_dpi),
                      hwnd, (HMENU)8, g_hInstance, NULL);

        // 底部信息
        CreateWindowW(L"STATIC", L"⚠️ 定时结束后将直接执行操作，请注意保存工作",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(410, g_dpi),
                      ScaleValue(410, g_dpi), ScaleValue(20, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // 设置字体
        SendMessage(g_hTitleLabel, WM_SETFONT, (WPARAM)g_hTitleFont, TRUE);
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        while (hChild)
        {
            if (hChild != g_hTitleLabel)
                SendMessage(hChild, WM_SETFONT, (WPARAM)g_hNormalFont, TRUE);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcBtn = (HDC)wParam;
        HWND hwndBtn = (HWND)lParam;

        // 检查控件是否被禁用
        if (!IsWindowEnabled(hwndBtn))
        {
            SetTextColor(hdcBtn, RGB(150, 150, 150)); // 灰色文本
        }
        else
        {
            SetTextColor(hdcBtn, COLOR_TEXT);
        }

        SetBkColor(hdcBtn, COLOR_BG);
        return (LRESULT)g_hBgBrush;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, COLOR_TEXT);
        SetBkColor(hdcEdit, COLOR_BG);
        return (LRESULT)g_hBgBrush;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == 1)
            OnStartShutdown(hwnd);
        else if (wmId == 2)
            OnCancelShutdown();
        else if (wmId == 5)
        {
            if (!g_isShutdownScheduled) // 只有在未倒计时时才允许切换
                g_shutdownType = 0;     // 关机
        }
        else if (wmId == 6)
        {
            if (!g_isShutdownScheduled) // 只有在未倒计时时才允许切换
                g_shutdownType = 1;     // 重启
        }
        else if (wmId == 7)
        {
            if (!g_isShutdownScheduled) // 只有在未倒计时时才允许切换
                g_shutdownType = 2;     // 注销
        }
        else if (wmId == ID_TRAY_SHOW)
            ShowWindow(hwnd, SW_RESTORE);
        else if (wmId == ID_TRAY_CANCEL)
        {
            OnCancelShutdown();
            wcscpy_s(g_nid.szTip, 128, L"定时关机程序 - 倒计时已终止");
            Shell_NotifyIcon(NIM_MODIFY, &g_nid);
        }
        else if (wmId == ID_TRAY_EXIT)
        {
            if (g_isShutdownScheduled)
                OnCancelShutdown();
            RemoveTrayIcon();
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam == 1 && g_isShutdownScheduled)
        {
            g_remainingSeconds--;
            UpdateTimerDisplay();

            // 检查是否需要显示3分钟提醒
            if (g_remainingSeconds == 180 && !g_threeMinuteNotified)
            {
                ShowThreeMinuteWarning(hwnd);
                g_threeMinuteNotified = true;
            }

            wchar_t tip[128];
            const wchar_t *actionText = L"关机";
            switch (g_shutdownType)
            {
            case 1:
                actionText = L"重启";
                break;
            case 2:
                actionText = L"注销";
                break;
            }

            int hours = g_remainingSeconds / 3600;
            int minutes = (g_remainingSeconds % 3600) / 60;
            int seconds = g_remainingSeconds % 60;

            if (hours > 0)
                swprintf_s(tip, L"%s剩余：%02d:%02d:%02d", actionText, hours, minutes, seconds);
            else
                swprintf_s(tip, L"%s剩余：%02d:%02d", actionText, minutes, seconds);

            wcscpy_s(g_nid.szTip, 128, tip);
            Shell_NotifyIcon(NIM_MODIFY, &g_nid);

            if (g_remainingSeconds <= 0)
            {
                KillTimer(hwnd, 1);
                if (EnableShutdownPrivilege())
                {
                    DWORD shutdownFlags = 0;
                    const wchar_t *actionText = L"关机";

                    switch (g_shutdownType)
                    {
                    case 0: // 关机
                        actionText = L"关机";
                        shutdownFlags = EWX_SHUTDOWN | EWX_FORCE | EWX_FORCEIFHUNG;
                        break;

                    case 1: // 重启
                        actionText = L"重启";
                        shutdownFlags = EWX_REBOOT | EWX_FORCE | EWX_FORCEIFHUNG;
                        break;

                    case 2: // 注销
                        actionText = L"注销";
                        shutdownFlags = EWX_LOGOFF | EWX_FORCE;
                        break;
                    }

                    wchar_t msg[128];
                    swprintf_s(msg, L"状态：正在%s...", actionText);
                    SetWindowTextW(g_hStatusLabel, msg);

                    // 给程序一点时间更新界面
                    UpdateWindow(hwnd);
                    Sleep(500);

                    if (g_shutdownType == 0 || g_shutdownType == 1) // 关机或重启
                    {
                        InitiateSystemShutdownEx(
                            NULL,                  // 所有计算机
                            NULL,                  // 不显示消息
                            0,                     // 等待时间(秒)
                            TRUE,                  // 强制关闭应用程序
                            (g_shutdownType == 1), // 是否重启
                            SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED);
                    }
                    else // 注销
                    {
                        ExitWindowsEx(shutdownFlags, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED);
                    }
                }
                else
                {
                    MessageBoxW(hwnd, L"⚠️ 操作权限获取失败！\n请以管理员身份运行程序。", L"错误", MB_ICONERROR);
                    OnCancelShutdown();
                }
            }
        }
        break;
    }

    case WM_TRAYICON:
    {
        if (lParam == WM_LBUTTONUP)
            ShowWindow(hwnd, SW_RESTORE);
        else if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            ShowTrayContextMenu(hwnd, pt);
        }
        break;
    }

    case WM_CLOSE:
    {
        // 创建三个按钮的对话框
        int result = MessageBoxW(hwnd,
                                 L"请选择操作：\n\n"
                                 L"• 是(Y) - 完全退出程序\n"
                                 L"• 否(N) - 隐藏到系统托盘\n"
                                 L"• 取消 - 返回程序\n\n"
                                 L"点击右上角X或按ESC取消关闭操作",
                                 L"关闭确认",
                                 MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON3);

        if (result == IDYES)
        {
            // 直接关闭
            if (g_isShutdownScheduled)
                OnCancelShutdown();
            RemoveTrayIcon();
            DestroyWindow(hwnd);
        }
        else if (result == IDNO)
        {
            // 最小化到托盘
            ShowWindow(hwnd, SW_HIDE);
        }
        // IDCANCEL: 取消操作（包括点击对话框右上角X）
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (g_hTitleFont)
            DeleteObject(g_hTitleFont);
        if (g_hNormalFont)
            DeleteObject(g_hNormalFont);
        if (g_hBgBrush)
            DeleteObject(g_hBgBrush);
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}