/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   GLRenderContextWin32.cpp
//! \author David Worsham / Kleber Garcia
//! \date   05th July 2013
//! \brief  Class that encapsulates an OGL rendering context, for windows specific platforms.

#if PEGASUS_GAPI_GL && PEGASUS_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Pegasus/Render/RenderContext.h"
#include "../Source/Pegasus/Render/GL/GLEWStaticInclude.h"
#include "../Source/Pegasus/Render/GL/GLExtensions.h"
#include "../Source/Pegasus/Render/GL/GLDeviceWin32.h"

namespace Pegasus {
namespace Render {

// We must run all the windows and render context drawing calls in the same thread, therefor enforcing this by
// making the following globals part of the thread local storage.    
static HGLRC gRenderContextHandle = 0; //!< Opaque GL context handle
static int   gRenderContextHandleReferences = 0; //reference count for the GL context
static HDC   gDeviceContextHandle = 0; // unique device context handle, for window display

Context::Context(const ContextConfig& config)
    : mAllocator(config.mAllocator),
      mParentDevice(config.mDevice),
      mDeviceContextHandle(config.mDeviceContextHandle)
{
    PG_ASSERT(mParentDevice != nullptr);
    GLDeviceWin32 * winDevice = static_cast<GLDeviceWin32*>(mParentDevice);
    HDC drawContextHandle = reinterpret_cast<HDC>(mDeviceContextHandle);

    // Full context
    int nPixelFormat = ChoosePixelFormat(drawContextHandle, winDevice->GetPixelFormatDescriptor());

    // Setup pixel format for backbuffer
    SetPixelFormat(drawContextHandle, nPixelFormat, winDevice->GetPixelFormatDescriptor());
    if (gRenderContextHandleReferences == 0)
    {
        // Make a new OpenGL context
        gRenderContextHandle = (HGLRC)wglCreateContextAttribsARB(drawContextHandle, 0, winDevice->GetContextAttributes());
    }

    PG_ASSERTSTR(gRenderContextHandle != 0x0, "Cannot instantiate render context! Check if your graphcis card supports the minor and major version");

    ++gRenderContextHandleReferences;

    PG_LOG('OGL_', "Context %u created", gRenderContextHandle);

    // Link it to the window
    Bind();
}

//----------------------------------------------------------------------------------------

Context::~Context()
{
    // Unbind and destroy the context
    Unbind();
    --gRenderContextHandleReferences;

    if (gRenderContextHandleReferences == 0)
    {
        wglDeleteContext(gRenderContextHandle);
        gRenderContextHandle = 0;
    }

    PG_LOG('OGL_', "Context %u destroyed", gRenderContextHandle);
}

//----------------------------------------------------------------------------------------

void Context::Bind() const
{
    if (gDeviceContextHandle != (HDC)mDeviceContextHandle)
    {
        wglMakeCurrent((HDC)mDeviceContextHandle, gRenderContextHandle);
        gDeviceContextHandle = (HDC)mDeviceContextHandle;
        PG_LOG('OGL_', "%u is now the active context", gRenderContextHandle);
    }
}

//----------------------------------------------------------------------------------------

void Context::Unbind() const
{ 
    wglMakeCurrent((HDC)mDeviceContextHandle, NULL);
    gDeviceContextHandle = NULL;
    PG_LOG('OGL_', "NULL is now the active context");
}

//----------------------------------------------------------------------------------------

void Context::Swap() const
{
    // Present (no need for glFlush() since SwapBuffers() takes care of it)
    SwapBuffers((HDC)mDeviceContextHandle);
}


}   // namespace Render
}   // namespace Pegasus
#else
PEGASUS_AVOID_EMPTY_FILE_WARNING
#endif //PEGASUS_PLATFORM_WINDOWS