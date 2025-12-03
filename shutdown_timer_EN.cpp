#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h> // Required for common controls
#include <string>
#include <cstdio>
#define IDI_MAIN_ICON 101

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Color definitions
#define COLOR_BG 0xFFFFFF   // White background
#define COLOR_TEXT 0x323130 // Text color - dark gray

// Tray messages
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002
#define ID_TRAY_CANCEL 1003 // Stop countdown menu ID

// Global variables
HWND g_hTimeEdit, g_hStartBtn, g_hCancelBtn, g_hStatusLabel, g_hTitleLabel;
HFONT g_hTitleFont, g_hNormalFont;
int g_remainingSeconds = 0;
bool g_isShutdownScheduled = false;
int g_dpi = 96;
HBRUSH g_hBgBrush = NULL;
bool g_forceShutdown = true; // Default to forced shutdown
int g_shutdownType = 0;      // 0:Shutdown, 1:Restart, 2:Logoff
HWND g_hForceRadio, g_hNormalRadio;
NOTIFYICONDATA g_nid = {0};
HINSTANCE g_hInstance;

// Function declarations
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

// Create custom font
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

// Display error message
void ShowError(const wchar_t *message)
{
    SetProcessDPIAware();
    MessageBoxW(NULL, message, L"Error", MB_ICONERROR | MB_OK);
}

// DPI scaling
int ScaleValue(int value, int dpi)
{
    return MulDiv(value, dpi, 96);
}

// Check if another instance is already running
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

// Create system tray icon
void CreateTrayIcon(HWND hwnd)
{
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    // Use LoadIconW to load icon
    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!g_nid.hIcon)
    {
        // Use default icon if loading fails
        g_nid.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512)); // 32512 = IDI_APPLICATION
    }

    // Use wcscpy_s for wide string copy
    wcscpy_s(g_nid.szTip, 128, L"Shutdown Timer");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// Remove system tray icon
void RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

// Show tray context menu
void ShowTrayContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"🔲 Show Window");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

    // Add "Stop Countdown" option if countdown is active
    if (g_isShutdownScheduled)
    {
        AppendMenu(hMenu, MF_STRING, ID_TRAY_CANCEL, L"⛔ Stop Countdown");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }

    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"❌ Exit");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

// Enable shutdown privilege
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

// Start shutdown timer
void OnStartShutdown(HWND hwnd)
{
    if (g_isShutdownScheduled)
    {
        MessageBoxW(hwnd, L"⚠️ A shutdown task is already scheduled!", L"Warning", MB_ICONWARNING);
        return;
    }

    wchar_t buffer[32];
    GetWindowTextW(g_hTimeEdit, buffer, 32);
    int minutes = _wtoi(buffer);

    if (minutes <= 0 || minutes > 1440)
    {
        MessageBoxW(hwnd, L"⚠️ Please enter a valid number of minutes (1-1440)!\n(Maximum 24 hours)", L"Error", MB_ICONERROR);
        return;
    }

    if (!EnableShutdownPrivilege())
    {
        MessageBoxW(hwnd, L"⚠️ Failed to acquire shutdown privilege!\nPlease run as administrator.", L"Error", MB_ICONERROR);
        return;
    }

    g_remainingSeconds = minutes * 60;
    g_isShutdownScheduled = true;
    SetTimer(hwnd, 1, 1000, NULL);

    EnableWindow(g_hStartBtn, FALSE);
    EnableWindow(g_hCancelBtn, TRUE);

    const wchar_t *actionText = L"shutdown";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"restart";
        break;
    case 2:
        actionText = L"logoff";
        break;
    }

    wchar_t status[128];
    swprintf_s(status, L"🕓 Status: %s scheduled in %d minutes", actionText, minutes);
    SetWindowTextW(g_hStatusLabel, status);
    UpdateTimerDisplay();
}

// Cancel shutdown
void OnCancelShutdown()
{
    if (!g_isShutdownScheduled)
        return;

    HWND hwnd = GetParent(g_hStartBtn);
    KillTimer(hwnd, 1);
    g_isShutdownScheduled = false;

    EnableWindow(g_hStartBtn, TRUE);
    EnableWindow(g_hCancelBtn, FALSE);

    const wchar_t *actionText = L"shutdown";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"restart";
        break;
    case 2:
        actionText = L"logoff";
        break;
    }

    wchar_t status[128];
    swprintf_s(status, L"📋 Status: %s cancelled", actionText);
    SetWindowTextW(g_hStatusLabel, status);

    HWND hDisplay = GetDlgItem(hwnd, 8);
    SetWindowTextW(hDisplay, L"⏱ Timer cancelled");

    wcscpy_s(g_nid.szTip, L"Shutdown Timer - Timer cancelled");
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

// Update timer display
void UpdateTimerDisplay()
{
    if (!g_isShutdownScheduled)
        return;

    const wchar_t *actionText = L"shutdown";
    switch (g_shutdownType)
    {
    case 1:
        actionText = L"restart";
        break;
    case 2:
        actionText = L"logoff";
        break;
    }

    int hours = g_remainingSeconds / 3600;
    int minutes = (g_remainingSeconds % 3600) / 60;
    int seconds = g_remainingSeconds % 60;

    wchar_t display[256];
    if (hours > 0)
        swprintf_s(display, L"⏱ %s in: %02d hours %02d minutes %02d seconds",
                   actionText, hours, minutes, seconds);
    else
        swprintf_s(display, L"⏱ %s in: %02d minutes %02d seconds",
                   actionText, minutes, seconds);

    HWND hDisplay = GetDlgItem(GetParent(g_hStartBtn), 8);
    SetWindowTextW(hDisplay, display);
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    if (IsInstanceRunning())
    {
        ShowError(L"Application is already running! If not visible, check system tray.");
        return 0;
    }

    g_hInstance = hInstance;
    SetProcessDPIAware();

    // Initialize common controls - this is required
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&icex))
    {
        ShowError(L"Failed to initialize common controls");
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
        ShowError(L"Failed to register window class");
        return 1;
    }

    HDC hdc = GetDC(NULL);
    g_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    int windowWidth = ScaleValue(500, g_dpi); // Slightly wider for English text
    int windowHeight = ScaleValue(520, g_dpi);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowW(
        L"ModernShutdownTimer", L"Shutdown Timer",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        (screenWidth - windowWidth) / 2,
        (screenHeight - windowHeight) / 2,
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        ShowError(L"Failed to create window");
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

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_hTitleFont = CreateCustomFont(16, L"Microsoft YaHei UI", FW_SEMIBOLD);
        g_hNormalFont = CreateCustomFont(10, L"Segoe UI", FW_NORMAL);

        // Title
        g_hTitleLabel = CreateWindowW(L"STATIC", L"🕓 Shutdown Timer",
                                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      ScaleValue(20, g_dpi), ScaleValue(20, g_dpi),
                                      ScaleValue(460, g_dpi), ScaleValue(30, g_dpi),
                                      hwnd, NULL, g_hInstance, NULL);

        // Time input label
        CreateWindowW(L"STATIC", L"Delay time (minutes):",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      ScaleValue(30, g_dpi), ScaleValue(70, g_dpi),
                      ScaleValue(150, g_dpi), ScaleValue(25, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // Time input box
        g_hTimeEdit = CreateWindowW(L"EDIT", L"60",
                                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
                                    ScaleValue(200, g_dpi), ScaleValue(70, g_dpi),
                                    ScaleValue(100, g_dpi), ScaleValue(28, g_dpi),
                                    hwnd, NULL, g_hInstance, NULL);

        // Start button
        g_hStartBtn = CreateWindowW(L"BUTTON", L"▶ Start Timer",
                                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    ScaleValue(100, g_dpi), ScaleValue(120, g_dpi),
                                    ScaleValue(120, g_dpi), ScaleValue(38, g_dpi),
                                    hwnd, (HMENU)1, g_hInstance, NULL);

        // Cancel button
        g_hCancelBtn = CreateWindowW(L"BUTTON", L"⏹ Cancel",
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     ScaleValue(230, g_dpi), ScaleValue(120, g_dpi),
                                     ScaleValue(120, g_dpi), ScaleValue(38, g_dpi),
                                     hwnd, (HMENU)2, g_hInstance, NULL);
        EnableWindow(g_hCancelBtn, FALSE);

        // Action type selection
        CreateWindowW(L"BUTTON", L"Action Type",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      ScaleValue(30, g_dpi), ScaleValue(170, g_dpi),
                      ScaleValue(440, g_dpi), ScaleValue(80, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // Shutdown option
        HWND hShutdownRadio = CreateWindowW(L"BUTTON", L"Shutdown",
                                            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                                            ScaleValue(50, g_dpi), ScaleValue(190, g_dpi),
                                            ScaleValue(120, g_dpi), ScaleValue(25, g_dpi),
                                            hwnd, (HMENU)5, g_hInstance, NULL);

        // Restart option
        HWND hRestartRadio = CreateWindowW(L"BUTTON", L"Restart",
                                           WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                           ScaleValue(180, g_dpi), ScaleValue(190, g_dpi),
                                           ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                           hwnd, (HMENU)6, g_hInstance, NULL);

        // Logoff option
        HWND hLogoffRadio = CreateWindowW(L"BUTTON", L"Log off",
                                          WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                          ScaleValue(290, g_dpi), ScaleValue(190, g_dpi),
                                          ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                          hwnd, (HMENU)7, g_hInstance, NULL);

        // Default select shutdown
        SendMessage(hShutdownRadio, BM_SETCHECK, BST_CHECKED, 0);

        // Action mode
        CreateWindowW(L"BUTTON", L"Action Mode (for shutdown/restart only)",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      ScaleValue(30, g_dpi), ScaleValue(260, g_dpi),
                      ScaleValue(440, g_dpi), ScaleValue(80, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        g_hForceRadio = CreateWindowW(L"BUTTON", L"Forced (don't save application data)",
                                      WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                                      ScaleValue(50, g_dpi), ScaleValue(280, g_dpi),
                                      ScaleValue(250, g_dpi), ScaleValue(25, g_dpi),
                                      hwnd, (HMENU)3, g_hInstance, NULL);

        g_hNormalRadio = CreateWindowW(L"BUTTON", L"Normal (wait for applications to close)",
                                       WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                       ScaleValue(50, g_dpi), ScaleValue(305, g_dpi),
                                       ScaleValue(250, g_dpi), ScaleValue(25, g_dpi),
                                       hwnd, (HMENU)4, g_hInstance, NULL);
        SendMessage(g_hForceRadio, BM_SETCHECK, BST_CHECKED, 0);

        // Status label
        g_hStatusLabel = CreateWindowW(L"STATIC", L"📋 Status: Waiting for timer setup",
                                       WS_CHILD | WS_VISIBLE | SS_LEFT,
                                       ScaleValue(20, g_dpi), ScaleValue(350, g_dpi),
                                       ScaleValue(460, g_dpi), ScaleValue(40, g_dpi),
                                       hwnd, NULL, g_hInstance, NULL);

        // Time display label
        CreateWindowW(L"STATIC", L"⏱ Remaining time will be displayed here",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(390, g_dpi),
                      ScaleValue(460, g_dpi), ScaleValue(80, g_dpi),
                      hwnd, (HMENU)8, g_hInstance, NULL);

        // Bottom information
        CreateWindowW(L"STATIC", L"⚠️ System will shutdown/restart/logoff directly after timer ends. Save your work!",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(470, g_dpi),
                      ScaleValue(460, g_dpi), ScaleValue(30, g_dpi), // Increased height for text
                      hwnd, NULL, g_hInstance, NULL);

        // Set fonts
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
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        SetTextColor(hdcStatic, COLOR_TEXT);
        SetBkColor(hdcStatic, COLOR_BG);
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
        else if (wmId == 3)
            g_forceShutdown = true;
        else if (wmId == 4)
            g_forceShutdown = false;
        else if (wmId == 5)
            g_shutdownType = 0; // Shutdown
        else if (wmId == 6)
            g_shutdownType = 1; // Restart
        else if (wmId == 7)
            g_shutdownType = 2; // Logoff
        else if (wmId == ID_TRAY_SHOW)
            ShowWindow(hwnd, SW_RESTORE);
        else if (wmId == ID_TRAY_CANCEL)
        {
            OnCancelShutdown();
            wcscpy_s(g_nid.szTip, 128, L"Shutdown Timer - Countdown stopped");
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

            wchar_t tip[128];
            const wchar_t *actionText = L"shutdown";
            switch (g_shutdownType)
            {
            case 1:
                actionText = L"restart";
                break;
            case 2:
                actionText = L"logoff";
                break;
            }

            int hours = g_remainingSeconds / 3600;
            int minutes = (g_remainingSeconds % 3600) / 60;
            int seconds = g_remainingSeconds % 60;

            if (hours > 0)
                swprintf_s(tip, L"%s in: %02d:%02d:%02d", actionText, hours, minutes, seconds);
            else
                swprintf_s(tip, L"%s in: %02d:%02d", actionText, minutes, seconds);

            wcscpy_s(g_nid.szTip, 128, tip);
            Shell_NotifyIcon(NIM_MODIFY, &g_nid);

            if (g_remainingSeconds <= 0)
            {
                KillTimer(hwnd, 1);
                if (EnableShutdownPrivilege())
                {
                    const wchar_t *actionText = L"shutdown";
                    DWORD shutdownFlags = EWX_SHUTDOWN;

                    switch (g_shutdownType)
                    {
                    case 1: // Restart
                        actionText = L"restart";
                        shutdownFlags = EWX_REBOOT;
                        break;
                    case 2: // Logoff
                        actionText = L"logoff";
                        shutdownFlags = EWX_LOGOFF;
                        break;
                    default: // Shutdown
                        actionText = L"shutdown";
                        shutdownFlags = EWX_SHUTDOWN;
                        break;
                    }

                    // Only apply force option for shutdown or restart
                    if (g_forceShutdown && (g_shutdownType == 0 || g_shutdownType == 1))
                        shutdownFlags |= EWX_FORCE;

                    wchar_t msg[128];
                    swprintf_s(msg, L"Status: %s in progress...", actionText);
                    SetWindowTextW(g_hStatusLabel, msg);

                    ExitWindowsEx(shutdownFlags, SHTDN_REASON_FLAG_PLANNED);
                }
                else
                {
                    MessageBoxW(hwnd, L"⚠️ Failed to acquire shutdown privilege!\nPlease run as administrator.", L"Error", MB_ICONERROR);
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
        // Create dialog with three options
        int result = MessageBoxW(hwnd,
                                 L"Please select an action:\n\n"
                                 L"• Yes(Y) - Exit completely\n"
                                 L"• No(N) - Minimize to system tray\n"
                                 L"• Cancel - Return to application\n\n"
                                 L"Click the X button or press ESC to cancel",
                                 L"Close Confirmation",
                                 MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON3);

        if (result == IDYES)
        {
            // Exit completely
            if (g_isShutdownScheduled)
                OnCancelShutdown();
            RemoveTrayIcon();
            DestroyWindow(hwnd);
        }
        else if (result == IDNO)
        {
            // Minimize to tray
            ShowWindow(hwnd, SW_HIDE);
        }
        // IDCANCEL: Cancel operation (including clicking X button)
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