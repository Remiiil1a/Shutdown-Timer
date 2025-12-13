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
#include <windowsx.h> // 添加windowsx.h以使用GET_X_LPARAM和GET_Y_LPARAM
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
#define ID_TRAY_CANCEL 1003       // 终止倒计时菜单ID
#define ID_TRAY_CANCEL_WATCH 1004 // 取消监视菜单ID

// 新添加的控件ID
#define ID_BTN_SELECT_WINDOW 9
#define ID_BTN_CLEAR_WINDOW 10
#define ID_STATIC_WINDOW_INFO 11

// 全局变量
HWND g_hTimeEdit, g_hStartBtn, g_hCancelBtn, g_hStatusLabel, g_hTitleLabel;
HWND g_hShutdownRadio, g_hRestartRadio, g_hLogoffRadio;
HWND g_hSelectWindowBtn, g_hClearWindowBtn, g_hWindowInfoLabel;
HFONT g_hTitleFont, g_hNormalFont;
int g_remainingSeconds = 0;
bool g_isShutdownScheduled = false;
int g_dpi = 96;
HBRUSH g_hBgBrush = NULL;
bool g_forceShutdown = true;
int g_shutdownType = 0;
HINSTANCE g_hInstance;
bool g_threeMinuteNotified = false;
NOTIFYICONDATA g_nid = {0};

// 新增窗口监视相关变量
HWND g_hWatchedWindow = NULL;
wchar_t g_szWatchedWindowTitle[256] = L"";
UINT_PTR g_watchTimerId = 0;
bool g_isWatchingWindow = false;
bool g_windowClosedNotified = false; // 新增：防止重复通知标志

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

// 新增窗口监视相关函数声明
void StartWindowSelection(HWND hwnd);
void StopWindowSelection();
void OnWindowSelected(HWND hwnd);
void StartWatchingWindow(HWND hwnd);
void StopWatchingWindow();
void CheckWatchedWindow();
void UpdateWindowInfoDisplay();
void UpdateTrayTip(); // 更新托盘提示文本
void ShowModernNotification(HWND hwnd, const wchar_t *title, const wchar_t *message);

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

    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!g_nid.hIcon)
    {
        g_nid.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
    }

    UpdateTrayTip(); // 使用函数更新托盘提示
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// 更新托盘提示文本
void UpdateTrayTip()
{
    if (g_isWatchingWindow && g_hWatchedWindow)
    {
        wchar_t tip[256];
        if (g_szWatchedWindowTitle[0])
        {
            // 检查窗口是否还存在
            if (IsWindow(g_hWatchedWindow))
                swprintf_s(tip, L"定时关机程序 - 监视中: %s", g_szWatchedWindowTitle);
            else
                wcscpy_s(tip, L"定时关机程序 - 窗口已关闭");
        }
        else
        {
            wcscpy_s(tip, L"定时关机程序 - 监视中");
        }
        wcscpy_s(g_nid.szTip, tip);
    }
    else if (g_isShutdownScheduled)
    {
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

        wchar_t tip[256];
        int hours = g_remainingSeconds / 3600;
        int minutes = (g_remainingSeconds % 3600) / 60;
        int seconds = g_remainingSeconds % 60;

        if (hours > 0)
            swprintf_s(tip, L"定时关机程序 - %s倒计时: %02d:%02d:%02d", actionText, hours, minutes, seconds);
        else
            swprintf_s(tip, L"定时关机程序 - %s倒计时: %02d:%02d", actionText, minutes, seconds);

        wcscpy_s(g_nid.szTip, tip);
    }
    else
    {
        wcscpy_s(g_nid.szTip, L"定时关机程序");
    }
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

    // 如果正在监视窗口，添加"取消监视"选项
    if (g_isWatchingWindow)
    {
        AppendMenu(hMenu, MF_STRING, ID_TRAY_CANCEL_WATCH, L"🚫 取消窗口监视");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }

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
    HWINSTA hCurrent = GetProcessWindowStation();
    if (hCurrent)
    {
        DWORD dwFlags;
        if (GetUserObjectInformationW(hCurrent, UOI_FLAGS, &dwFlags, sizeof(dwFlags), NULL))
        {
            return !(dwFlags & WSF_VISIBLE);
        }
    }
    return false;
}

// 显示现代通知（适用于Windows 10/11）
void ShowModernNotification(HWND hwnd, const wchar_t *title, const wchar_t *message)
{
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO | NIF_SHOWTIP;
    nid.dwInfoFlags = NIIF_WARNING | NIIF_LARGE_ICON;
    nid.uTimeout = 10000; // 10秒
    nid.uVersion = NOTIFYICON_VERSION_4;

    wcscpy_s(nid.szInfoTitle, title);
    wcscpy_s(nid.szInfo, message);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// 显示3分钟警告
#ifdef _DEBUG
#define DEBUG_PRINT(msg) OutputDebugStringW(msg)
#else
#define DEBUG_PRINT(msg)
#endif

void ShowThreeMinuteWarning(HWND hwnd)
{
    DEBUG_PRINT(L"[ShutdownTimer] ShowThreeMinuteWarning called\n");

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

    // 使用现代通知方式
    ShowModernNotification(hwnd, L"定时关机程序 - 提醒", message);
}

// ==================== 窗口监视相关函数 ====================

// 开始窗口选择模式
void StartWindowSelection(HWND hwnd)
{
    // 检查是否已有倒计时在运行（需求2）
    if (g_isShutdownScheduled)
    {
        MessageBoxW(hwnd, L"⚠️ 倒计时已在运行，无法启动窗口监视！\n\n请先取消倒计时。", L"错误", MB_ICONERROR);
        return;
    }

    // 设置捕获，等待用户点击窗口
    SetCapture(hwnd);

    // 改变光标为十字准星
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_CROSS));

    // 更新状态
    SetWindowTextW(g_hStatusLabel, L"🔍 状态：请点击要监视的窗口...");
    SetWindowTextW(g_hWindowInfoLabel, L"点击任意窗口进行选择");

    // 启用清除按钮
    EnableWindow(g_hClearWindowBtn, FALSE);
}

// 停止窗口选择模式
void StopWindowSelection()
{
    ReleaseCapture();
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// 当用户选择一个窗口
void OnWindowSelected(HWND hwndSelected)
{
    if (!hwndSelected || hwndSelected == GetParent(g_hSelectWindowBtn))
        return;

    // 检查窗口是否有效
    if (!IsWindow(hwndSelected))
    {
        MessageBoxW(GetParent(g_hSelectWindowBtn),
                    L"⚠️ 选择的窗口无效！",
                    L"窗口选择错误",
                    MB_ICONWARNING | MB_OK);
        return;
    }

    // 获取窗口标题
    wchar_t title[256];
    GetWindowTextW(hwndSelected, title, 256);

    if (wcslen(title) == 0)
    {
        wcscpy_s(title, L"无标题窗口");
    }

    // 保存窗口信息
    g_hWatchedWindow = hwndSelected;
    wcscpy_s(g_szWatchedWindowTitle, title);

    // 重置通知标志
    g_windowClosedNotified = false;

    // 开始监视窗口
    StartWatchingWindow(GetParent(g_hSelectWindowBtn));
}

// 开始监视窗口
void StartWatchingWindow(HWND hwnd)
{
    if (g_hWatchedWindow == NULL)
        return;

    // 启动定时器检查窗口状态（每秒检查一次）
    g_watchTimerId = SetTimer(hwnd, 3, 1000, NULL);
    g_isWatchingWindow = true;

    // 更新显示
    UpdateWindowInfoDisplay();

    // 启用清除按钮
    EnableWindow(g_hClearWindowBtn, TRUE);

    // 禁用手动倒计时相关控件（需求1）
    EnableWindow(g_hStartBtn, FALSE);
    EnableWindow(g_hTimeEdit, FALSE);
    EnableWindow(g_hShutdownRadio, FALSE);   // +++ 新增：禁用操作类型选择 +++
    EnableWindow(g_hRestartRadio, FALSE);    // +++ 新增：禁用操作类型选择 +++
    EnableWindow(g_hLogoffRadio, FALSE);     // +++ 新增：禁用操作类型选择 +++
    EnableWindow(g_hSelectWindowBtn, FALSE); // 禁用选择窗口按钮，防止重复选择

    // 更新状态
    SetWindowTextW(g_hStatusLabel, L"👁 状态：正在监视窗口...");

    // 更新托盘提示
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 停止监视窗口
void StopWatchingWindow()
{
    if (g_watchTimerId != 0)
    {
        KillTimer(GetParent(g_hSelectWindowBtn), 3);
        g_watchTimerId = 0;
    }

    g_hWatchedWindow = NULL;
    g_szWatchedWindowTitle[0] = L'\0';
    g_isWatchingWindow = false;
    g_windowClosedNotified = false; // 重置通知标志

    // 更新显示
    UpdateWindowInfoDisplay();

    // 禁用清除按钮
    EnableWindow(g_hClearWindowBtn, FALSE);

    // 启用手动倒计时相关控件（需求1）
    EnableWindow(g_hStartBtn, TRUE);
    EnableWindow(g_hTimeEdit, TRUE);

    // +++ 修改：只有在没有倒计时的情况下才启用操作类型选择 +++
    if (!g_isShutdownScheduled)
    {
        EnableWindow(g_hShutdownRadio, TRUE);
        EnableWindow(g_hRestartRadio, TRUE);
        EnableWindow(g_hLogoffRadio, TRUE);
    }

    EnableWindow(g_hSelectWindowBtn, TRUE); // 启用选择窗口按钮

    // 更新状态
    SetWindowTextW(g_hStatusLabel, L"📋 状态：等待设置定时时间");

    // 更新托盘提示
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 检查被监视的窗口
void CheckWatchedWindow()
{
    if (!g_hWatchedWindow || !g_isWatchingWindow)
        return;

    // 检查窗口是否存在
    if (!IsWindow(g_hWatchedWindow))
    {
        // 窗口已关闭，启动3分钟倒计时
        HWND hwnd = GetParent(g_hSelectWindowBtn);

        // 如果已有倒计时在运行，不重复启动
        if (g_isShutdownScheduled)
        {
            StopWatchingWindow();
            return;
        }

        // 防止重复通知（修复bug 3）
        if (g_windowClosedNotified)
            return;

        g_windowClosedNotified = true; // 设置通知标志

        // 显示窗口关闭提示（需求5）
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

        wchar_t message[512];
        swprintf_s(message,
                   L"⚠️ 监视的窗口已关闭！\n\n"
                   L"已启动3分钟倒计时，倒计时结束后将执行%s操作。\n\n"
                   L"如需取消，请点击【取消操作】按钮或从托盘菜单取消。",
                   actionText);

        // 使用现代通知（修复bug 4）
        ShowModernNotification(hwnd, L"窗口监视提示", message);

        // 设置3分钟倒计时
        g_remainingSeconds = 180; // 3分钟
        g_isShutdownScheduled = true;

        // 启动倒计时定时器
        SetTimer(hwnd, 1, 1000, NULL);

        // 更新按钮状态（取消按钮启用）
        EnableWindow(g_hCancelBtn, TRUE);

        // +++ 新增：禁用操作类型选择 +++
        EnableWindow(g_hShutdownRadio, FALSE);
        EnableWindow(g_hRestartRadio, FALSE);
        EnableWindow(g_hLogoffRadio, FALSE);

        // 重置3分钟提醒标记
        g_threeMinuteNotified = false;

        // 更新状态显示
        wchar_t status[256];
        swprintf_s(status, L"🪟 状态：监视的窗口已关闭，%s倒计时已启动", actionText);
        SetWindowTextW(g_hStatusLabel, status);

        UpdateTimerDisplay();

        // 停止窗口监视
        StopWatchingWindow();
    }
    else
    {
        // 窗口仍然存在，更新托盘提示
        UpdateTrayTip();
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    }
}

// 更新窗口信息显示
void UpdateWindowInfoDisplay()
{
    if (g_hWatchedWindow && g_isWatchingWindow)
    {
        wchar_t info[512];
        // 检查窗口是否仍然存在
        if (IsWindow(g_hWatchedWindow) && g_szWatchedWindowTitle[0])
        {
            swprintf_s(info, L"🔍 正在监视：%s", g_szWatchedWindowTitle);
        }
        else if (g_szWatchedWindowTitle[0])
        {
            swprintf_s(info, L"🔍 正在监视：%s (窗口已关闭)", g_szWatchedWindowTitle);
        }
        else
        {
            wcscpy_s(info, L"🔍 正在监视窗口");
        }
        SetWindowTextW(g_hWindowInfoLabel, info);
    }
    else
    {
        SetWindowTextW(g_hWindowInfoLabel, L"📁 未选择监视窗口");
    }
}

// ==================== 原有功能函数 ====================

// 开始关机定时
void OnStartShutdown(HWND hwnd)
{
    if (g_isShutdownScheduled)
    {
        MessageBoxW(hwnd, L"⚠️ 已有定时任务在运行！", L"提示", MB_ICONWARNING);
        return;
    }

    // 检查是否正在监视窗口（需求1）
    if (g_isWatchingWindow)
    {
        MessageBoxW(hwnd, L"⚠️ 正在监视窗口，无法手动设置倒计时！\n\n请先取消窗口监视。", L"错误", MB_ICONERROR);
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
    EnableWindow(g_hTimeEdit, FALSE);

    // 禁用操作类型选择
    EnableWindow(g_hShutdownRadio, FALSE);
    EnableWindow(g_hRestartRadio, FALSE);
    EnableWindow(g_hLogoffRadio, FALSE);

    // 禁用窗口监视相关控件（需求2）
    EnableWindow(g_hSelectWindowBtn, FALSE);
    EnableWindow(g_hClearWindowBtn, FALSE);

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

    // 更新托盘提示
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 取消关机
void OnCancelShutdown()
{
    if (!g_isShutdownScheduled)
        return;

    HWND hwnd = GetParent(g_hStartBtn);
    KillTimer(hwnd, 1);
    g_isShutdownScheduled = false;
    g_threeMinuteNotified = false;

    // 重新启用所有相关控件
    EnableWindow(g_hStartBtn, TRUE);
    EnableWindow(g_hCancelBtn, FALSE);
    EnableWindow(g_hTimeEdit, TRUE);

    // +++ 修改：重新启用操作类型选择 +++
    // 只有在没有窗口监视的情况下才启用操作类型选择
    if (!g_isWatchingWindow)
    {
        EnableWindow(g_hShutdownRadio, TRUE);
        EnableWindow(g_hRestartRadio, TRUE);
        EnableWindow(g_hLogoffRadio, TRUE);
    }

    // 重新启用窗口监视相关控件，但需要根据当前状态决定
    EnableWindow(g_hSelectWindowBtn, TRUE);
    if (g_isWatchingWindow)
        EnableWindow(g_hClearWindowBtn, TRUE);
    else
        EnableWindow(g_hClearWindowBtn, FALSE);

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

    // 更新托盘提示
    UpdateTrayTip();
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

    // 更新托盘提示
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    if (IsInstanceRunning())
    {
        ShowError(L"程序已在运行中！如果找不到，请检查系统托盘");
        return 0;
    }

    g_hInstance = hInstance;
    SetProcessDPIAware();
    CheckAndWarnAdminPrivilege();

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
        wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
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
    int windowHeight = ScaleValue(520, g_dpi);
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

        // ============ 新增窗口监视功能区域 ============
        // 窗口监视分组框
        CreateWindowW(L"BUTTON", L"窗口监视功能（部分窗口的监视可能不起作用，请自行测试）",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      ScaleValue(30, g_dpi), ScaleValue(260, g_dpi),
                      ScaleValue(390, g_dpi), ScaleValue(100, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // 选择窗口按钮
        g_hSelectWindowBtn = CreateWindowW(L"BUTTON", L"🔍 选择要监视的窗口",
                                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           ScaleValue(50, g_dpi), ScaleValue(285, g_dpi),
                                           ScaleValue(160, g_dpi), ScaleValue(28, g_dpi),
                                           hwnd, (HMENU)ID_BTN_SELECT_WINDOW, g_hInstance, NULL);

        // 清除窗口按钮
        g_hClearWindowBtn = CreateWindowW(L"BUTTON", L"🗑 清除监视",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          ScaleValue(220, g_dpi), ScaleValue(285, g_dpi),
                                          ScaleValue(100, g_dpi), ScaleValue(28, g_dpi),
                                          hwnd, (HMENU)ID_BTN_CLEAR_WINDOW, g_hInstance, NULL);
        EnableWindow(g_hClearWindowBtn, FALSE);

        // 窗口信息显示
        g_hWindowInfoLabel = CreateWindowW(L"STATIC", L"📁 未选择监视窗口",
                                           WS_CHILD | WS_VISIBLE | SS_LEFT,
                                           ScaleValue(50, g_dpi), ScaleValue(320, g_dpi),
                                           ScaleValue(350, g_dpi), ScaleValue(25, g_dpi),
                                           hwnd, (HMENU)ID_STATIC_WINDOW_INFO, g_hInstance, NULL);

        // ============ 原有其他控件 ============
        // 强制操作提示
        CreateWindowW(L"STATIC", L"⚠️ 操作将强制进行，不等待应用程序关闭",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      ScaleValue(30, g_dpi), ScaleValue(370, g_dpi),
                      ScaleValue(390, g_dpi), ScaleValue(40, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // 状态标签
        g_hStatusLabel = CreateWindowW(L"STATIC", L"📋 状态：等待设置定时时间",
                                       WS_CHILD | WS_VISIBLE | SS_LEFT,
                                       ScaleValue(20, g_dpi), ScaleValue(420, g_dpi),
                                       ScaleValue(410, g_dpi), ScaleValue(40, g_dpi),
                                       hwnd, NULL, g_hInstance, NULL);

        // 时间显示标签
        CreateWindowW(L"STATIC", L"⏱ 剩余时间将在此显示",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(460, g_dpi),
                      ScaleValue(410, g_dpi), ScaleValue(60, g_dpi),
                      hwnd, (HMENU)8, g_hInstance, NULL);

        // 底部信息
        CreateWindowW(L"STATIC", L"⚠️ 定时结束后将直接执行操作，请注意保存工作",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(495, g_dpi),
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

        if (!IsWindowEnabled(hwndBtn))
        {
            SetTextColor(hdcBtn, RGB(150, 150, 150));
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
            if (!g_isShutdownScheduled && !g_isWatchingWindow)
                g_shutdownType = 0;
        }
        else if (wmId == 6)
        {
            if (!g_isShutdownScheduled && !g_isWatchingWindow)
                g_shutdownType = 1;
        }
        else if (wmId == 7)
        {
            if (!g_isShutdownScheduled && !g_isWatchingWindow)
                g_shutdownType = 2;
        }
        else if (wmId == ID_TRAY_SHOW)
            ShowWindow(hwnd, SW_RESTORE);
        else if (wmId == ID_TRAY_CANCEL)
        {
            OnCancelShutdown();
            UpdateTrayTip();
            Shell_NotifyIcon(NIM_MODIFY, &g_nid);
        }
        else if (wmId == ID_TRAY_CANCEL_WATCH) // 新增：从托盘菜单取消监视
        {
            StopWatchingWindow();
        }
        else if (wmId == ID_TRAY_EXIT)
        {
            if (g_isShutdownScheduled)
                OnCancelShutdown();
            if (g_isWatchingWindow)
                StopWatchingWindow();
            RemoveTrayIcon();
            DestroyWindow(hwnd);
        }
        // 新增窗口监视功能处理
        else if (wmId == ID_BTN_SELECT_WINDOW)
        {
            StartWindowSelection(hwnd);
        }
        else if (wmId == ID_BTN_CLEAR_WINDOW)
        {
            StopWatchingWindow();
        }
        break;
    }

    case WM_LBUTTONDOWN:
    {
        // 如果在窗口选择模式下，处理窗口选择
        if (GetCapture() == hwnd)
        {
            // 获取鼠标位置
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(hwnd, &pt);

            // 获取鼠标位置下的窗口句柄
            HWND hwndSelected = WindowFromPoint(pt);

            // 停止选择模式
            StopWindowSelection();

            // 处理选择的窗口
            OnWindowSelected(hwndSelected);

            return 0;
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam == 1 && g_isShutdownScheduled) // 倒计时定时器
        {
            g_remainingSeconds--;
            UpdateTimerDisplay();

            if (g_remainingSeconds == 180 && !g_threeMinuteNotified)
            {
                ShowThreeMinuteWarning(hwnd);
                g_threeMinuteNotified = true;
            }

            if (g_remainingSeconds <= 0)
            {
                KillTimer(hwnd, 1);
                if (EnableShutdownPrivilege())
                {
                    DWORD shutdownFlags = 0;
                    const wchar_t *actionText = L"关机";

                    switch (g_shutdownType)
                    {
                    case 0:
                        actionText = L"关机";
                        shutdownFlags = EWX_SHUTDOWN | EWX_FORCE | EWX_FORCEIFHUNG;
                        break;
                    case 1:
                        actionText = L"重启";
                        shutdownFlags = EWX_REBOOT | EWX_FORCE | EWX_FORCEIFHUNG;
                        break;
                    case 2:
                        actionText = L"注销";
                        shutdownFlags = EWX_LOGOFF | EWX_FORCE;
                        break;
                    }

                    wchar_t msg[128];
                    swprintf_s(msg, L"状态：正在%s...", actionText);
                    SetWindowTextW(g_hStatusLabel, msg);

                    UpdateWindow(hwnd);
                    Sleep(500);

                    if (g_shutdownType == 0 || g_shutdownType == 1)
                    {
                        InitiateSystemShutdownEx(
                            NULL,
                            NULL,
                            0,
                            TRUE,
                            (g_shutdownType == 1),
                            SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED);
                    }
                    else
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
        else if (wParam == 3) // 窗口监视定时器
        {
            CheckWatchedWindow();
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
        // 如果在窗口选择模式下，取消选择
        if (GetCapture() == hwnd)
        {
            StopWindowSelection();
            SetWindowTextW(g_hStatusLabel, L"📋 状态：等待设置定时时间");
            return 0;
        }

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
            if (g_isShutdownScheduled)
                OnCancelShutdown();
            if (g_isWatchingWindow)
                StopWatchingWindow();
            RemoveTrayIcon();
            DestroyWindow(hwnd);
        }
        else if (result == IDNO)
        {
            ShowWindow(hwnd, SW_HIDE);
        }
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (g_watchTimerId)
            KillTimer(hwnd, 3);
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