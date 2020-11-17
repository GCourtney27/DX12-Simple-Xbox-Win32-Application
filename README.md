# Simple Xbox Win32 DX12 Application
A simple cross platform Direct3D 12 application that runs on Xbox One, Win32 desktops and as a Windows Store app. You can run as a Xbox application, a standalone UWP app, or standalone Win32 app. C++/CX and C++/WinRT frameworks are supported and are domonstrated using their respective projects in the applicaions folder inside Visual Studio.

## Platforms
Windows (Win32 mode) <br/>
Windows 10 (UWP mode) <br/>
Xbox One (UWP mode) <br/>

## Requirements
Visual Studio 2017/19 <br/>
Microsoft Developer Account (If you're deploying to the Xbox. You dont need it to deploy on Windows or as a regular UWP app though) <br/>
DirectX 12 compatible graphics card (If running on PC) <br/>

## Getting Started
### Windows Deployment
1. Download and install Universal Windows Platform module for Visual Studio from the Visual Studio installer. <br/>
2. Make sure you have the most recent Windows 10 SDK. <br/>
3. To run as a standard Win32 application set "Application_Windows" as the startup project and run. <br/>
4. To run as a Windows Store app (UWP) set "Application_UWP" as the startup project and run. Windows Store App and Universal Windows Platform are the same thing, they both run on the Xbox. <br/>

### Xbox One Deployment
* Make sure your xbox is in dev mode before proceeding <br/>
1. Set "Application_UWP" as the startup project. <br/>
2. Set the debugger type to "Remote Windows Debugger" and enter your consoles IP address when prompted. If not prompted right click on "Application_UWP" and go the properties pannel. Click on "Debugging" then "Remote Machine" in the "Machine Name" field enter your consoles IP address. <br/>
3. Run, and the project will be uploaded to the Xbox and start automatically. <br/>
