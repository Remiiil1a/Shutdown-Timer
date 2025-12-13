#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7 and above
#endif

#include <windows.h>
#include <windowsx.h> // Add windowsx.h for GET_X_LPARAM and GET_Y_LPARAM
#include <commctrl.h>
#include <string>
#include <cstdio>
#include <VersionHelpers.h>
#define IDI_MAIN_ICON 101

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Color definitions
#define COLOR_BG 0xFFFFFF     // White background
#define COLOR_TEXT 0x323130   // Text color - dark gray

// Tray messages
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002
#define ID_TRAY_CANCEL 1003       // Stop countdown menu ID
#define ID_TRAY_CANCEL_WATCH 1004 // Cancel watch menu ID

// New control IDs
#define ID_BTN_SELECT_WINDOW 9
#define ID_BTN_CLEAR_WINDOW 10
#define ID_STATIC_WINDOW_INFO 11

// Global variables
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

// New window monitoring related variables
HWND g_hWatchedWindow = NULL;
wchar_t g_szWatchedWindowTitle[256] = L"";
UINT_PTR g_watchTimerId = 0;
bool g_isWatchingWindow = false;
bool g_windowClosedNotified = false; // New: prevent duplicate notification flag

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
void CheckAndWarnAdminPrivilege();
bool IsSystemLocked();
void ShowThreeMinuteWarning(HWND hwnd);

// New window monitoring related function declarations
void StartWindowSelection(HWND hwnd);
void StopWindowSelection();
void OnWindowSelected(HWND hwnd);
void StartWatchingWindow(HWND hwnd);
void StopWatchingWindow();
void CheckWatchedWindow();
void UpdateWindowInfoDisplay();
void UpdateTrayTip(); // Update tray tip text
void ShowModernNotification(HWND hwnd, const wchar_t *title, const wchar_t *message);

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
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    g_nid.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!g_nid.hIcon)
    {
        g_nid.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
    }

    UpdateTrayTip(); // Use function to update tray tip
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

// Update tray tip text
void UpdateTrayTip()
{
    if (g_isWatchingWindow && g_hWatchedWindow)
    {
        wchar_t tip[256];
        if (g_szWatchedWindowTitle[0])
        {
            // Check if window still exists
            if (IsWindow(g_hWatchedWindow))
                swprintf_s(tip, L"Shutdown Timer - Monitoring: %s", g_szWatchedWindowTitle);
            else
                wcscpy_s(tip, L"Shutdown Timer - Window closed");
        }
        else
        {
            wcscpy_s(tip, L"Shutdown Timer - Monitoring");
        }
        wcscpy_s(g_nid.szTip, tip);
    }
    else if (g_isShutdownScheduled)
    {
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

        wchar_t tip[256];
        int hours = g_remainingSeconds / 3600;
        int minutes = (g_remainingSeconds % 3600) / 60;
        int seconds = g_remainingSeconds % 60;

        if (hours > 0)
            swprintf_s(tip, L"Shutdown Timer - %s countdown: %02d:%02d:%02d", actionText, hours, minutes, seconds);
        else
            swprintf_s(tip, L"Shutdown Timer - %s countdown: %02d:%02d", actionText, minutes, seconds);

        wcscpy_s(g_nid.szTip, tip);
    }
    else
    {
        wcscpy_s(g_nid.szTip, L"Shutdown Timer");
    }
}

// Remove system tray icon function
void RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

// Show tray context menu
void ShowTrayContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();

    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"🔲 Show Window");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

    // Add "Cancel Window Monitoring" option if monitoring
    if (g_isWatchingWindow)
    {
        AppendMenu(hMenu, MF_STRING, ID_TRAY_CANCEL_WATCH, L"🚫 Cancel Window Monitoring");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    }

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

// Check if running as administrator and show warning
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
                    L"⚠️ Program is not running as administrator, some features may be limited.\n"
                    L"Recommended to restart the program as administrator for full functionality.",
                    L"Privilege Warning", MB_ICONWARNING | MB_OK);
    }
}

// Check if system is locked
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

// Show modern notification (for Windows 10/11)
void ShowModernNotification(HWND hwnd, const wchar_t *title, const wchar_t *message)
{
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO | NIF_SHOWTIP;
    nid.dwInfoFlags = NIIF_WARNING | NIIF_LARGE_ICON;
    nid.uTimeout = 10000; // 10 seconds
    nid.uVersion = NOTIFYICON_VERSION_4;

    wcscpy_s(nid.szInfoTitle, title);
    wcscpy_s(nid.szInfo, message);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// Show 3-minute warning
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

    wchar_t message[256];
    swprintf_s(message, L"⚠️ %s in 3 minutes! Please prepare.", actionText);

    DEBUG_PRINT(L"[ShutdownTimer] Message prepared: ");
    DEBUG_PRINT(message);
    DEBUG_PRINT(L"\n");

    // Use modern notification method
    ShowModernNotification(hwnd, L"Shutdown Timer - Reminder", message);
}

// ==================== Window Monitoring Related Functions ====================

// Start window selection mode
void StartWindowSelection(HWND hwnd)
{
    // Check if countdown is already running (Requirement 2)
    if (g_isShutdownScheduled)
    {
        MessageBoxW(hwnd, L"⚠️ Countdown is already running, cannot start window monitoring!\n\nPlease cancel the countdown first.", L"Error", MB_ICONERROR);
        return;
    }

    // Set capture to wait for user to click a window
    SetCapture(hwnd);

    // Change cursor to crosshair
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_CROSS));

    // Update status
    SetWindowTextW(g_hStatusLabel, L"🔍 Status: Click the window to monitor...");
    SetWindowTextW(g_hWindowInfoLabel, L"Click any window to select");

    // Disable clear button
    EnableWindow(g_hClearWindowBtn, FALSE);
}

// Stop window selection mode
void StopWindowSelection()
{
    ReleaseCapture();
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// When user selects a window
void OnWindowSelected(HWND hwndSelected)
{
    if (!hwndSelected || hwndSelected == GetParent(g_hSelectWindowBtn))
        return;

    // Check if window is valid
    if (!IsWindow(hwndSelected))
    {
        MessageBoxW(GetParent(g_hSelectWindowBtn),
                    L"⚠️ Selected window is invalid!",
                    L"Window Selection Error",
                    MB_ICONWARNING | MB_OK);
        return;
    }

    // Get window title
    wchar_t title[256];
    GetWindowTextW(hwndSelected, title, 256);

    if (wcslen(title) == 0)
    {
        wcscpy_s(title, L"Untitled Window");
    }

    // Save window information
    g_hWatchedWindow = hwndSelected;
    wcscpy_s(g_szWatchedWindowTitle, title);

    // Reset notification flag
    g_windowClosedNotified = false;

    // Start monitoring window
    StartWatchingWindow(GetParent(g_hSelectWindowBtn));
}

// Start monitoring window
void StartWatchingWindow(HWND hwnd)
{
    if (g_hWatchedWindow == NULL)
        return;

    // Start timer to check window status (check every second)
    g_watchTimerId = SetTimer(hwnd, 3, 1000, NULL);
    g_isWatchingWindow = true;

    // Update display
    UpdateWindowInfoDisplay();

    // Enable clear button
    EnableWindow(g_hClearWindowBtn, TRUE);

    // Disable manual countdown related controls (Requirement 1)
    EnableWindow(g_hStartBtn, FALSE);
    EnableWindow(g_hTimeEdit, FALSE);
    EnableWindow(g_hShutdownRadio, FALSE);   // +++ New: Disable action type selection +++
    EnableWindow(g_hRestartRadio, FALSE);    // +++ New: Disable action type selection +++
    EnableWindow(g_hLogoffRadio, FALSE);     // +++ New: Disable action type selection +++
    EnableWindow(g_hSelectWindowBtn, FALSE); // Disable select window button to prevent reselection

    // Update status
    SetWindowTextW(g_hStatusLabel, L"👁 Status: Monitoring window...");

    // Update tray tip
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// Stop monitoring window
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
    g_windowClosedNotified = false; // Reset notification flag

    // Update display
    UpdateWindowInfoDisplay();

    // Disable clear button
    EnableWindow(g_hClearWindowBtn, FALSE);

    // Enable manual countdown related controls (Requirement 1)
    EnableWindow(g_hStartBtn, TRUE);
    EnableWindow(g_hTimeEdit, TRUE);

    // +++ Modified: Only enable action type selection if no countdown is active +++
    if (!g_isShutdownScheduled)
    {
        EnableWindow(g_hShutdownRadio, TRUE);
        EnableWindow(g_hRestartRadio, TRUE);
        EnableWindow(g_hLogoffRadio, TRUE);
    }

    EnableWindow(g_hSelectWindowBtn, TRUE); // Enable select window button

    // Update status
    SetWindowTextW(g_hStatusLabel, L"📋 Status: Waiting for timer setup");

    // Update tray tip
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// Check monitored window
void CheckWatchedWindow()
{
    if (!g_hWatchedWindow || !g_isWatchingWindow)
        return;

    // Check if window exists
    if (!IsWindow(g_hWatchedWindow))
    {
        // Window closed, start 3-minute countdown
        HWND hwnd = GetParent(g_hSelectWindowBtn);

        // If countdown is already running, don't start again
        if (g_isShutdownScheduled)
        {
            StopWatchingWindow();
            return;
        }

        // Prevent duplicate notification (Fix bug 3)
        if (g_windowClosedNotified)
            return;

        g_windowClosedNotified = true; // Set notification flag

        // Show window closed prompt (Requirement 5)
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

        wchar_t message[512];
        swprintf_s(message,
                   L"⚠️ Monitored window has closed!\n\n"
                   L"3-minute countdown started, %s will be executed after countdown ends.\n\n"
                   L"To cancel, click the [Cancel] button or cancel from the tray menu.",
                   actionText);

        // Use modern notification (Fix bug 4)
        ShowModernNotification(hwnd, L"Window Monitoring Alert", message);

        // Set 3-minute countdown
        g_remainingSeconds = 180; // 3 minutes
        g_isShutdownScheduled = true;

        // Start countdown timer
        SetTimer(hwnd, 1, 1000, NULL);

        // Update button status (cancel button enabled)
        EnableWindow(g_hCancelBtn, TRUE);

        // +++ New: Disable action type selection +++
        EnableWindow(g_hShutdownRadio, FALSE);
        EnableWindow(g_hRestartRadio, FALSE);
        EnableWindow(g_hLogoffRadio, FALSE);

        // Reset 3-minute reminder flag
        g_threeMinuteNotified = false;

        // Update status display
        wchar_t status[256];
        swprintf_s(status, L"🪟 Status: Monitored window closed, %s countdown started", actionText);
        SetWindowTextW(g_hStatusLabel, status);

        UpdateTimerDisplay();

        // Stop window monitoring
        StopWatchingWindow();
    }
    else
    {
        // Window still exists, update tray tip
        UpdateTrayTip();
        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
    }
}

// Update window information display
void UpdateWindowInfoDisplay()
{
    if (g_hWatchedWindow && g_isWatchingWindow)
    {
        wchar_t info[512];
        // Check if window still exists
        if (IsWindow(g_hWatchedWindow) && g_szWatchedWindowTitle[0])
        {
            swprintf_s(info, L"🔍 Monitoring: %s", g_szWatchedWindowTitle);
        }
        else if (g_szWatchedWindowTitle[0])
        {
            swprintf_s(info, L"🔍 Monitoring: %s (window closed)", g_szWatchedWindowTitle);
        }
        else
        {
            wcscpy_s(info, L"🔍 Monitoring window");
        }
        SetWindowTextW(g_hWindowInfoLabel, info);
    }
    else
    {
        SetWindowTextW(g_hWindowInfoLabel, L"📁 No window selected for monitoring");
    }
}

// ==================== Original Function Functions ====================

// Start shutdown timer
void OnStartShutdown(HWND hwnd)
{
    if (g_isShutdownScheduled)
    {
        MessageBoxW(hwnd, L"⚠️ A scheduled task is already running!", L"Warning", MB_ICONWARNING);
        return;
    }

    // Check if window monitoring is active (Requirement 1)
    if (g_isWatchingWindow)
    {
        MessageBoxW(hwnd, L"⚠️ Window monitoring is active, cannot manually set countdown!\n\nPlease cancel window monitoring first.", L"Error", MB_ICONERROR);
        return;
    }

    wchar_t buffer[32];
    GetWindowTextW(g_hTimeEdit, buffer, 32);
    int minutes = _wtoi(buffer);

    if (minutes < 0 || minutes > 1440)
    {
        MessageBoxW(hwnd, L"⚠️ Please enter a valid number of minutes (0-1440)!\n(Maximum 24 hours)", L"Error", MB_ICONERROR);
        return;
    }

    // Reset 3-minute reminder flag
    g_threeMinuteNotified = false;

    g_remainingSeconds = minutes * 60;
    g_isShutdownScheduled = true;
    SetTimer(hwnd, 1, 1000, NULL);

    // Disable all related controls
    EnableWindow(g_hStartBtn, FALSE);
    EnableWindow(g_hCancelBtn, TRUE);
    EnableWindow(g_hTimeEdit, FALSE);

    // Disable action type selection
    EnableWindow(g_hShutdownRadio, FALSE);
    EnableWindow(g_hRestartRadio, FALSE);
    EnableWindow(g_hLogoffRadio, FALSE);

    // Disable window monitoring related controls (Requirement 2)
    EnableWindow(g_hSelectWindowBtn, FALSE);
    EnableWindow(g_hClearWindowBtn, FALSE);

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

    // Update tray tip
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// Cancel shutdown
void OnCancelShutdown()
{
    if (!g_isShutdownScheduled)
        return;

    HWND hwnd = GetParent(g_hStartBtn);
    KillTimer(hwnd, 1);
    g_isShutdownScheduled = false;
    g_threeMinuteNotified = false;

    // Re-enable all related controls
    EnableWindow(g_hStartBtn, TRUE);
    EnableWindow(g_hCancelBtn, FALSE);
    EnableWindow(g_hTimeEdit, TRUE);

    // +++ Modified: Re-enable action type selection +++
    // Only enable action type selection if no window monitoring is active
    if (!g_isWatchingWindow)
    {
        EnableWindow(g_hShutdownRadio, TRUE);
        EnableWindow(g_hRestartRadio, TRUE);
        EnableWindow(g_hLogoffRadio, TRUE);
    }

    // Re-enable window monitoring related controls, but depends on current state
    EnableWindow(g_hSelectWindowBtn, TRUE);
    if (g_isWatchingWindow)
        EnableWindow(g_hClearWindowBtn, TRUE);
    else
        EnableWindow(g_hClearWindowBtn, FALSE);

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
    swprintf_s(status, L"📋 Status: %s timer cancelled", actionText);
    SetWindowTextW(g_hStatusLabel, status);

    HWND hDisplay = GetDlgItem(hwnd, 8);
    SetWindowTextW(hDisplay, L"⏱ Timer cancelled");

    // Update tray tip
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// Update status display
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
        swprintf_s(display, L"⏱ %s remaining: %02d hours %02d minutes %02d seconds",
                   actionText, hours, minutes, seconds);
    else
        swprintf_s(display, L"⏱ %s remaining: %02d minutes %02d seconds",
                   actionText, minutes, seconds);

    HWND hDisplay = GetDlgItem(GetParent(g_hStartBtn), 8);
    SetWindowTextW(hDisplay, display);

    // Update tray tip
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    if (IsInstanceRunning())
    {
        ShowError(L"Program is already running! If not visible, check system tray.");
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
        wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
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

    int windowWidth = ScaleValue(500, g_dpi);  // Slightly wider for English text
    int windowHeight = ScaleValue(530, g_dpi); // Adjusted height
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
        g_hShutdownRadio = CreateWindowW(L"BUTTON", L"Shutdown",
                                         WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                                         ScaleValue(50, g_dpi), ScaleValue(190, g_dpi),
                                         ScaleValue(120, g_dpi), ScaleValue(25, g_dpi),
                                         hwnd, (HMENU)5, g_hInstance, NULL);

        // Restart option
        g_hRestartRadio = CreateWindowW(L"BUTTON", L"Restart",
                                        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                        ScaleValue(180, g_dpi), ScaleValue(190, g_dpi),
                                        ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                        hwnd, (HMENU)6, g_hInstance, NULL);

        // Logoff option
        g_hLogoffRadio = CreateWindowW(L"BUTTON", L"Log off",
                                       WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                       ScaleValue(290, g_dpi), ScaleValue(190, g_dpi),
                                       ScaleValue(100, g_dpi), ScaleValue(25, g_dpi),
                                       hwnd, (HMENU)7, g_hInstance, NULL);

        // Default select shutdown
        SendMessage(g_hShutdownRadio, BM_SETCHECK, BST_CHECKED, 0);

        // ============ New Window Monitoring Function Area ============
        // Window monitoring group box
        CreateWindowW(L"BUTTON", L"Window Monitoring (Sometimes may not work, please test first)",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      ScaleValue(30, g_dpi), ScaleValue(260, g_dpi),
                      ScaleValue(440, g_dpi), ScaleValue(100, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // Select window button
        g_hSelectWindowBtn = CreateWindowW(L"BUTTON", L"🔍 Select Window to Monitor",
                                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           ScaleValue(50, g_dpi), ScaleValue(285, g_dpi),
                                           ScaleValue(200, g_dpi), ScaleValue(28, g_dpi),
                                           hwnd, (HMENU)ID_BTN_SELECT_WINDOW, g_hInstance, NULL);

        // Clear window button
        g_hClearWindowBtn = CreateWindowW(L"BUTTON", L"🗑 Clear Monitoring",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          ScaleValue(260, g_dpi), ScaleValue(285, g_dpi),
                                          ScaleValue(150, g_dpi), ScaleValue(28, g_dpi),
                                          hwnd, (HMENU)ID_BTN_CLEAR_WINDOW, g_hInstance, NULL);
        EnableWindow(g_hClearWindowBtn, FALSE);

        // Window information display
        g_hWindowInfoLabel = CreateWindowW(L"STATIC", L"📁 No window selected for monitoring",
                                           WS_CHILD | WS_VISIBLE | SS_LEFT,
                                           ScaleValue(50, g_dpi), ScaleValue(320, g_dpi),
                                           ScaleValue(380, g_dpi), ScaleValue(25, g_dpi),
                                           hwnd, (HMENU)ID_STATIC_WINDOW_INFO, g_hInstance, NULL);

        // ============ Original Other Controls ============
        // Forced operation warning
        CreateWindowW(L"STATIC", L"⚠️ Operation will be forced, will not wait for applications to close",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      ScaleValue(30, g_dpi), ScaleValue(370, g_dpi),
                      ScaleValue(440, g_dpi), ScaleValue(40, g_dpi),
                      hwnd, NULL, g_hInstance, NULL);

        // Status label
        g_hStatusLabel = CreateWindowW(L"STATIC", L"📋 Status: Waiting for timer setup",
                                       WS_CHILD | WS_VISIBLE | SS_LEFT,
                                       ScaleValue(20, g_dpi), ScaleValue(420, g_dpi),
                                       ScaleValue(460, g_dpi), ScaleValue(40, g_dpi),
                                       hwnd, NULL, g_hInstance, NULL);

        // Time display label
        CreateWindowW(L"STATIC", L"⏱ Remaining time will be displayed here",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(460, g_dpi),
                      ScaleValue(460, g_dpi), ScaleValue(60, g_dpi),
                      hwnd, (HMENU)8, g_hInstance, NULL);

        // Bottom information
        CreateWindowW(L"STATIC", L"⚠️ Action will be executed directly after timer ends. Save your work!",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      ScaleValue(20, g_dpi), ScaleValue(500, g_dpi),
                      ScaleValue(460, g_dpi), ScaleValue(25, g_dpi),
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
        else if (wmId == ID_TRAY_CANCEL_WATCH) // New: Cancel monitoring from tray menu
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
        // New window monitoring function handling
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
        // If in window selection mode, handle window selection
        if (GetCapture() == hwnd)
        {
            // Get mouse position
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(hwnd, &pt);

            // Get window handle at mouse position
            HWND hwndSelected = WindowFromPoint(pt);

            // Stop selection mode
            StopWindowSelection();

            // Process selected window
            OnWindowSelected(hwndSelected);

            return 0;
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam == 1 && g_isShutdownScheduled) // Countdown timer
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
                    const wchar_t *actionText = L"shutdown";

                    switch (g_shutdownType)
                    {
                    case 0:
                        actionText = L"shutdown";
                        shutdownFlags = EWX_SHUTDOWN | EWX_FORCE | EWX_FORCEIFHUNG;
                        break;
                    case 1:
                        actionText = L"restart";
                        shutdownFlags = EWX_REBOOT | EWX_FORCE | EWX_FORCEIFHUNG;
                        break;
                    case 2:
                        actionText = L"logoff";
                        shutdownFlags = EWX_LOGOFF | EWX_FORCE;
                        break;
                    }

                    wchar_t msg[128];
                    swprintf_s(msg, L"Status: %s in progress...", actionText);
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
                    MessageBoxW(hwnd, L"⚠️ Failed to acquire operation privilege!\nPlease run as administrator.", L"Error", MB_ICONERROR);
                    OnCancelShutdown();
                }
            }
        }
        else if (wParam == 3) // Window monitoring timer
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
        // If in window selection mode, cancel selection
        if (GetCapture() == hwnd)
        {
            StopWindowSelection();
            SetWindowTextW(g_hStatusLabel, L"📋 Status: Waiting for timer setup");
            return 0;
        }

        int result = MessageBoxW(hwnd,
                                 L"Please select an action:\n\n"
                                 L"• Yes(Y) - Exit completely\n"
                                 L"• No(N) - Hide to system tray\n"
                                 L"• Cancel - Return to program\n\n"
                                 L"Click the X button or press ESC to cancel",
                                 L"Close Confirmation",
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