#if PEGASUS_ENABLE_PROXIES
#include "Pegasus/Core/Shared/IoErrors.h"
#include "Pegasus/AssetLib/Proxy/RuntimeAssetObjectProxy.h"
#include "Pegasus/AssetLib/Proxy/AssetProxy.h"
#include "Pegasus/AssetLib/RuntimeAssetObject.h"
#include "Pegasus/AssetLib/Asset.h"
#include "Pegasus/AssetLib/AssetLib.h"
#include "Pegasus/Utils/String.h"
#include "Pegasus/Core/Log.h"


using namespace Pegasus;
using namespace Pegasus::AssetLib;

RuntimeAssetObjectProxy::RuntimeAssetObjectProxy(RuntimeAssetObject* object)
: mObject(object)
{
}

RuntimeAssetObjectProxy::~RuntimeAssetObjectProxy()
{
}


IAssetProxy* RuntimeAssetObjectProxy::GetOwnerAsset() const
{
    return mObject->GetOwnerAsset()->GetProxy();
}

const char* RuntimeAssetObjectProxy::GetDisplayName() const
{
    return mObject->GetDisplayName();
}

void RuntimeAssetObjectProxy::Write(IAssetProxy* asset)
{
    mObject->Write(static_cast<AssetProxy*>(asset)->GetObject());
}

bool RuntimeAssetObjectProxy::ReloadFromAsset()
{
    Asset* asset = mObject->GetOwnerAsset();
    AssetLib* lib = asset->GetLib();

    //copy the path to a temporary buffer. before destroying the asset
    const int MAX_TEMP_PATH = 512;
    char mTemporaryPath[MAX_TEMP_PATH];
    mTemporaryPath[0] = '\0';
    const char* oldPath = asset->GetPath();
    PG_ASSERTSTR(Utils::Strlen(oldPath) + 1 <= MAX_TEMP_PATH, "Temporary path buffer is not big enough! you must increaseit! this will cause a stomp.");
    
    Utils::Strcat(mTemporaryPath, oldPath);
    bool wasStructured = asset->GetFormat() == Asset::FMT_STRUCTURED;
    const Pegasus::PegasusAssetTypeDesc* oldType = asset->GetTypeDesc();
    lib->UnloadAsset(asset);    
    asset = nullptr;
    if (lib->LoadAsset(mTemporaryPath, wasStructured, &asset) != Pegasus::Io::ERR_NONE)
    {
        PG_LOG('ERR_', "Trying to reload an asset (%s) but failed.", mTemporaryPath);
        return false;
    }
    asset->SetTypeDesc(oldType);
    bool retValue = mObject->Read(asset);
    if (asset != mObject->GetOwnerAsset())
    {
        PG_FAILSTR("The asset trying to reload is not opened already!");
        return false;
    }
    else
    {
        return retValue;
    }
}


#else
PEGASUS_AVOID_EMPTY_FILE_WARNING
#endif
