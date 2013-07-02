/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	Assertion.h
//! \author	Kevin Boulanger
//! \date	24th June 2013
//! \brief	Assertion test macros and manager

#ifndef PEGASUS_CORE_ASSERTION_H
#define PEGASUS_CORE_ASSERTION_H

#include "Pegasus/Preprocessor.h"


#if PEGASUS_ENABLE_ASSERT

//! Use this macro for hard failures, typically code paths that should not be reached
#define PG_FAIL()               { Pegasus::Core::AssertionManager::GetInstance().AssertionError(\
                                        "FAILURE", __FILE__, __LINE__); }

//! Use this macro for simple assertion tests, which do not require a message to explain the test
#define PG_ASSERT(t)            { if (!(t)) Pegasus::Core::AssertionManager::GetInstance().AssertionError(\
                                        #t, __FILE__, __LINE__); }

//! Use this macro for assertion tests, with a message that describes the test
#define PG_ASSERTSTR(t, str)    { if (!(t)) Pegasus::Core::AssertionManager::GetInstance().AssertionError(\
                                        #t, __FILE__, __LINE__, str); }

#else

#define PG_FAIL()
#define PG_ASSERT(t)
#define PG_ASSERTSTR(t, str)

#endif  // PEGASUS_ENABLE_ASSERT

//----------------------------------------------------------------------------------------

#if PEGASUS_ENABLE_ASSERT

#include "Pegasus/Core/Singleton.h"

namespace Pegasus {
namespace Core {
    
//! \class Assertion manager
//! Assertion manager (singleton) that redirects the macros to the assertion handler
class AssertionManager : public AutoSingleton<AssertionManager>
{
public:

    AssertionManager();
    ~AssertionManager();

    //! Callback function declaration.
    //! One function with this type needs to be declared in the user application
    //! to handle assertion errors.
    //! \param testStr String representing the assertion test itself
    //! \param fileStr String with the filename where the assertion test failed
    //! \param line Line number where the assertion test failed
    //! \param msgStr String of an optional message making the assertion test easier to understand
    typedef void (* Handler)(const char * testStr,
                             const char * fileStr,
                             int line,
                             const char * msgStr);

    //! Register the assertion error handler
    //! \warning To be called at least once by the user application. Otherwise,
    //!          a crash will purposely occur to show where the assertion error happened
    //! \param handler Function pointer of the assertion handler (!= nullptr)
    void RegisterHandler(Handler handler);

    //! Unregister the assertion handler if defined
    void UnregisterHandler();


    //! Throw an assertion error.
    //! The handler registered with \a RegisterHandle is called with the provided parameters
    //! \param testStr String representing the assertion test itself
    //! \param fileStr String with the filename where the assertion test failed
    //! \param line Line number where the assertion test failed
    //! \param msgStr String of an optional message making the assertion test easier to understand
    void AssertionError(const char * testStr,
                        const char * fileStr,
                        int line,
                        const char * msgStr = nullptr);

private:

    //! Function pointer of the assertion error handler, nullptr by default
    Handler mHandler;
};


}   // namespace Core
}   // namespace Pegasus

#endif  // PEGASUS_ENABLE_ASSERT

#endif  // PEGASUS_CORE_ASSERTION_H