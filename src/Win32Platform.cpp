#include "Win32Platform.h"
#include "Config.h"

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
    						0, 0, mInstance, 0);
    if(!mWindow)
    {
    	return;
    }

    UpdateWindow(mWindow);
    ShowWindow(mWindow, SW_SHOW);

    gIsRunning = true;
}

Win32Platform::~Win32Platform()
{
	DestroyWindow(mWindow);
}

bool Win32Platform::ShouldClose()
{
	return !gIsRunning;
}

bool Win32Platform::ProcessEvents()
{
	MSG message;
	if(PeekMessageA(&message, mWindow, 0, 0, PM_REMOVE))
	{
        TranslateMessage(&message);
    	DispatchMessageA(&message);
    	return true;
	}
	return false;
}

void *Win32Platform::GetWindowHandle()
{
    return (void *)mWindow;
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
	if(message == WM_CLOSE)
	{
		gIsRunning = false;
	}
	return DefWindowProc(window, message, wParam, lParam);
}