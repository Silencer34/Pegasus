/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file   CodeEditorWidget.cpp	
//! \author	Kleber Garcia
//! \date	30th Match 2014
//! \brief	Code Editor IDE

#include "Editor.h"
#include "Log.h"
#include "Assertion.h"
#include "Settings/Settings.h"
#include "CodeEditor/CodeEditorWidget.h"
#include "CodeEditor/CodeTextEditorWidget.h"
#include "CodeEditor/SourceCodeManagerEventListener.h"
#include "CodeEditor/CodeTextEditorTreeWidget.h"
#include "Pegasus/Core/Shared/ISourceCodeProxy.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QToolBar>
#include <QColor>
#include <QSyntaxHighlighter>
#include <QPushButton>
#include <QRegExp>
#include <QTextCharFormat>
#include <QFontMetrics>
#include <QSignalMapper>
#include <QMutex>
#include <QSet>
#include <QAction>
#include <QStatusBar>

static const char * DOCKABLE_DESC = "Dockable: Allow to be docked when hovering over main window.";
static const char * UNDOCKABLE_DESC = "Undockable: Allow to hover over window w/o docking.";
static const char * INSTANT_COMPILATION_DESC_ON = "Turn instant compilation ON\nInstant Compilation Mode: code gets compiled at every key stroke, in an efficient manner. ";
static const char * INSTANT_COMPILATION_DESC_OFF = "Turn instant compilation OFF\nInstant Compilation Mode: code gets compiled at every key stroke, in an efficient manner. ";

CodeEditorWidget::CodeEditorWidget (QWidget * parent)
: 
    QDockWidget(parent),
    mCompilationRequestPending(false), 
    mInternalBlockTextUpdated(false),
    mOpenCodeCount(0),
    mPreviousTabIndex(-1),
    mCloseViewAction(nullptr),
    mPinAction(nullptr),
    mSaveAction(nullptr),
    mHorizontalAction(nullptr),
    mVerticalAction(nullptr),
    mCompilationRequestMutex(nullptr),
    mInstantCompilationFlag(true), //on by default
    mSavedInstantCompilationFlag(true),
    mCompilationPolicy(Pegasus::Core::ISourceCodeProxy::POLICY_USER_DEFINED)
{
    mCompilationRequestMutex = new QMutex();
    SetupUi();
}

CodeEditorWidget::~CodeEditorWidget()
{
    delete mCompilationRequestMutex;
}

void CodeEditorWidget::SetupUi()
{
    QWidget * mainWidget = new QWidget(this);
    mUi.mMainLayout = new QVBoxLayout();
    mainWidget->setLayout(mUi.mMainLayout);
    setWidget(mainWidget);

	setFeatures(  QDockWidget::DockWidgetClosable
				| QDockWidget::DockWidgetMovable
				| QDockWidget::DockWidgetFloatable);

    setAllowedAreas(Qt::NoDockWidgetArea);

    QToolBar * toolBar = new QToolBar(mainWidget);
    //TODO hook up these actions
    QIcon saveIcon(tr(":/CodeEditor/save.png"));
    QIcon singleIcon(tr(":/CodeEditor/single.png"));
    QIcon verticalIcon(tr(":/CodeEditor/vertical.png"));
    QIcon horizontalIcon(tr(":/CodeEditor/horizontal.png"));
    QIcon compileIcon(tr(":/CodeEditor/compile.png"));

    mSaveAction = toolBar->addAction(saveIcon, tr("save this source to its file"));

    mCloseViewAction  = toolBar->addAction(singleIcon, tr("close current view"));
    mVerticalAction   = toolBar->addAction(verticalIcon, tr("split views vertically"));
    mHorizontalAction = toolBar->addAction(horizontalIcon, tr("split views horizontally"));
    mCompileAction    = toolBar->addAction(compileIcon, tr("compile current code (Ctrl + F7)"));
    mCompileAction->setShortcut(tr("Ctrl+F7"));

    toolBar->addSeparator();

    mPinIcon.addFile(tr(":/CodeEditor/pin.png"));
    mUnpinIcon.addFile(tr(":/CodeEditor/unpin.png"));
    mPinAction = toolBar->addAction(mUnpinIcon, tr(UNDOCKABLE_DESC));   

    mOnInstantCompilationIcon.addFile(tr(":/Toolbar/Edit/switchon.png"));
    mOffInstantCompilationIcon.addFile(tr(":/Toolbar/Edit/switchoff.png"));

    mInstantCompilationAction = toolBar->addAction(mOnInstantCompilationIcon, INSTANT_COMPILATION_DESC_OFF);
    toolBar->setIconSize(QSize(16,16));

    // connect toolbar actions
    connect(mPinAction, SIGNAL(triggered(bool)),
            this, SLOT(SignalPinActionTriggered()));

    connect(mInstantCompilationAction, SIGNAL(triggered(bool)),
            this, SLOT(SignalInstantCompilationActionTriggered()));
    

    mSaveAction->setShortcut(tr("Ctrl+S"));
    connect(mSaveAction, SIGNAL(triggered(bool)),
            this, SLOT(SignalSaveCurrentCode()));

    resize(550, 700);
    setWindowTitle(tr("Code Editor"));
    setObjectName("CodeEditor");

    mUi.mTabWidget = new QTabBar(mainWidget);
    mUi.mTabWidget->setTabsClosable(true);
    
    connect(
        mUi.mTabWidget, SIGNAL(tabCloseRequested(int)),
        this, SLOT(RequestClose(int))
    );
 
    //setup the tree editor
    QSignalMapper * textChangedMapper = new QSignalMapper(this);
    QSignalMapper * selectedSignalMapper = new QSignalMapper(this);
    mUi.mTextEditorSignals.Initialize(
        textChangedMapper,
        selectedSignalMapper
    );

    mUi.mTreeEditor = new CodeTextEditorTreeWidget(mUi.mTextEditorSignals, mainWidget);

    connect(
        textChangedMapper, SIGNAL(mapped(QWidget *)),
        this, SLOT(OnTextChanged(QWidget *))
    );

    connect(
        selectedSignalMapper, SIGNAL(mapped(QWidget *)),
        this, SLOT(OnTextWindowSelected(QWidget*))
    );

    
    // QT's definition of horizontal is different from the user... thats why we flip the
    // the callbacks
    connect(
        mCloseViewAction, SIGNAL(triggered(bool)),
        mUi.mTreeEditor, SLOT(CloseSplit())
    );

    connect(
        mVerticalAction, SIGNAL(triggered(bool)),
        mUi.mTreeEditor, SLOT(HorizontalSplit())
    );

    connect(
        mHorizontalAction, SIGNAL(triggered(bool)),
        mUi.mTreeEditor, SLOT(VerticalSplit())
    );

    connect(
        mCompileAction, SIGNAL(triggered(bool)),
        this, SLOT(SignalCompileCurrentCode())
    );

    connect(
        mUi.mTabWidget, SIGNAL(currentChanged(int)),
        this,   SLOT(SignalViewCode(int))
    );
    //setup the status bar
    mUi.mStatusBar = new QStatusBar(mainWidget);
    
    mUi.mMainLayout->addWidget(toolBar);
    mUi.mMainLayout->addWidget(mUi.mTabWidget);
    mUi.mMainLayout->addWidget(mUi.mTreeEditor);    
    mUi.mMainLayout->addWidget(mUi.mStatusBar);

}

bool CodeEditorWidget::AsyncHasCompilationRequestPending()
{
    bool tmp;
    mCompilationRequestMutex->lock();
    tmp = mCompilationRequestPending;
    mCompilationRequestMutex->unlock();
    return tmp;
}


void CodeEditorWidget::AsyncSetCompilationRequestPending()
{
    mCompilationRequestMutex->lock();
    mCompilationRequestPending = true;
    mCompilationRequestMutex->unlock();
}

void CodeEditorWidget::FlushTextEditorToCode(int id)
{
    mCompilationRequestMutex->lock();
    if (id >= 0 && id < mOpenCodeCount) //is there a ui element for this code?
    {
        CodeTextEditorWidget * editor = mUi.mTreeEditor->FindCodeInEditors(mOpenedCodes[id]);
        if (editor != nullptr)
        {
            editor->FlushTextToCode();
        }

    }
    mCompilationRequestPending = false;
    mCompilationRequestMutex->unlock();
}

void CodeEditorWidget::SignalCompileCurrentCode()
{
    int idx = mUi.mTabWidget->currentIndex();
    if (idx >= 0 && idx < mOpenCodeCount)
    {
        Pegasus::Core::ISourceCodeProxy * code = mOpenedCodes[idx];
        if (code != nullptr)
        {
            //ensure that no compilation is happening while saving this file
            CompileCode(idx);
        }
    }
}

void CodeEditorWidget::SignalCompilationError(void * srcPtr, int line, QString errorString)
{
    Pegasus::Core::ISourceCodeProxy* target = static_cast<Pegasus::Core::ISourceCodeProxy*>(srcPtr);
    CodeUserData * userData = static_cast<CodeUserData*>(target->GetUserData());
    if (userData != nullptr)
    {
        userData->InvalidateLine(line);
        userData->InsertMessage(line, errorString);
        PostStatusBarMessage(errorString);
        CodeTextEditorWidget * editor = mUi.mTreeEditor->FindCodeInEditors(target);
        if (editor != nullptr)
        {
            UpdateSyntaxForLine(editor, line);
        }
    }
}

void CodeEditorWidget::SignalLinkingEvent(QString message, int eventType)
{
    if (static_cast<Pegasus::Core::CompilerEvents::LinkingEvent::Type>(eventType) != Pegasus::Core::CompilerEvents::LinkingEvent::LINKING_SUCCESS)
    {
        PostStatusBarMessage(message);
    }
}

void CodeEditorWidget::SignalCompilationBegin(void* code)
{
    Pegasus::Core::ISourceCodeProxy* target = static_cast<Pegasus::Core::ISourceCodeProxy*>(code);
    CodeTextEditorWidget * editor = mUi.mTreeEditor->FindCodeInEditors(target);
    if (editor != nullptr && target->GetUserData() != nullptr)
    {
        PostStatusBarMessage("");
        CodeUserData * codeUserData = static_cast<CodeUserData*>(target->GetUserData());
        ED_ASSERT(!codeUserData->IsProgram());
        QSet<int> lineSetCopy = codeUserData->GetInvalidLineSet();
        codeUserData->ClearInvalidLines();
        for (int line : lineSetCopy)
        {
            UpdateSyntaxForLine(editor, line);
        }
    }
}

void CodeEditorWidget::SignalCompilationEnd(QString log)
{
    //update the status bar with whichever compilation error.
    if (log != "")
    {
        PostStatusBarMessage(log);
    }
    else
    {
         PostStatusBarMessage("File Compiled Successfuly.");
    }
}

void CodeEditorWidget::SignalPinActionTriggered()
{
    ED_ASSERT(mPinAction != nullptr);
    if (allowedAreas() == Qt::NoDockWidgetArea)
    {
        setAllowedAreas(Qt::AllDockWidgetAreas);
        mPinAction->setIcon(mPinIcon);
        mPinAction->setText(tr(DOCKABLE_DESC));
    }
    else if (allowedAreas() == Qt::AllDockWidgetAreas)
    {
        setAllowedAreas(Qt::NoDockWidgetArea);
        mPinAction->setIcon(mUnpinIcon);
        mPinAction->setText(tr(UNDOCKABLE_DESC));
    } 
}

void CodeEditorWidget::SetInstantCompilationState(bool state)
{
    if (state)
    {
        mInstantCompilationAction->setIcon(mOnInstantCompilationIcon);
        mInstantCompilationAction->setToolTip(tr(INSTANT_COMPILATION_DESC_OFF));
    }
    else
    {   
        mInstantCompilationAction->setIcon(mOffInstantCompilationIcon);
        mInstantCompilationAction->setToolTip(tr(INSTANT_COMPILATION_DESC_ON));
    }
    mInstantCompilationFlag = state;
}

void CodeEditorWidget::EnableModeInstantCompilationButton(bool enableMode)
{
    mInstantCompilationAction->setDisabled(!enableMode);
}

void CodeEditorWidget::SignalInstantCompilationActionTriggered()
{
    SetInstantCompilationState(!mInstantCompilationFlag);
}

void CodeEditorWidget::SignalSaveCurrentCode()
{
    PostStatusBarMessage("");
    int idx = mUi.mTabWidget->currentIndex();
    if (idx >= 0 && idx < mOpenCodeCount)
    {
        Pegasus::Core::ISourceCodeProxy * code = mOpenedCodes[idx];
        if (code != nullptr)
        {
            //ensure that no compilation is happening while saving this file
            mCompilationRequestMutex->lock();
            code->SaveSourceToFile();
            mCompilationRequestMutex->unlock();
        }
    }
}

void CodeEditorWidget::SignalSavedFileSuccess()
{
    PostStatusBarMessage(tr("File Saved."));
}

void CodeEditorWidget::SignalViewCode(int tabId)
{
    if (tabId != -1)
    {
        ED_ASSERT(tabId >= 0 && tabId < mOpenCodeCount);

        CodeTextEditorWidget * editor = nullptr;

        if (mPreviousTabIndex >= 0 && mPreviousTabIndex < mOpenCodeCount)
        {
            editor = mUi.mTreeEditor->FindCodeInEditors(mOpenedCodes[mPreviousTabIndex]);
        }
        mInternalBlockTextUpdated = true;
        mUi.mTreeEditor->DisplayCode(mOpenedCodes[tabId], editor);
        mInternalBlockTextUpdated = false;

        UpdateInstantCompilationButton(mOpenedCodes[tabId]);

        mPreviousTabIndex = tabId;
    }
}

void CodeEditorWidget::SignalSavedFileIoError(int ioError, QString msg)
{
    QString barMsg = tr("IO ERROR: file not saved correctly: ") + msg;
    PostStatusBarMessage(barMsg);
}

void CodeEditorWidget::UpdateSyntaxForLine(CodeTextEditorWidget * editor, int line)
{
    mInternalBlockTextUpdated = true;
    editor->UpdateLineSyntax(line);
    mInternalBlockTextUpdated = false;
}

int CodeEditorWidget::FindIndex(Pegasus::Core::ISourceCodeProxy * target)
{
    for (int i = 0; i < mOpenCodeCount; ++i)
    {
        if (target == mOpenedCodes[i])
        {
            return i;
        }
    }
    return -1;
}

int CodeEditorWidget::FindIndex(CodeTextEditorWidget * target)
{
    Pegasus::Core::ISourceCodeProxy * code = target->GetCode();
    if (code != nullptr)
    {
        return FindIndex(code);
    }
    else
    {
        return -1;
    }
}

void CodeEditorWidget::PostStatusBarMessage(const QString& message)
{
    // do not opaque the previous error
    if (message == "" || mUi.mStatusBarMessage == "")
    {
        mUi.mStatusBarMessage = message;
        mUi.mStatusBar->showMessage(message);
    }
}

void CodeEditorWidget::OnSettingsChanged()
{
    mUi.mTreeEditor->ForceUpdateAllStyles();
}

void CodeEditorWidget::RequestClose(int index)
{
    ED_ASSERT(index >= 0 && index < mOpenCodeCount);
    //remove tab
    mInternalBlockTextUpdated = true;
    mUi.mTreeEditor->HideCode(mOpenedCodes[index]);
    mInternalBlockTextUpdated = false;

    //compress the opened code list
    for (int i = index; i < mOpenCodeCount - 1; ++i)
    {
        ED_ASSERT(i < MAX_OPENED_CODES);
        mOpenedCodes[i] = mOpenedCodes[i + 1];
        
    }
    --mOpenCodeCount;
    mUi.mTabWidget->removeTab(index);
}

void CodeEditorWidget::OnTextChanged(QWidget * sender)
{
    //do not trigger a text update event if the syntax highlighter rehighlights
    if (!mInstantCompilationFlag || mInternalBlockTextUpdated)
        return;

    CodeTextEditorWidget * textEditor = static_cast<CodeTextEditorWidget*>(sender);
    int id = FindIndex(textEditor);
    if (id != -1)
    {
        CompileCode(id);
    }
}

void CodeEditorWidget::OnTextWindowSelected(QWidget * sender)
{
    CodeTextEditorWidget * textEditor = static_cast<CodeTextEditorWidget*>(sender);
    if (textEditor != nullptr)
    {
        Pegasus::Core::ISourceCodeProxy * code = textEditor->GetCode();

        if (code != nullptr)
        {
            int i = FindIndex(code);
            if (i >= 0 && i < mUi.mTabWidget->count() && i != mUi.mTabWidget->currentIndex())
            {
                mUi.mTabWidget->setCurrentIndex(i);
            }
        }
    }
}

void CodeEditorWidget::CompileCode(int id)
{
    // The following snippet is meant to optimize this set of functions:
    //if (!AsyncHasCompilationRequestPending()) 
    //{
    //    AsyncSetCompilationRequestPending();
    //    emit(RequestCodeCompilation(id));
    //}
    mCompilationRequestMutex->lock(); 
    if (!mCompilationRequestPending)
    {
        mCompilationRequestPending = true;            
        mCompilationRequestMutex->unlock();
        emit(RequestCodeCompilation(id));
    }
    else
    {
        mCompilationRequestMutex->unlock();
    }
}

void CodeEditorWidget::UpdateInstantCompilationButton(Pegasus::Core::ISourceCodeProxy* proxy)
{
    if (mCompilationPolicy != proxy->GetCompilationPolicy())
    {
        mCompilationPolicy = proxy->GetCompilationPolicy();
        if (mCompilationPolicy == Pegasus::Core::ISourceCodeProxy::POLICY_USER_DEFINED)
        {
            if (!mInstantCompilationAction->isEnabled())
            {
                EnableModeInstantCompilationButton(true);
                SetInstantCompilationState(mSavedInstantCompilationFlag);
            }
        }
        else
        {
            ED_ASSERT(mCompilationPolicy == Pegasus::Core::ISourceCodeProxy::POLICY_FORCE_ON_SAVE);
            mSavedInstantCompilationFlag = mInstantCompilationFlag;
            SetInstantCompilationState(false);
            EnableModeInstantCompilationButton(false);
        }
    }

}

void CodeEditorWidget::RequestOpen(Pegasus::Core::ISourceCodeProxy * codeProxy)
{
    //dump code contents to document text
    //set the text of the current text editor
    const char * srcChar = nullptr;
    int srcSize = 0;
    codeProxy->GetSource(&srcChar, srcSize);
    QChar * qchar = new QChar[srcSize];
    for (int i = 0; i < srcSize; ++i)
    {
        qchar[i] = srcChar[i];
    }

    CodeUserData* codeUserData = static_cast<CodeUserData*>(codeProxy->GetUserData());
    QString srcQString(qchar, srcSize);
    codeUserData->GetDocument()->setPlainText(srcQString);
    delete[] qchar;            

    int currentTabIndex = FindIndex(codeProxy);
    if (currentTabIndex == -1)
    {
        if (mOpenCodeCount >= MAX_OPENED_CODES)
        {
            ED_LOG("Maximum number of opened codes reached!");
        }
        else
        {
            //try to find and focus the previous code selected.
            //this will make the code selected appear in the screen.
            currentTabIndex = mUi.mTabWidget->count();

            //  pattern must be appending to the end
            ED_ASSERT(currentTabIndex == mOpenCodeCount);
            mOpenedCodes[currentTabIndex] = codeProxy;
            ++mOpenCodeCount;

            //is this opened already in the pool?
            mUi.mTabWidget->addTab(QString(codeProxy->GetName()));
        }
    }
    mUi.mTabWidget->setCurrentIndex(currentTabIndex); 
    if (currentTabIndex == mUi.mTabWidget->currentIndex())
    {
        SignalViewCode(currentTabIndex);
    }
}

void CodeEditorWidget::UpdateUIForAppFinished()
{
    //compress the opened code list
    while (mOpenCodeCount > 0)
    {
        RequestClose(0);
    }
    mCompilationRequestPending = false; //kill any compilation request
}
