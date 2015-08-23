/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus                                           */
/*                                                                                      */
/****************************************************************************************/

//! \file	PegasusAssetTypes.h
//! \author	Kleber Garcia
//! \date	30th July 2015
//! \brief	high level description of all the assets this app can have.
//!         \note Edit and view PegasusAssetTypes.inl for full data description of assets

#ifndef PEGASUS_ASSET_TYPES_DEFS_H
#define PEGASUS_ASSET_TYPES_DEFS_H

namespace Pegasus {

//Structure that describes the type of asset that pegasus has
struct PegasusAssetTypeDesc
{
    int         mTypeGuid;  //!< the int guid. Must be unique per asset.
    const char* mTypeName;  //!< the human readable name of this asset
    const char* mExtension; //!< the file extension of this asset type
    bool        mIsStructured; //!< true if this asset is structured, false otherwise
                               //!< a structured asset means is a json file.
                               //!< a non structured asset means is a raw file
};

#define REGISTER_ASSET_TYPE(__runtimeName, __typeGuid, __typeName, __ext, __isStruct) \
    const PegasusAssetTypeDesc __runtimeName = { __typeGuid, __typeName, __ext, __isStruct };   
#include "Pegasus/PegasusAssetTypes.inl"
#undef REGISTER_ASSET_TYPE

/////////////////////// ALL ASSETS ////////////////////////
//! \return a list of all the asset types declared in this file. 
//          The list ends with a null pointer.
const PegasusAssetTypeDesc * const *  GetAllAssetTypesDescs();
///////////////////////////////////////////////////////////

}

#endif //PEGASUS_ASSET_TYPES_DEFS_H

