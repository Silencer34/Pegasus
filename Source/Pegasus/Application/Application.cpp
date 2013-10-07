/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   Application.cpp
//! \author David Worsham
//! \date   4th July 2013
//! \brief  Building block class for a Pegasus application.
//!         Manages access to the Pegasus runtime.

#include "Pegasus/Application/Application.h"
#include "Pegasus/Application/Shared/ApplicationConfig.h"
#include "Pegasus/Render/GL/GLExtensions.h"
#include "Pegasus/Render/RenderContext.h"
#include "Pegasus/Window/Window.h"

#include <windows.h>

namespace Pegasus {
namespace Application {

//----------------------------------------------------------------------------------------

Application::Application()
    : mInitialized(false), mModuleHandle(NULL), mNumWindows(0)
{
    // Init windows array
    for (unsigned int i = 0; i < MAX_NUM_WINDOWS; i++)
    {
        mWindows[i] = nullptr;
    }
}

//----------------------------------------------------------------------------------------

Application::~Application()
{
    // Handle the extension manager and stuff
    ShutdownAppInternal();

    //! \todo Assert that no windows exist here, to alert the user

    // Free windows
    for (unsigned int i = 0; i < MAX_NUM_WINDOWS; i++)
    {
        if (mWindows[i] != nullptr)
        {
            DetachWindow(mWindows[i]);
        }
    }
}

//----------------------------------------------------------------------------------------

void Application::Initialize(const ApplicationConfig& config)
{
    Window::Window* startupWindow = nullptr;

    // Cache off application handle
    mModuleHandle = config.mModuleHandle;

    // start up the app, which creates and destroys the dummy window
    StartupAppInternal();

    //! \todo Assert on init
    // Initted
    mInitialized = true;
}

//----------------------------------------------------------------------------------------

//! Shutdown this application.
void Application::Shutdown()
{
    //! \todo Assert on init
    // No longer initted
    mInitialized = false;
}

//----------------------------------------------------------------------------------------

int Application::Run()
{
    MSG curMsg;
    bool appDone = false;

    // Run message pump until application exits
    while(!appDone)
    {
        // Grab a message and dispatch it
        PeekMessage(&curMsg, NULL, NULL, NULL, PM_REMOVE);
        if (curMsg.message == WM_QUIT)
        {
            // Bail out, app is finished
            appDone = true;
        }
        else
        {
            // Draw stuff
            Render();

            // Dispatch it
            TranslateMessage(&curMsg); 
            DispatchMessage(&curMsg);
        }
    }

    // wParam of WM_QUIT is the app exit code
    return curMsg.wParam;

    return 0;
}

//----------------------------------------------------------------------------------------

void Application::Render()
{
    // Present all windows
    for (unsigned int i = 0; i < mNumWindows; i++)
    {
        mWindows[i]->GetRenderContext()->Swap();
    }
}

//----------------------------------------------------------------------------------------

Window::Window* Application::AttachWindow(const AppWindowConfig& appConfig)
{
    Window::Window* newWnd = nullptr;
    Window::WindowConfig config;

    // Create window
    //! \todo Assert/log on failure
    config.mModuleHandle = mModuleHandle;
    config.mIsStartupWindow = false;
    newWnd = new Window::Window(config);

    // Assign it
    //! \todo Assert on windows list not full
    if (newWnd)
    {
        mWindows[mNumWindows++] = newWnd;
    }

    return newWnd;
}

//----------------------------------------------------------------------------------------

void Application::DetachWindow(const Window::Window* wnd)
{
    //! \todo Assert window not null
    //! \todo Assert window ID in range
    //delete mWindows[mNumWindows - 1];
    delete wnd;
    mWindows[mNumWindows - 1] = nullptr; // Aliases wnd
    mNumWindows--;
}

//----------------------------------------------------------------------------------------

//! Starts up the application.
//! \note Creates the dummy startup window used to initialize the OGL extensions.
void Application::StartupAppInternal()
{
    Window::Window* newWnd = nullptr;
    Window::WindowConfig config;

    // First register window classes
    Window::Window::RegisterWindowClass(mModuleHandle);

    // Create window and immediately destroy it
    //! \todo Assert/log on failure
    config.mModuleHandle = mModuleHandle;
    config.mIsStartupWindow = true;
    newWnd = new Window::Window(config);

    // Init openGL extensions now that we have a context
    Render::GLExtensions::CreateInstance();

    // Write some temporary debugging information
    //! \todo Remove this after testing
    Render::GLExtensions & extensions = Render::GLExtensions::GetInstance();
    switch (extensions.GetMaximumProfile())
    {
    case Render::GLExtensions::PROFILE_GL_3_3:
        OutputDebugString("OpenGL 3.3 is the maximum detected profile.\n");
        break;

    case Render::GLExtensions::PROFILE_GL_4_3:
        OutputDebugString("OpenGL 4.3 is the maximum detected profile.\n");
        break;

    default:
        OutputDebugString("Error when initializing GLExtensions.\n");
        break;
    }
    if (extensions.IsGLExtensionSupported("GL_ARB_draw_indirect"))
    {
        OutputDebugString("GL_ARB_draw_indirect detected.\n");
    }
    else
    {
        OutputDebugString("GL_ARB_draw_indirect NOT detected.\n");
    }
    if (extensions.IsGLExtensionSupported("GL_ATI_fragment_shader"))
    {
        OutputDebugString("GL_ATI_fragment_shader detected.\n");
    }
    else
    {
        OutputDebugString("GL_ATI_fragment_shader NOT detected.\n");
    }
    if (extensions.IsWGLExtensionSupported("WGL_ARB_buffer_region"))
    {
        OutputDebugString("WGL_ARB_buffer_region detected.\n");
    }
    else
    {
        OutputDebugString("WGL_ARB_buffer_region NOT detected.\n");
    }
    if (extensions.IsWGLExtensionSupported("WGL_3DL_stereo_control"))
    {
        OutputDebugString("WGL_3DL_stereo_control detected.\n");
    }
    else
    {
        OutputDebugString("WGL_3DL_stereo_control NOT detected.\n");
    }

    // Destroy the window, it is no longer needed
    delete newWnd;
}

//----------------------------------------------------------------------------------------

//! shuts down the application.
void Application::ShutdownAppInternal()
{
    // Destroy openGL extensions
    Render::GLExtensions::DestroyInstance();

    // Unregister window classes
    Window::Window::UnregisterWindowClass(mModuleHandle);
}

//----------------------------------------------------------------------------------------

}   // namespace Application
}   // namespace Pegasus