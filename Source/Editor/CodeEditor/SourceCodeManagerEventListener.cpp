/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   SourceCodeManagerEventListener.cpp
//! \author Kleber Garcia
//! \date   4rth April 2014
//! \brief  Pegasus Source Code Manager Event Listener	
#include "CodeEditor/SourceCodeManagerEventListener.h"
#include "Pegasus/Shader/Shared/IProgramProxy.h"
#include <QTextDocument>
#include <string.h>

CodeUserData::CodeUserData(Pegasus::Core::ISourceCodeProxy * code)
: mIsValid(true), mIntermediateDocument(nullptr)
{
    mData.mSourceCode = code;
    mIsProgram = false;
}

CodeUserData::CodeUserData(Pegasus::Shader::IProgramProxy * program)
: mIsValid(true)
{
    mData.mProgram = program;
    mIsProgram = true;
}

SourceCodeManagerEventListener::SourceCodeManagerEventListener ()
    : QObject(nullptr)
{
}

SourceCodeManagerEventListener::~SourceCodeManagerEventListener()
{
}

void SourceCodeManagerEventListener::OnEvent(Pegasus::Core::IEventUserData * userData, Pegasus::Core::CompilerEvents::CompilationEvent& e)
{
    if (userData != nullptr)
    {
        CodeUserData * codeUserData = static_cast<CodeUserData*>(userData);
        bool previousVal = codeUserData->IsValid();
        codeUserData->SetIsValid(e.IsSuccess());
        emit( OnCompilationEnd(e.GetLogString()) );
        if (previousVal != e.IsSuccess())
        {
            emit(CompilationResultsChanged());
        }
    }
}

void SourceCodeManagerEventListener::OnEvent(Pegasus::Core::IEventUserData * userData, Pegasus::Core::CompilerEvents::LinkingEvent& e)
{
    if (userData != nullptr)
    {
        CodeUserData * programUserData = static_cast<CodeUserData*>(userData); 
        ED_ASSERT(programUserData->IsProgram());
        bool previousIsValid = programUserData->IsValid();
        bool isSuccess = Pegasus::Core::CompilerEvents::LinkingEvent::LINKING_SUCCESS == e.GetEventType();
        programUserData->SetIsValid(isSuccess);
        programUserData->SetErrorMessage(e.GetLog());
        emit(OnLinkingEvent(e.GetLog(), e.GetEventType()));
        if (previousIsValid != isSuccess)
        {
            emit(CompilationResultsChanged());
        }
    }
}

void SourceCodeManagerEventListener::OnInitUserData(Pegasus::Core::IBasicSourceProxy* proxy, const char* name)
{
    QString strName = name;
    if (strName == "ProgramLinkage")
    {
        Pegasus::Shader::IProgramProxy* programProxy = static_cast<Pegasus::Shader::IProgramProxy*>(proxy);
        CodeUserData* newUserData = new CodeUserData(programProxy);
        newUserData->SetName(strName);
        programProxy->SetUserData(newUserData);
    }
    else
    {
        CodeUserData* newUserData = new CodeUserData(static_cast<Pegasus::Core::ISourceCodeProxy*>(proxy));
        newUserData->SetName(strName);
        proxy->SetUserData(newUserData);
        emit(OnBlessUserData(newUserData));
    }
}

void SourceCodeManagerEventListener::OnDestroyUserData(Pegasus::Core::IBasicSourceProxy* proxy, const char* name)
{
    //not yet!
    ED_ASSERT(proxy->GetUserData() != nullptr);
    CodeUserData* userData = static_cast<CodeUserData*>(proxy->GetUserData());
    userData->ClearData();
    QString strName = name;
    if (strName != "ProgramLinkage")
    {
        emit(OnUnblessUserData(userData));
    }
}

void SourceCodeManagerEventListener::SafeDestroyUserData(CodeUserData* codeUserData)
{
    ED_ASSERT(codeUserData != nullptr);
    delete codeUserData;
}

void SourceCodeManagerEventListener::OnEvent(Pegasus::Core::IEventUserData * userData, Pegasus::Core::CompilerEvents::SourceLoadedEvent& e)
{
}

void SourceCodeManagerEventListener::OnEvent(Pegasus::Core::IEventUserData * userData, Pegasus::Core::CompilerEvents::CompilationNotification& e)
{
    if (userData != nullptr)
    {
        CodeUserData * codeUserData = static_cast<CodeUserData*>(userData);
        if (e.GetType() == Pegasus::Core::CompilerEvents::CompilationNotification::COMPILATION_BEGIN)
        {
            emit( OnCompilationBegin(codeUserData) ) ;
        }
        else
        {        
            emit(
                OnCompilationError(
                    codeUserData,
                    e.GetRow(),
                    QString(e.GetDescription())
                )
             );
        } 
    }
}
