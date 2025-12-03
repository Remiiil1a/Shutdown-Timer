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
- ⏰ **Timer Function**: Supports 1-1440 minutes (24 hours) timing
- 🔄 **Multiple Actions**: Supports shutdown, restart, and logoff
- ⚙️ **Action Modes**: Supports forced shutdown (no app data saving) and normal shutdown
- 📊 **Real-time Display**: Real-time countdown display in main window and tray icon
- 🗔 **System Tray**: Supports minimizing to system tray with right-click menu

### User Interface
- 🎨 **Modern Design**: Clean white background with dark gray text
- 📱 **DPI Awareness**: Supports high DPI displays
- 📍 **Tray Integration**: Complete tray icon functionality
- 🔄 **Status Indicators**: Clear status prompts and countdown display

---

## Usage

### Setting a Timer Task
1. Run the program (recommended as administrator)
2. Enter minutes (1-1440) in the "Delay time" input box
3. Select action type: Shutdown, Restart, or Logoff
4. Select action mode: Forced or Normal (only for shutdown and restart)
5. Click the "▶ Start Timer" button

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
- Windows SDK
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

## Frequently Asked Questions

### Q: What if the program cannot shutdown/restart?
A: Try running the program as administrator.

### Q: Can multiple instances run?
A: No, the program uses a mutex to prevent multiple instances.

### Q: Can the timer exceed 24 hours?
A: No, maximum support is 24 hours (1440 minutes).

### Q: Does logoff support forced mode?
A: No, forced mode only works for shutdown and restart.

### Q: How to exit the program completely?
A: You can exit via the tray menu, or select "Yes(Y)" when closing the window.

---

[Back to Main Page]() | [View Chinese Version](README_zh.md)
