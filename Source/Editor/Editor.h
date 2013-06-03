#ifndef EDITOR_H
#define EDITOR_H

#include <QtWidgets/QMainWindow>

class ViewportDockWidget;
class TimelineDockWidget;

class QSplashScreen;
class QAction;


class Editor : public QMainWindow
{
    Q_OBJECT

public:
    Editor(QWidget *parent = 0);
    ~Editor();

	//------------------------------------------------------------------------------------

public:

    //! Raise the splash screen (put the window on top)
    void RaiseSplashScreen();

    //! Close the splash screen (if opened)
    void CloseSplashScreen();

    //! Get the application options object
    //! \return Pointer to the application options object
    //inline EditorOptions * GetOptions() { return mEditorOptions; }

	//------------------------------------------------------------------------------------

public:

    //! Get the viewport dock widget
    //! \return Pointer to the viewport dock widget
    inline ViewportDockWidget * GetViewportDockWidget() const { return mViewportDockWidget; }

    //! Get the timeline dock widget
    //! \return Pointer to the timeline dock widget
    inline TimelineDockWidget * GetTimelineDockWidget() const { return mTimelineDockWidget; }

    //------------------------------------------------------------------------------------

private:

    //! Create the set of QT actions needed by the main window
    void CreateActions();

    //! Create the menu of the main window
    void CreateMenu();

    //! Create the tool bars of the main window
    void CreateToolBars();

    //! Create the dock widgets of the main window
    void CreateDockWidgets();

    //! Create the status bar of the main window
    void CreateStatusBar();

    //------------------------------------------------------------------------------------

protected:

    // Overrides

    //! Function called when the main window is asked to be closed
    //! \param event Description of the event
    void closeEvent(QCloseEvent * event);
    
    //------------------------------------------------------------------------------------

private slots:

    //@{
    //! Slots for the actions of the File menu
    void NewScene();
    void OpenApp();
    void Quit();
    //@}

    //@{
    //! Slots for the actions of the Edit menu
    void OpenPreferences();
    //@}

    //@{
    //! Slots for the actions of the View menu
    void ShowFullscreenViewport();
    //@}

    //@{
    //! Slots for the actions of the Create menu
    void CreateBox();
    void CreateSphere();
    //@}

    //@{
    //! Slots for the actions of the Window menu
    void OpenViewportWindow();
    void OpenTimelineWindow();
    //@}

    //@{
    //! Slots for the actions of the help menu
    void HelpIndex();
    void About();
    //@}

    //------------------------------------------------------------------------------------

private:

    //@{
    //! Actions triggered when an item of the File menu is selected
    QAction * mActionFileNewScene;
    QAction * mActionFileOpenApp;
    QAction * mActionFileQuit;
    //@}

    //@{
    //! Actions triggered when an item of the Edit menu is selected
    QAction * mActionEditPreferences;
    //@}

    //@{
    //! Actions triggered when an item of the View menu is selected
    QAction * mActionViewShowFullscreenViewport;
    //@}

    //@{
    //! Actions triggered when an item of the Create menu is selected
	QAction * mActionCreatePrimitiveBox;
    QAction * mActionCreatePrimitiveSphere;
    //@}

    //@{
    //! Actions triggered when an item of the Window menu is selected
	QAction * mActionWindowViewport;
    QAction * mActionWindowTimeline;
    //@}

    //@{
    //! Actions triggered when an item of the help menu is selected
    QAction * mActionHelpIndex;
    QAction * mActionHelpAboutQt;
    QAction * mActionHelpAbout;
    //@}

    //------------------------------------------------------------------------------------

private:

    //! Initial width of the main window
    static const int INITIAL_WINDOW_WIDTH = 1280;

    //! Initial height of the main window
    static const int INITIAL_WINDOW_HEIGHT = 720;


    //! Splash screen displayed at the startup of the program
	QSplashScreen * mSplashScreen;

    //! Application options
    //! \todo Implement app-wide options
	//EditorOptions * mOptions;

    //@{
    //! Dock widgets
    ViewportDockWidget * mViewportDockWidget;
    TimelineDockWidget * mTimelineDockWidget;
    //@}

	//! Menu containing the checkable actions for the tool bars
	QMenu * mToolbarMenu;

	//! Menu containing the checkable actions for the dock widgets
	QMenu * mDockMenu;
};

#endif // EDITOR_H
