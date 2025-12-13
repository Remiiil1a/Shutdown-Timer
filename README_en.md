# Shutdown Timer

## Powerful Windows shutdown timer with new window monitoring feature...

---

### Note:

### *1. The program requires shutdown privileges, it is recommended to run as administrator*
### *2. Most of the code in this project is written by deepseek, it is a practice project*
### *3. Users assume all responsibility for any consequences of using this program*

---

## Features

### 🚀 Core Features
- ⏰ **Timer Function**: Supports 0-1440 minutes (24 hours) timing
- 🔄 **Three Actions**: Supports shutdown, restart, and logoff
- 🪟 **Window Monitoring**: New window monitoring feature, automatically starts 3-minute countdown when window closes
- 🔔 **Smart Reminder**: Automatically displays warning notification at 3 minutes remaining
- 📊 **Real-time Display**: Real-time countdown display in main window and tray icon
- 🗔 **System Tray**: Supports minimizing to system tray with right-click menu
- 🔒 **Silent Lock Screen**: No notifications displayed when system is locked

### 🎨 User Interface
- 🎨 **Modern Design**: Clean white background with dark gray text
- 📱 **DPI Awareness**: Supports high DPI displays
- 📍 **Tray Integration**: Complete tray icon functionality
- 🔄 **Status Indicators**: Clear status prompts and countdown display
- 🔍 **Intuitive Operation**: Simple window selection mechanism

---

## Usage

### Basic Timer Function
1. Run the program (recommended as administrator)
2. Enter minutes (0-1440) in the "Delay time" input box
   - Enter 0 for immediate execution
   - Maximum support is 24 hours (1440 minutes)
3. Select action type: Shutdown, Restart, or Logoff
4. Click the "▶ Start Timer" button

### 🪟 Window Monitoring Function
1. Click the "🔍 Select Window to Monitor" button
2. Mouse cursor changes to crosshair, click any window to monitor
3. Program starts monitoring the window, status bar displays monitoring info
4. When the monitored window closes, automatically starts 3-minute countdown
5. After countdown ends, executes selected action

### Canceling Timer Task
- Click the "⏹ Cancel" button in the main window
- Or select "⛔ Stop Countdown" from the system tray icon right-click menu

### Canceling Window Monitoring
- Click the "🗑 Clear Monitoring" button in the main window
- Or select "🚫 Cancel Window Monitoring" from the system tray icon right-click menu

### System Tray Operations
- **Left Click**: Show/Hide main window
- **Right Click**: Show context menu
  - 🔲 Show Window
  - 🚫 Cancel Window Monitoring (only shown when monitoring window)
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

### Window Monitoring Function
- **Automatic Trigger**: Automatically starts 3-minute countdown when monitored window closes
- **Smart Notifications**: Shows detailed prompt when window closes
- **Prevent Duplicates**: Prevents duplicate notifications and duplicate countdown starts
- **Status Feedback**: Displays monitoring status in system tray

### 3-Minute Warning Function
- Automatically displays reminder at 3 minutes remaining
- Skips notifications when system is locked to avoid disturbance
- Supports system tray balloon notifications and message box alerts

### Operation Characteristics
- **Shutdown/Restart**: Forced mode, does not wait for applications to close
- **Logoff**: Forced logoff of current user session
- **Lock Screen Compatibility**: Continues countdown when system is locked but no notifications displayed

### Mutual Exclusion Features
- **Countdown and Monitoring Exclusion**: Cannot perform window monitoring and manual countdown simultaneously
- **Smart State Management**: Automatically enables/disables related controls based on current state
- **Conflict Prevention**: Ensures program state consistency

---

## Frequently Asked Questions

### Q: What if the program cannot shutdown/restart?
A: Try running the program as administrator. The program automatically detects administrator privileges and provides prompts.

### Q: Can multiple instances run?
A: No, the program uses a mutex to prevent multiple instances.

### Q: Can I perform window monitoring and manual countdown simultaneously?
A: No, these two functions are mutually exclusive. You can only use one at a time.

### Q: Does window monitoring work for all windows?
A: Most windows are supported, but some special system windows may not work properly.

### Q: Can the countdown be canceled after window closes?
A: Yes, you can cancel it anytime during the 3-minute countdown.

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
- New window monitoring function modules

---

[Back to Main Page]() | [View Chinese Version](README_zh.md)
