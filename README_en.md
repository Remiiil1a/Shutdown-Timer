# Shutdown Timer

## A beautiful and easy-to-use Windows shutdown, restart, and logoff timer...

---

### Note:

### *1. The program requires shutdown privileges, it is recommended to run as administrator*
### *2. Most of the code in this project is written by deepseek, it is a practice project*
### *3. Users assume all responsibility for any consequences of using this program*

---

## Features

### Core Features
- ⏰ **Timer Function**: Supports 0-1440 minutes (24 hours) timing
- 🔄 **Three Actions**: Supports shutdown, restart, and logoff
- 🔔 **Smart Reminder**: Automatically displays warning notification at 3 minutes remaining
- 📊 **Real-time Display**: Real-time countdown display in main window and tray icon
- 🗔 **System Tray**: Supports minimizing to system tray with right-click menu
- 🔒 **Silent Lock Screen**: No notifications displayed when system is locked

### User Interface
- 🎨 **Modern Design**: Clean white background with dark gray text
- 📱 **DPI Awareness**: Supports high DPI displays
- 📍 **Tray Integration**: Complete tray icon functionality
- 🔄 **Status Indicators**: Clear status prompts and countdown display

---

## Usage

### Setting a Timer Task
1. Run the program (recommended as administrator)
2. Enter minutes (0-1440) in the "Delay time" input box
   - Enter 0 for immediate execution
   - Maximum support is 24 hours (1440 minutes)
3. Select action type: Shutdown, Restart, or Logoff
4. Click the "▶ Start Timer" button

### Canceling a Timer Task
- Click the "⏹ Cancel" button in the main window
- Or select "⛔ Stop Countdown" from the system tray icon right-click menu

### System Tray Operations
- **Left Click**: Show/Hide main window
- **Right Click**: Show context menu
  - 🔲 Show Window
  - ⛔ Stop Countdown (only shown when countdown is running)
  - ❌ Exit

### Close Options
When clicking the window close button, options appear:
- **Yes(Y)** - Exit completely
- **No(N)** - Minimize to system tray
- **Cancel** - Return to application

---

## Technical Details

### Compilation Requirements
- Windows SDK (Windows 7 or higher)
- C++ compiler (supporting Windows API)
- Requires linking `comctl32.lib`

### Permission Requirements
The program requires:
- Shutdown privileges (recommended administrator run)
- System tray access permissions
- Mutex registration to prevent multiple instances

### System Requirements
- Windows 7 or higher
- Unicode support
- Common Controls 6.0 support

---

## Special Features Explanation

### 3-Minute Warning Function
- Automatically displays reminder at 3 minutes remaining
- Skips notifications when system is locked to avoid disturbance
- Supports system tray balloon notifications and message box alerts

### Operation Characteristics
- **Shutdown/Restart**: Forced mode, does not wait for applications to close
- **Logoff**: Forced logoff of current user session
- **Lock Screen Compatibility**: Continues countdown when system is locked but no notifications displayed

---

## Frequently Asked Questions

### Q: What if the program cannot shutdown/restart?
A: Try running the program as administrator. The program automatically detects administrator privileges and provides prompts.

### Q: Can multiple instances run?
A: No, the program uses a mutex to prevent multiple instances.

### Q: Can the timer exceed 24 hours?
A: No, maximum support is 24 hours (1440 minutes).

### Q: Does logoff support forced mode?
A: Yes, logoff operation uses forced mode.

### Q: How to exit the program completely?
A: You can exit via the tray menu, or select "Yes(Y)" when closing the window.

### Q: Will 3-minute reminder display when system is locked?
A: No, the program automatically detects and skips notifications when system is locked to avoid disturbance.

---

## Development Notes

### Code Structure
- Developed using pure Win32 API, no external dependencies
- Unicode and multilingual support
- Complete error handling and user experience optimization

---

[Back to Main Page]() | [View Chinese Version](README_zh.md)
