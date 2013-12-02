/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	BlockProxy.cpp
//! \author	Kevin Boulanger
//! \date	09th November 2013
//! \brief	Proxy object, used by the editor to interact with the timeline blocks

//! \todo Why do we need this in Rel-Debug? LaneProxy should not even be compiled in REL mode
PEGASUS_AVOID_EMPTY_FILE_WARNING

#if PEGASUS_ENABLE_PROXIES

#include "Pegasus/Timeline/BlockProxy.h"
#include "Pegasus/Timeline/Block.h"

namespace Pegasus {
namespace Timeline {


BlockProxy::BlockProxy(Block * block)
:   mBlock(block)
{
    PG_ASSERTSTR(block != nullptr, "Trying to create a timeline block proxy from an invalid timeline block object");
}

//----------------------------------------------------------------------------------------

BlockProxy::~BlockProxy()
{
}

//----------------------------------------------------------------------------------------

void BlockProxy::SetPosition(float position)
{
    mBlock->SetPosition(position);
}

//----------------------------------------------------------------------------------------

float BlockProxy::GetPosition() const
{
    return mBlock->GetPosition();
}

//----------------------------------------------------------------------------------------

void BlockProxy::SetLength(float length)
{
    mBlock->SetLength(length);
}

//----------------------------------------------------------------------------------------

float BlockProxy::GetLength() const
{
    return mBlock->GetLength();
}


}   // namespace Timeline
}   // namespace Pegasus

#endif  // PEGASUS_ENABLE_PROXIES