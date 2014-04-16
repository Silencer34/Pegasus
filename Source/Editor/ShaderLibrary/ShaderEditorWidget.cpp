/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file   ShaderEditorWidget.cpp	
//! \author	Kleber Garcia
//! \date	30th Match 2014
//! \brief	Shader Editor IDE

#include "Editor.h"
#include "Log.h"
#include "Assertion.h"
#include "Settings/Settings.h"
#include "ShaderLibrary/ShaderEditorWidget.h"
#include "Pegasus/Shader/Shared/IShaderProxy.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTextEdit>
#include <QColor>
#include <QSyntaxHighlighter>
#include <QRegExp>
#include <QTextCharFormat>

//! internal class serving as the syntax highlighter for shade code
class ShaderSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    ShaderSyntaxHighlighter(QTextDocument * parent)
    : QSyntaxHighlighter(parent)
    {
       
        static const char  * keywords[] = {
            "\\b[0-9]*\\.?[0-9]+f?\\b",
            "\\bsin\\b",
            "\\bcos\\b",
            "\\bmix\\b",
            "\\bdot\\b",
            "\\bnormalize\\b",
            "\\bdistance\\b",
            "\\bvoid\\b",
            "\\bfloat\\b",
            "\\b[biud]?vec[1-4]?\\b",
            "\\bbool\\b",
            "\\bint\\b",
            "\\buint\\b",
            "\\bfloat\\b",
            "\\bdouble\\b",
            "\\bmat[1-4]x?[1-4]?\\b",
            "\\buniform\\b",
            "\\bvarying\\b",
            "\\battribute\\b",
            "\\bstruct\\b",
            "\\bout\\b",
            "\\blayout\\b",
            "\\bin\\b",
            "\\bpow\\b",
            nullptr
        };
        const char * ptr = keywords[0];
        int i = 0;
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(130,225,50,255)); 
        while (ptr != nullptr)
        {
            QRegExp keywordExp(ptr);
            mRules << keywordExp;
            ptr = keywords[++i]; 
        }
    }

    virtual ~ShaderSyntaxHighlighter() {}

    
protected:

    QVector<QRegExp> mRules;

    //! sets the formats for comments
    void SetCCommentStyle(int start, int end, Settings * settings)
    {
        setFormat(start, end, settings->GetShaderSyntaxColor(Settings::SYNTAX_C_COMMENT));
    }

    //! sets the formats for CPP coments
    void SetCPPCommentStyle(int start, int end, Settings * settings)
    {
         setFormat(start, end, settings->GetShaderSyntaxColor(Settings::SYNTAX_CPP_COMMENT));
    }

    //! sets the formats for regular style
    void SetNormalStyle(int start, int end, Settings * settings)
    {
        setFormat(start, end, settings->GetShaderSyntaxColor(Settings::SYNTAX_NORMAL_TEXT));
    }

    //! qt callback that processes highlighting in a line of text
    virtual void highlightBlock(const QString& text)
    {
        //TODO: Because the settings has a direct dependency on the widget dock, 
        //      we must request the settings singleton every time we highlight a line.
        //      this is a lot of memory dereference :/ we could optimize this if we cache
        //      the settings as a member of this class
        Settings * settings = Editor::GetInstance().GetSettings();
        // state machine definitions
        const int NormalState = -1;
        const int CommentState = 0;
        int state = previousBlockState();
        int start = 0;
        bool isCommentLine = false;

        // for every character
        for (int i = 0; i < text.length(); ++i)
        {
            if (state == CommentState)
            {
                //close the comment
                if (text.mid(i, 2) == "*/")
                {
                    state = NormalState;
                    SetCPPCommentStyle(start, i - start + 2, settings);
                    start = i + 2;
                }
            }
            else
            {
                // trigger a full line comment
                if (text.mid(i, 2) == "//")
                {
                    isCommentLine = true;
                    SetCCommentStyle(i, text.length() - i, settings);
                    start = text.length();
                    break;
                }
                // trigger a comment block
                else if (text.mid(i, 2) == "/*")
                {
                    start = i;
                    state = CommentState; 
                }
            }
        }

        if (state == CommentState && start < text.length())
        {
            SetCPPCommentStyle(start, text.length() - start, settings);
        }

        if (state != CommentState && !isCommentLine)
        {
            for (int i = 0; i < mRules.size(); ++i)
            {
                QRegExp& pattern = mRules[i];
                int index = pattern.indexIn(text);
                while (index >= 0)
                {
                    int length = pattern.matchedLength();
                    Settings::ShaderEditorSyntaxStyle style = i == 0 ? Settings::SYNTAX_NUMBER_VALUE : Settings::SYNTAX_KEYWORD;
                    setFormat(index, length, settings->GetShaderSyntaxColor(style));
                    index = pattern.indexIn(text, index + length);
                }
            }
        }
        setCurrentBlockState(state);
    }
    
};

ShaderEditorWidget::ShaderEditorWidget (QWidget * parent)
: QWidget(parent), mTabCount(0)
{
    SetupUi();
}

ShaderEditorWidget::~ShaderEditorWidget()
{
}

void ShaderEditorWidget::SetupUi()
{
    resize(550, 700);
    setWindowTitle(QString("Shader Editor"));
    mUi.mMainLayout = new QVBoxLayout();
    mUi.mTabWidget = new QTabWidget(this);
    mUi.mTabWidget->setTabsClosable(true);
    
    connect(
        mUi.mTabWidget, SIGNAL(tabCloseRequested(int)),
        this, SLOT(RequestClose(int))
    );
    
    mUi.mMainLayout->addWidget(mUi.mTabWidget);
    
    for (int i = 0; i < MAX_TEXT_TABS; ++i)
    {
        mUi.mWidgetPool[i] = new QWidget();
        mUi.mTextEditPool[i] = new QTextEdit();

        mUi.mTextEditPool[i]->setFontFamily(QString("Courier"));
        mUi.mSyntaxHighlighterPool[i] = new ShaderSyntaxHighlighter(mUi.mTextEditPool[i]->document());
    
        QHBoxLayout * horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addWidget(mUi.mTextEditPool[i]);
        mUi.mWidgetPool[i]->setLayout(horizontalLayout);

    }

    setLayout(mUi.mMainLayout);
}

int ShaderEditorWidget::FindIndex(Pegasus::Shader::IShaderProxy * target)
{
    for (int i = 0; i < mTabCount; ++i)
    {
        if (target == mShaderProxyPool[i])
        {
            return i;
        }
    }
    return -1;
}

void ShaderEditorWidget::OnSettingsChanged()
{
    for (int i = 0; i < mTabCount; ++i)
    {
        mUi.mSyntaxHighlighterPool[i]->rehighlight();
        QPalette p = mUi.mTextEditPool[i]->palette();
        p.setColor(QPalette::Base, Editor::GetInstance().GetSettings()->GetShaderSyntaxColor(Settings::SYNTAX_BACKGROUND));
        p.setColor(QPalette::Text, Editor::GetInstance().GetSettings()->GetShaderSyntaxColor(Settings::SYNTAX_NORMAL_TEXT));
        mUi.mTextEditPool[i]->setPalette(p);
    }
}

void ShaderEditorWidget::RequestClose(int index)
{
    //remove tab
    mUi.mTabWidget->removeTab(index);

    //update tab indices
    QWidget * tempW = mUi.mWidgetPool[index];
    QTextEdit * tempTextEdit = mUi.mTextEditPool[index];
    QSyntaxHighlighter * mtempSyntax = mUi.mSyntaxHighlighterPool[index];

    //copy the pointers back
    for (int i = index; i < MAX_TEXT_TABS - 1; ++i)
    {
        mUi.mWidgetPool[i]  = mUi.mWidgetPool[i + 1];
        mUi.mTextEditPool[i]  = mUi.mTextEditPool[i + 1];
        mUi.mSyntaxHighlighterPool[i]  = mUi.mSyntaxHighlighterPool[i + 1];
        mShaderProxyPool[i] = mShaderProxyPool[i + 1];
    }

    //send deleted pointer to the back of the buffer
    mUi.mWidgetPool[MAX_TEXT_TABS - 1] = tempW;
    mUi.mTextEditPool[MAX_TEXT_TABS - 1] = tempTextEdit;
    mUi.mSyntaxHighlighterPool[MAX_TEXT_TABS - 1] = mtempSyntax;

    --mTabCount;
}

void ShaderEditorWidget::RequestOpen(Pegasus::Shader::IShaderProxy * shaderProxy)
{
    if (mTabCount >= MAX_TEXT_TABS)
    {
        //TODO - error, too many text tabs open, throw a signal or something
    }
    else
    {
        int currentTabIndex = FindIndex(shaderProxy);
        if (currentTabIndex == -1) //is this opened already in the pool?
        {
            mShaderProxyPool[mTabCount] = shaderProxy;
            mUi.mTabWidget->addTab(mUi.mWidgetPool[mTabCount], QString(shaderProxy->GetName()));
            currentTabIndex = mUi.mTabWidget->count() - 1;            
            const char * srcChar = nullptr;
            int srcSize = 0;
            shaderProxy->GetSource(&srcChar, srcSize);
            QChar * qchar = new QChar[srcSize];
            for (int i = 0; i < srcSize; ++i)
            {
                qchar[i] = srcChar[i];
            }
            QString srcQString(qchar, srcSize);
            mUi.mTextEditPool[currentTabIndex]->setText(srcQString);
    
            QPalette p = mUi.mTextEditPool[currentTabIndex]->palette();
            p.setColor(QPalette::Base, Editor::GetInstance().GetSettings()->GetShaderSyntaxColor(Settings::SYNTAX_BACKGROUND));
            p.setColor(QPalette::Text, Editor::GetInstance().GetSettings()->GetShaderSyntaxColor(Settings::SYNTAX_NORMAL_TEXT));
            mUi.mTextEditPool[currentTabIndex]->setPalette(p);
        
            delete[] qchar;
            mTabCount++;
        }
        mUi.mTabWidget->setCurrentIndex(currentTabIndex); 
    }
}

