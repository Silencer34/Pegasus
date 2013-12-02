/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	LaneProxy.h
//! \author	Kevin Boulanger
//! \date	07th November 2013
//! \brief	Proxy object, used by the editor to interact with the timeline lanes

#ifndef PEGASUS_TEXTURE_LANEPROXY_H
#define PEGASUS_TEXTURE_LANEPROXY_H

#if PEGASUS_ENABLE_PROXIES

#include "Pegasus/Timeline/Shared/ILaneProxy.h"

namespace Pegasus {
    namespace Timeline {
        class Lane;
    }
}

namespace Pegasus {
namespace Timeline {


//! Proxy object, used by the editor to interact with the timeline lanes
class LaneProxy : public ILaneProxy
{
public:

    // Constructor
    //! \param lane Proxied timeline lane object, cannot be nullptr
    LaneProxy(Lane * lane);

    //! Destructor
    virtual ~LaneProxy();


    //! Set the name of the lane
    //! \param name New name of the lane, can be empty or nullptr, but no longer than Lane::MAX_NAME_LENGTH
    virtual void SetName(const char * name);

    //! Get the name of the lane
    //! \return Name of the lane, can be empty or nullptr
    virtual const char * GetName() const;

    //------------------------------------------------------------------------------------
    
private:

    //! Proxied timeline lane object
    Lane * const mLane;
};


}   // namespace Timeline
}   // namespace Pegasus

#endif  // PEGASUS_ENABLE_PROXIES
#endif  // PEGASUS_TEXTURE_LANEPROXY_H