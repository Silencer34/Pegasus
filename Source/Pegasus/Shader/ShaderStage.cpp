/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   ShaderStage.cpp
//! \author Kleber Garcia
//! \date   1st December 2013
//! \brief  Pegasus Shader Stage	

#include "Pegasus/Core/Shared/CompilerEvents.h"
#include "Pegasus/Shader/ShaderStage.h"
#include "Pegasus/Shader/IShaderFactory.h"
#include "Pegasus/Utils/String.h"
#include "Pegasus/Utils/Memcpy.h"
#include "Pegasus/Shader/ShaderTracker.h"

using namespace Pegasus::Core;


//! private data structures
namespace PegasusShaderPrivate {


//lookup struct array with extension / GL type / pegasus Shader stage mappings
static struct PegasusExtensionMappings
{
    Pegasus::Shader::ShaderType mType;
    const char * mExtension;
} gPegasusShaderStageExtensionMappings[Pegasus::Shader::SHADER_STAGES_COUNT] =
{
    { Pegasus::Shader::VERTEX,                 ".vs"  },
    { Pegasus::Shader::FRAGMENT,               ".ps"  },
    { Pegasus::Shader::TESSELATION_CONTROL,    ".tcs" },
    { Pegasus::Shader::TESSELATION_EVALUATION, ".tes" },
    { Pegasus::Shader::GEOMETRY,               ".gs"  },
    { Pegasus::Shader::COMPUTE,                ".cs"  }
}
;

} // namespace PegasusShaderPrivate

Pegasus::Shader::ShaderStage::ShaderStage(Pegasus::Alloc::IAllocator * allocator, Pegasus::Alloc::IAllocator * nodeDataAllocator)
    : Pegasus::Graph::GeneratorNode(allocator, nodeDataAllocator), 
      mAllocator(allocator),
      mType(Pegasus::Shader::SHADER_STAGE_INVALID),
      mFactory(nullptr),
      mLoader(nullptr)
#if PEGASUS_ENABLE_PROXIES
      , mShaderTracker(nullptr)
      , mProxy(this)
#endif
{
    GRAPH_EVENT_INIT_DISPATCHER
}

Pegasus::Shader::ShaderStage::~ShaderStage()
{
    if (GetData() != nullptr)
    {
        mFactory->DestroyShaderGPUData(&(*GetData()));
    }
#if PEGASUS_ENABLE_PROXIES
    if (mShaderTracker != nullptr)
    {
        mShaderTracker->DeleteShader(this);
    }
#endif

    GRAPH_EVENT_DISPATCH(
        this,
        Pegasus::Core::CompilerEvents::ObjectOperation, 
        // Event specific arguments:
        Pegasus::Core::CompilerEvents::ObjectOperation::DESTROYED_OPERATION,
        "ShaderStage"
    );

}

void Pegasus::Shader::ShaderStage::ReleaseDataAndPropagate()
{
    if (GetData() != nullptr)
    {
#if PEGASUS_ENABLE_DETAILED_LOG
#if PEGASUS_ENABLE_PROXIES
    PG_LOG('SHDR', "Destroying the GPU data of shader stage \"%s\"", GetName());
#else
    PG_LOG('SHDR', "Destroying the GPU data of a shader stage");
#endif
#endif  // PEGASUS_ENABLE_DETAILED_LOG

        mFactory->DestroyShaderGPUData(&(*GetData()));
    }
    Pegasus::Graph::Node::ReleaseDataAndPropagate();
}

void Pegasus::Shader::ShaderStage::SetSource(Pegasus::Shader::ShaderType type, const char * src, int srcSize)
{

    InvalidateData();

    //reallocate buffer size if more space requested on recompilation
    if (srcSize > mFileBuffer.GetFileSize())
    {
        mFileBuffer.DestroyBuffer();
        mFileBuffer.OwnBuffer (
            mAllocator,
            PG_NEW_ARRAY(mAllocator, -1, "shader src", Pegasus::Alloc::PG_MEM_PERM, char, srcSize),
            srcSize
        );
    }
    mFileBuffer.SetFileSize(srcSize);
    PG_ASSERTSTR(mFileBuffer.GetBufferSize() >= srcSize, "Not enough size to hold the string buffer!");
    Pegasus::Utils::Memcpy(mFileBuffer.GetBuffer(),src,srcSize);
    mType = type;
}

void Pegasus::Shader::ShaderStage::SetSource(const char * src, int srcSize)
{
    PG_ASSERT(mType != Pegasus::Shader::SHADER_STAGE_INVALID);
    SetSource(mType, src, srcSize);
}

void Pegasus::Shader::ShaderStage::GetSource ( const char ** outSrc, int& outSize) const
{
    *outSrc = mFileBuffer.GetBuffer(); 
    outSize = mFileBuffer.GetFileSize();
}

bool Pegasus::Shader::ShaderStage::SetSourceFromFile(Pegasus::Shader::ShaderType type, const char * path)
{
    InvalidateData();
    PG_ASSERTSTR(mLoader != nullptr, "You must set the file loader first before calling any IO!");
    PG_ASSERT(path != nullptr);
    mType = type;
    if (mType != Pegasus::Shader::SHADER_STAGE_INVALID)
    {
        mFileBuffer.DestroyBuffer(); //clear any buffers pre-allocated to this
        Pegasus::Io::IoError ioError = mLoader->OpenFileToBuffer(path, mFileBuffer, true, mAllocator);
        if (ioError == Pegasus::Io::ERR_NONE)
        {
            GRAPH_EVENT_DISPATCH(
                this,
                CompilerEvents::SourceLoadedEvent, 
                // Event specific arguments:
                mFileBuffer.GetBuffer(), 
                mFileBuffer.GetFileSize()
            );
            return true;
        }
        else
        {
            GRAPH_EVENT_DISPATCH(
                this,
                CompilerEvents::FileOperationEvent, 
                // Event specific arguments:
                CompilerEvents::FileOperationEvent::IO_ERROR, 
                ioError,
                path, 
                "Io error"
            );
        }
    }
    else
    {
        GRAPH_EVENT_DISPATCH(
            this,
            CompilerEvents::FileOperationEvent, 
            // Event specific arguments:
            CompilerEvents::FileOperationEvent::WRONG_EXTENSION, 
            Pegasus::Io::ERR_NONE,
            path, 
            "wrong file format"
        );
    }
    return false;
}


void Pegasus::Shader::ShaderStage::InvalidateData()
{
    //! mark data as dirty
    if (GetData() != nullptr)
    {
        GetData()->Invalidate();
    }
}

Pegasus::Graph::NodeData * Pegasus::Shader::ShaderStage::AllocateData() const
{
    return PG_NEW(GetNodeDataAllocator(), -1, "Shader Node Data", Pegasus::Alloc::PG_MEM_TEMP) Pegasus::Graph::NodeData(GetNodeDataAllocator());
}

void Pegasus::Shader::ShaderStage::GenerateData()
{
    PG_ASSERT(GetData() != nullptr);

#if PEGASUS_ENABLE_DETAILED_LOG
#if PEGASUS_ENABLE_PROXIES
    PG_LOG('SHDR', "Generating the GPU data of shader stage \"%s\"", GetFileName());
#else
    PG_LOG('SHDR', "Generating the GPU data of a shader stage");
#endif
#endif  // PEGASUS_ENABLE_DETAILED_LOG

    GRAPH_EVENT_DISPATCH(
        this,
        CompilerEvents::CompilationNotification, 
        // Event specific arguments:
        CompilerEvents::CompilationNotification::COMPILATION_BEGIN, 
        0, // unused
        "" // unused
    );

    mFactory->GenerateShaderGPUData(&(*this), &(*GetData()));
} 


//! editor metadata
#if PEGASUS_ENABLE_PROXIES
void Pegasus::Shader::ShaderStage::SetFullFilePath(const char * name)
{
    int len = 0;
    if (name)
    {
        mFullPath[0] = '\0';
        PG_ASSERT(Pegasus::Utils::Strlen(name) < METADATA_NAME_LENGTH * 2); //does it all fit?
        Pegasus::Utils::Strcat(mFullPath, name);
        int fullLen = Pegasus::Utils::Strlen(name);
        const char * nameString1 = Pegasus::Utils::Strrchr(name, '/');
        const char * nameString2 = Pegasus::Utils::Strrchr(name, '\\');
        const char * nameString = nameString1 > nameString2 ? nameString1 : nameString2;
        if (nameString != nullptr)
        {
            fullLen = fullLen - (nameString - name + 1);
            Pegasus::Utils::Memcpy(mName, nameString + 1, fullLen);
            mName[fullLen] = '\0';
            fullLen = nameString - name + 1;
            Pegasus::Utils::Memcpy(mPath, name, fullLen);
            mPath[fullLen] = '\0';
        }
        else
        {
            len = fullLen < Pegasus::Shader::ShaderStage::METADATA_NAME_LENGTH - 1 ? fullLen : Pegasus::Shader::ShaderStage::METADATA_NAME_LENGTH - 1;
            Pegasus::Utils::Memcpy(mName, name, len);
            mName[len] = '\0';
            mPath[0] = '\0';
        }
    }
} 

void Pegasus::Shader::ShaderStage::SaveSourceToFile()
{
    Pegasus::Io::IoError err = mLoader->SaveFileToBuffer(mFullPath, mFileBuffer);
    if (err == Pegasus::Io::ERR_NONE)
    {
        GRAPH_EVENT_DISPATCH(
            this,
            CompilerEvents::FileOperationEvent, 
            // Event specific arguments:
            CompilerEvents::FileOperationEvent::IO_FILE_SAVE_SUCCESS,
            err,
            mFullPath,
            ""
        );
    }
    else
    {
        GRAPH_EVENT_DISPATCH (
            this,
            CompilerEvents::FileOperationEvent, 
            // Event specific arguments:
            CompilerEvents::FileOperationEvent::IO_FILE_SAVE_ERROR,
            err,
            mFullPath,
            "Error saving file :/"
        );
    }

}

#endif


Pegasus::Graph::NodeReturn Pegasus::Shader::ShaderStage::CreateNode(Alloc::IAllocator* nodeAllocator, Alloc::IAllocator* nodeDataAllocator)
{
    return PG_NEW(nodeAllocator, -1, "ShaderStage", Pegasus::Alloc::PG_MEM_TEMP) Pegasus::Shader::ShaderStage(nodeAllocator, nodeDataAllocator);
}
