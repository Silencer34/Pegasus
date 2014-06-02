/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   AppWindowManager.cpp
//! \author David Worsham
//! \date   16th October 2013
//! \brief  Window manager for a Pegasus app.

#include "Pegasus/Application/AppWindowManager.h"
#include "Pegasus/Utils/String.h"
#include "Pegasus/Utils/Memcpy.h"

namespace Pegasus {
namespace App {

//! Window type table entry
struct TypeTableEntry
{
public:
    // Ctor / dtor
    TypeTableEntry();
    TypeTableEntry(const TypeTableEntry& other);
    TypeTableEntry& operator=(const TypeTableEntry& other);
    ~TypeTableEntry();

    // Helpers
    void Clear();
    inline bool TypeEqual(const TypeTableEntry& other) { return (mTypeTag == other.mTypeTag); }
    inline bool ClassEqual(const TypeTableEntry& other) { return (Utils::Strcmp(mClassName, other.mClassName) == 0); }
    inline bool Empty() { return (mClassName[0] == '\0'); }

    WindowTypeTag mTypeTag;
    char mDescription[AppWindowManager::APPWINDOW_DESC_LENGTH]; // Human-friendly description
    char mClassName[AppWindowManager::APPWINDOW_CLASS_LENGTH]; // Class name
    Wnd::WindowFactoryFunc mCreateFunc; // Factory function
};

//----------------------------------------------------------------------------------------

//! Window type table
struct TypeTable
{
public:
    // Ctor / dtor
    TypeTable(unsigned int max, Alloc::IAllocator* alloc);
    ~TypeTable();

    // Helpers
    bool Contains(const char* typeName) const;
    bool Contains(WindowTypeTag typeTag) const;
    TypeTableEntry* Get(const char* typeName) const;
    void Insert(const TypeTableEntry& entry);
    void Remove(const char* typeName);


    Alloc::IAllocator* mAllocator; //! Allocator to use when creating this object
    TypeTableEntry* mTable; // Table members
    unsigned int mCurrentSize; // Current table size
    unsigned int mMaxSize; // Maximum table size
    int mMainWindowIndex; // Index for MAIN window type
#if PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    int mSecondaryWindowIndex; // Index for SECONDARY window type
#endif
};

//----------------------------------------------------------------------------------------

//! Window table
struct WindowTable
{
public:
    // ctor / dtor
    WindowTable(unsigned int max, Alloc::IAllocator* alloc);
    ~WindowTable();

    // Helpers
    bool Contains(const Wnd::Window* entry) const;
    void Insert(Wnd::Window* entry);
    void Remove(Wnd::Window* entry);


    Alloc::IAllocator* mAllocator; //! Allocator to use when creating this object
    Wnd::Window** mTable; // Table members
    unsigned int mCurrentSize; // Table size
    unsigned int mMaxSize; // Max table size
};


//----------------------------------------------------------------------------------------

TypeTableEntry::TypeTableEntry()
    : mTypeTag(WINDOW_TYPE_INVALID),
      mCreateFunc(nullptr)
{
    mDescription[0] = '\0';
    mClassName[0] = '\0';
}

//----------------------------------------------------------------------------------------

TypeTableEntry::~TypeTableEntry()
{
}

//----------------------------------------------------------------------------------------

TypeTableEntry::TypeTableEntry(const TypeTableEntry& other)
{
    if (&other != this)
    {
        mTypeTag = other.mTypeTag;
        mCreateFunc = other.mCreateFunc;
        strncpy_s(mDescription, other.mDescription, AppWindowManager::APPWINDOW_DESC_LENGTH);
        mDescription[AppWindowManager::APPWINDOW_DESC_LENGTH - 1] = '\0';
        strncpy_s(mClassName, other.mClassName, AppWindowManager::APPWINDOW_CLASS_LENGTH);
        mDescription[AppWindowManager::APPWINDOW_CLASS_LENGTH - 1] = '\0';
    }
}

//----------------------------------------------------------------------------------------

TypeTableEntry& TypeTableEntry::operator=(const TypeTableEntry& other)
{
    if (&other != this)
    {
        mTypeTag = other.mTypeTag;
        mCreateFunc = other.mCreateFunc;
        strncpy_s(mDescription, other.mDescription, AppWindowManager::APPWINDOW_DESC_LENGTH);
        mDescription[AppWindowManager::APPWINDOW_DESC_LENGTH - 1] = '\0';
        strncpy_s(mClassName, other.mClassName, AppWindowManager::APPWINDOW_CLASS_LENGTH);
        mDescription[AppWindowManager::APPWINDOW_CLASS_LENGTH - 1] = '\0';
    }

    return *this;
}

//----------------------------------------------------------------------------------------

void TypeTableEntry::Clear()
{
    mTypeTag = WINDOW_TYPE_INVALID;
    mDescription[0] = '\0';
    mClassName[0] = '\0';
    mCreateFunc = nullptr;
}

//----------------------------------------------------------------------------------------

TypeTable::TypeTable(unsigned int max, Alloc::IAllocator* alloc)
    : mAllocator(alloc), mCurrentSize(0), mMaxSize(max), mMainWindowIndex(-1)
#if PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    , mSecondaryWindowIndex(-1)
#endif
{
    mTable = PG_NEW_ARRAY(mAllocator, -1, "TypeTableEntries", Pegasus::Alloc::PG_MEM_PERM, TypeTableEntry, mMaxSize);
}

//----------------------------------------------------------------------------------------

TypeTable::~TypeTable()
{
    PG_ASSERTSTR(mCurrentSize == 0, "Type table not empty upon destruction!");

    PG_DELETE_ARRAY(mAllocator, mTable);
}

//----------------------------------------------------------------------------------------

bool TypeTable::Contains(const char* typeName) const
{
    bool found = false;

    // Iterate over table
    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (Pegasus::Utils::Strcmp(typeName, mTable[i].mClassName) == 0)
        {
            found = true;
            break;
        }
    }

    return found;
}

//----------------------------------------------------------------------------------------

bool TypeTable::Contains(WindowTypeTag typeTag) const
{
    bool found = false;

    // Iterate over table
    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (typeTag == mTable[i].mTypeTag)
        {
            found = true;
            break;
        }
    }

    return found;
}

//----------------------------------------------------------------------------------------

TypeTableEntry* TypeTable::Get(const char* typeName) const
{
    int index = -1;

    // Iterate over table
    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (Pegasus::Utils::Strcmp(typeName, mTable[i].mClassName) == 0)
        {
            index = i;
            break;
        }
    }

    // Return suitable entry
    if (index < 0)
    {
        return nullptr;
    }
    else
    {
        return (mTable + index);
    }
}

//----------------------------------------------------------------------------------------

void TypeTable::Insert(const TypeTableEntry& entry)
{
    PG_ASSERTSTR(!Contains(entry.mClassName), "Type table already contains this class type!");
    PG_ASSERTSTR(mCurrentSize < mMaxSize, "Type table is full!");

    if (entry.mTypeTag == WINDOW_TYPE_MAIN)
    {
        PG_ASSERTSTR(!Contains(WINDOW_TYPE_MAIN), "Type table already contains a MAIN window!");

        mMainWindowIndex = (int) mCurrentSize;
    }
#if PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    else if (entry.mTypeTag == WINDOW_TYPE_SECONDARY)
    {
        PG_ASSERTSTR(!Contains(WINDOW_TYPE_SECONDARY), "Type table already contains a SECONDARY window!");

        mSecondaryWindowIndex = (int) mCurrentSize;
    }
#endif  // PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    mTable[mCurrentSize++] = entry;
}

//----------------------------------------------------------------------------------------

void TypeTable::Remove(const char* typeName)
{
    int index = -1;
    mMainWindowIndex = -1;

#if PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    mSecondaryWindowIndex = -1;
#endif  // PEGASUS_ENABLE_EDITOR_WINDOW_TYPES

    // Iterate over table
    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (Pegasus::Utils::Strcmp(typeName, mTable[i].mClassName) == 0)
        {
            index = i;
            Pegasus::Utils::Memcpy(mTable + index, mTable + index + 1, sizeof(TypeTableEntry) * (mCurrentSize - index - 1)); // Fill hole
            mCurrentSize--;
            break;            
        }
    }

    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (mTable[i].mTypeTag == WINDOW_TYPE_MAIN)
        {
            mMainWindowIndex = i;
        }

    #if PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
        // Check for SECONDARY
        if (mTable[i].mTypeTag == WINDOW_TYPE_SECONDARY)
        {
            mSecondaryWindowIndex = i;
        }
    #endif  // PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    }
    
    PG_ASSERTSTR(index != -1, "Trying to remove unknown window type!");
}

//----------------------------------------------------------------------------------------

WindowTable::WindowTable(unsigned int max, Alloc::IAllocator* alloc)
: mAllocator(alloc), mCurrentSize(0), mMaxSize(max)
{
    mTable = PG_NEW_ARRAY(mAllocator, -1, "WindowTableEntries", Pegasus::Alloc::PG_MEM_PERM, Wnd::Window*, mMaxSize);

    // Zero out window table
    for (unsigned int i = 0; i < mMaxSize; i++)
    {
        mTable[i] = nullptr;
    }
}

//----------------------------------------------------------------------------------------

WindowTable::~WindowTable()
{
    PG_ASSERTSTR(mCurrentSize == 0, "Window table not empty upon destruction!");

    PG_DELETE_ARRAY(mAllocator, mTable);
}

//----------------------------------------------------------------------------------------

bool WindowTable::Contains(const Wnd::Window* entry) const
{
    bool found = false;

    // Iterate over table
    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (entry == mTable[i])
        {
            found = true;
            break;
        }
    }

    return found;
}

//----------------------------------------------------------------------------------------

void WindowTable::Insert(Wnd::Window* entry)
{
    PG_ASSERTSTR(mCurrentSize < mMaxSize, "Window table is full!");

    mTable[mCurrentSize++] = entry;
}

//----------------------------------------------------------------------------------------

void WindowTable::Remove(Wnd::Window* entry)
{
    int index = -1;

    // Iterate over table
    for (unsigned int i = 0; i < mCurrentSize; i++)
    {
        if (entry == mTable[i])
        {
            index = i;
            break;
        }
    }

    PG_ASSERTSTR(index != -1, "Trying to remove unknown window!");
    PG_DELETE(mAllocator, mTable[index]);
    Pegasus::Utils::Memcpy(mTable + index, mTable + index + 1, sizeof(Wnd::Window*) * (mCurrentSize - index - 1)); // Fill hole
    mCurrentSize--;
}

//----------------------------------------------------------------------------------------

AppWindowManager::AppWindowManager(const AppWindowManagerConfig& config)
    : mAllocator(config.mAllocator)
{
    // 1 is added to the number of window types, as the startup window class is used internally
    // and is not supposed to be known from the application
    mTypeTable = PG_NEW(mAllocator, -1, "WindowManager-TypeTable", Pegasus::Alloc::PG_MEM_PERM) TypeTable(config.mMaxWindowTypes + 1, mAllocator);
    mWindowTable = PG_NEW(mAllocator, -1, "WindowManager-TypeTable", Pegasus::Alloc::PG_MEM_PERM) WindowTable(config.mMaxNumWindows, mAllocator);
}

//----------------------------------------------------------------------------------------

AppWindowManager::~AppWindowManager()
{
    // Clean up tables
    PG_DELETE(mAllocator, mWindowTable);
    PG_DELETE(mAllocator, mTypeTable);
}

//----------------------------------------------------------------------------------------

void AppWindowManager::RegisterWindowClass(const char* className, const WindowRegistration& classReg)
{
    TypeTableEntry entry;

    // Set up and insert
    entry.mTypeTag = classReg.mTypeTag;
    entry.mCreateFunc = classReg.mCreateFunc;
    strncpy_s(entry.mDescription, classReg.mDescription, AppWindowManager::APPWINDOW_DESC_LENGTH);
    entry.mDescription[AppWindowManager::APPWINDOW_DESC_LENGTH - 1] = '\0';
    strncpy_s(entry.mClassName, className, AppWindowManager::APPWINDOW_CLASS_LENGTH);
    entry.mClassName[AppWindowManager::APPWINDOW_CLASS_LENGTH - 1] = '\0';

    mTypeTable->Insert(entry);
}

//----------------------------------------------------------------------------------------

void AppWindowManager::UnregisterWindowClass(const char* typeName)
{
    mTypeTable->Remove(typeName);
}

//----------------------------------------------------------------------------------------

const char* AppWindowManager::GetMainWindowType() const
{
    if (mTypeTable->mMainWindowIndex != -1)
    {
        return mTypeTable->mTable[mTypeTable->mMainWindowIndex].mClassName;
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------

#if PEGASUS_ENABLE_EDITOR_WINDOW_TYPES
    
const char* AppWindowManager::GetSecondaryWindowType() const
{
    if (mTypeTable->mSecondaryWindowIndex != -1)
    {
        return mTypeTable->mTable[mTypeTable->mSecondaryWindowIndex].mClassName;
    }

    return nullptr;
}

#endif  // PEGASUS_ENABLE_EDITOR_WINDOW_TYPES

//----------------------------------------------------------------------------------------

Wnd::Window* AppWindowManager::CreateNewWindow(const char* typeName, const Wnd::WindowConfig& config)
{
    TypeTableEntry* entry = nullptr;
    Wnd::Window* ret = nullptr;

    // Find an entry and use it
    entry = mTypeTable->Get(typeName);
    PG_ASSERTSTR(entry != nullptr, "Trying to create window of unknown type!")
    ret = entry->mCreateFunc(config, mAllocator);

    // Add it to the window table
    mWindowTable->Insert(ret);

    // The main view window in the editor, or the main app window when launched alone, requires
    //special events to be executed
    if (entry->mTypeTag == WINDOW_TYPE_MAIN )       
    {
        ret->HandleMainWindowEvents();
    }

    return ret;
}

//----------------------------------------------------------------------------------------

void AppWindowManager::DestroyWindow(Wnd::Window* window)
{
    // Remove it from the window table
    mWindowTable->Remove(window);
}

}   // namespace App
}   // namespace Pegasus
