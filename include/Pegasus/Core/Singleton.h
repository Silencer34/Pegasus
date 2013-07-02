/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	Singleton.h
//! \author	Kevin Boulanger
//! \date	24th June 2013
//! \brief	Singleton base classes (manual and automatic creation)

#ifndef PEGASUS_CORE_SINGLETON_H
#define PEGASUS_CORE_SINGLETON_H

namespace Pegasus {
namespace Core {


//! \class Singleton with manual creation/destruction
//! Base class for an object that has to exist only once in the application.
//! Creation and destruction have to be handled manually.
//! \warning \a GetInstance() can return nullptr, so be careful when using the instance pointer
template <class C>
class ManualSingleton
{
public:

    //! Create the unique instance of the class
    //! \warning Cannot be called twice without a call to \a DestroyInstance() first
    static void CreateInstance();

    //! Destroy the unique instance of the class
    //! \warning \a CreateInstace() must be called at least once before this function
    static void DestroyInstance();

    //! Get the unique instance of the class
    //! \return Pointer to the unique instance of the class,
    //!         nullptr is \a CreateInstance() has not been called yet
    static C * GetInstance();


protected:

    //! Constructor, protected to allow only the derived class' constructor to call it
    ManualSingleton() { };

    //! Destructor, protected to allow only the derived class' destructor to call it
    virtual ~ManualSingleton() { };


private:

    //! Copy constructor, private to prevent any copy of the singleton
    explicit ManualSingleton(const C &);

    //! Copy operator, private to prevent any copy of the singleton
    C & operator=(const C &);

    //! Unique instance of the class, nullptr by default
    static C * sInstance;
};

//----------------------------------------------------------------------------------------

//! \class Singleton with automatic creation
//! Base class for an object that has to exist for the entire life of the application.
//! The singleton is created when calling \a GetInstance() for the first time.
//! \warning \a GetInstance() is not thread-safe
//! \todo Create a thread-safe version of this class
template <class C>
class AutoSingleton
{
public:

    //! Get the unique instance of the class
    //! \return Reference to the unique instance of the class, always valid
    static C & GetInstance();


protected:

    //! Constructor, protected to allow only the derived class' constructor to call it
    AutoSingleton() { };

    //! Destructor, protected to allow only the derived class' destructor to call it
    virtual ~AutoSingleton() { };


private:

    //! Copy constructor, private to prevent any copy of the singleton
    explicit AutoSingleton(const C &);

    //! Copy operator, private to prevent any copy of the singleton
    C & operator=(const C &);

    //! Unique instance of the class, nullptr only before the first call to \a GetInstance()
    static C * sInstance;
};


//----------------------------------------------------------------------------------------
// Implementation


template <class C>
C * ManualSingleton<C>::sInstance = nullptr;

template <class C>
C * AutoSingleton<C>::sInstance = nullptr;

//----------------------------------------------------------------------------------------

template <class C>
void ManualSingleton<C>::CreateInstance()
{
    if (sInstance == nullptr)
    {
        //! \todo Provide an allocator
        sInstance = new C();
    }
    else
    {
        //! \todo Handle error or assertion
    }
}

//----------------------------------------------------------------------------------------

template <class C>
void ManualSingleton<C>::DestroyInstance()
{
    if (sInstance != nullptr)
    {
        //! \todo Provide an allocator
        delete sInstance;
        sInstance = nullptr;
    }
    else
    {
        //! \todo Handle error or assertion
    }
}

//----------------------------------------------------------------------------------------

template <class C>
C * ManualSingleton<C>::GetInstance()
{
    return sInstance;
}

//----------------------------------------------------------------------------------------

template <class C>
C & AutoSingleton<C>::GetInstance()
{
    if (sInstance == nullptr)
    {
        //! \todo Provide an allocator?
        sInstance = new C();
    }

    return *sInstance;
}


}   // namespace Core
}   // namespace Pegasus

#endif  // PEGASUS_CORE_ASSERTION_H