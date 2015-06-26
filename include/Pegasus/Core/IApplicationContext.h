/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   IApplicationContext.h
//! \author David Worsham
//! \date   01st Nov 2013
//! \brief  Context interface for windows, for access to external parameters

#ifndef PEGASUS_WND_IWINDOWCONTEXT_H
#define PEGASUS_WND_IWINDOWCONTEXT_H

//forward declarations to avoid direct package dependencies
namespace Pegasus {
    namespace Io {
        class IOManager;
    }
    namespace Graph {
        class NodeManager;
    }
    namespace Shader {
        class ShaderManager;
    }
    namespace Texture {
        class TextureManager;
    }
    namespace Mesh {
        class MeshManager;
    }
    namespace Camera {
        class CameraManager;
    }
    namespace Timeline {
        class Timeline;
    }
    namespace BlockScript {
        class BlockScriptManager;
    }
    namespace AssetLib {
        class AssetLib;
    }
}

namespace Pegasus {
namespace Core {

//! Context interface for windows
class IApplicationContext 
{
public:
    //! Destructor
    virtual ~IApplicationContext() {};


    //! Gets the IO manager for use by this window
    //! \return The IO manager.
    virtual Io::IOManager* GetIOManager() const = 0;

    //! Gets the node manager for use by this window
    //! \return The node manager.
    virtual Graph::NodeManager* GetNodeManager() const = 0;

    //! Gets the shader manager for use by this window
    //! \return The shader manager.
    virtual Shader::ShaderManager* GetShaderManager() const = 0;

    //! Gets the texture manager for use by this window
    //! \return The texture manager.
    virtual Texture::TextureManager* GetTextureManager() const = 0;

    //! Gets the texture manager for use by this window
    //! \return The texture manager.
    virtual Mesh::MeshManager* GetMeshManager() const = 0;

    //! Gets the camera manager for use by this window
    //! \return The camera manager.
    virtual Camera::CameraManager* GetCameraManager() const = 0;

    //! Gets the timeline for use by this window
    //! \return The timeline.
    virtual Timeline::Timeline* GetTimeline() const = 0;

    //! Gets the blockscript manager to construct / create block script libraries / scripts
    //! \return the BlockScriptManager
    virtual BlockScript::BlockScriptManager* GetBlockScriptManager() const = 0;

    //! Gets the asset library
    //! \return AssetLibrary
    virtual AssetLib::AssetLib* GetAssetLib() const = 0;
};


}   // namespace Wnd
}   // namespace Pegasus

#endif  // PEGASUS_WND_IWINDOWCONTEXT_H