#include "Win32Platform.h"
#include <Windowsx.h>

#include "Config.h"

#include "ServiceProvider.h"
#include "Input.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

static bool gIsRunning = false;

LRESULT Wndproc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

Win32Platform::Win32Platform(const Config& config)
{
	mInstance = GetModuleHandle(0);

	WNDCLASS wndClass = {};
    wndClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    wndClass.lpfnWndProc = Wndproc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = mInstance;
    wndClass.hIcon = 0;
    wndClass.hCursor = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = "Win32StardustEngineClass";
    if(!RegisterClassA(&wndClass))
    {
    	return;
    }

    // TODO: Adjust Window Rect
    mWindow = CreateWindowA(wndClass.lpszClassName,
    						config.Title, WS_OVERLAPPEDWINDOW,
    						CW_USEDEFAULT, CW_USEDEFAULT,
    						config.ScreenWidth, config.ScreenHeight,
    						0, 0, mInstance, this);
    if(!mWindow)
    {
    	return;
    }

    UpdateWindow(mWindow);
    ShowWindow(mWindow, SW_SHOW);

    gIsRunning = true;
    mIsPaused = false;

    QueryPerformanceFrequency(&mFrequency);
}

Win32Platform::~Win32Platform()
{
	DestroyWindow(mWindow);
}

bool Win32Platform::ShouldClose()
{
	return !gIsRunning;
}

bool Win32Platform::IsPaused()
{
    return mIsPaused;   
}

void Win32Platform::ProcessEvents()
{
	MSG message;
	while(PeekMessageA(&message, mWindow, 0, 0, PM_REMOVE))
	{
        TranslateMessage(&message);
    	DispatchMessageA(&message);
	}
}

void *Win32Platform::GetWindowHandle()
{
    return (void *)mWindow;
}

double Win32Platform::GetTime()
{
    LARGE_INTEGER currentCounter;
    QueryPerformanceCounter(&currentCounter);
    double currentTime = (double)currentCounter.QuadPart / mFrequency.QuadPart;
    return currentTime;
}

LRESULT Win32Platform::MsgProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = {};
    switch(message)
    {
        case WM_CLOSE:
        {
            gIsRunning = false;
        } break;
        case WM_SIZE:
        {
            if(wParam == SIZE_MINIMIZED)
            {
                mIsPaused = true;
            }
            if(wParam == SIZE_MAXIMIZED)
            {
                mIsPaused = false;
            }
            if(wParam == SIZE_RESTORED)
            {
                mIsPaused = false;
            }
            if(!IsPaused())
            {
                WindowResizeEvent event;
                event.Width = LOWORD(lParam);
                event.Height = HIWORD(lParam);
                GetEventBus()->RiseEvent(EventType::WindowResizeEvent, event);
            }
        } break;
        case WM_GETMINMAXINFO:
        {
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;  
        } break;
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            DWORD keyCode = (DWORD)wParam;
            bool isDown = ((lParam & (1u << 31)) == 0);
            GetInput()->SetKey(keyCode, isDown);
        } break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        {
            GetInput()->SetMouseButton(MOUSE_BUTTON_LEFT, (wParam & MK_LBUTTON) != 0); 
            GetInput()->SetMouseButton(MOUSE_BUTTON_MIDDLE, (wParam & MK_MBUTTON) != 0); 
            GetInput()->SetMouseButton(MOUSE_BUTTON_RIGHT, (wParam & MK_RBUTTON) != 0); 
        } break;
        case WM_MOUSEMOVE:
        {
            int mouseX = (int)GET_X_LPARAM(lParam);
            int mouseY = (int)GET_Y_LPARAM(lParam);
            GetInput()->SetMousePosition(mouseX, mouseY);
        } break; 
        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);

        } break;
    }
    return result;
}

const char* Win32Platform::GetVulkanPlatformExtension()
{
    return "VK_KHR_win32_surface";
}

bool Win32Platform::CreateVulkanPlatformSurface(const VkInstance& instance, VkSurfaceKHR& surface)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = mWindow;
    createInfo.hinstance = mInstance;
    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
    {
        return false;
    }
    return true;
}

LRESULT Wndproc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE)
    {
        // Extraemos el puntero 'this' pasado en CreateWindowA
        CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    Win32Platform *platform = reinterpret_cast<Win32Platform *>(GetWindowLongPtr(window, GWLP_USERDATA));
    return platform->MsgProc(window, message, wParam, lParam);
}