/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file	AssetLibraryWidget.h
//! \author	Kleber Garcia
//! \date	16 Match 2014
//! \brief	Graphics widget representing the shader library form.

#ifndef EDITOR_SHADERLIBRARYWIDGET_H
#define EDITOR_SHADERLIBRARYWIDGET_H

#include <QDockWidget>
#include <QAction>
#include "ui_AssetLibraryWidget.h"

class QItemSelectionModel;
class CodeEditorWidget;
class ProgramTreeModel;
class SourceCodeListModel;
class SourceCodeManagerEventListener;

//! Graphics Widget meant for shader navigation & management
class AssetLibraryWidget : public QDockWidget
{
    Q_OBJECT

public:
    AssetLibraryWidget(QWidget * parent, CodeEditorWidget * editorWidget);
    virtual ~AssetLibraryWidget();

public slots:
    //! slot triggered when app is loaded
    void UpdateUIForAppLoaded();

    //! updates the ui items laytout
    void UpdateUIItemsLayout();

    //! slot triggered when app is unloaded
    void UpdateUIForAppFinished();

    //! slot triggered when shader editor is dispatched
    void DispatchShaderEditButton();

    //! slot triggered when a program editor is to be dispatched
    void DispatchProgramEditButton();

    //! slot triggered when something has been selected through shader tree view
    void DispatchTextEditorThroughShaderView(const QModelIndex& index);

    //! slot triggered when something has been selected through program tree view
    void DispatchTextEditorThroughProgramView(const QModelIndex& index);

     //! slot triggered when something has been selected through blockscript tree view
    void DispatchTextEditorThroughBlockScriptView(const QModelIndex& index);
    
    //! slot triggered when the editor style has changed, forces the text editor to resync styles
    void UpdateEditorStyle();

    //! slot triggered when there is a program linking event
    void OnProgramLinkingEvent(void * program, QString message, int eventType);

    //! gets the shader editor widget
    CodeEditorWidget * GetCodeEditorWidget() { return mCodeEditorWidget; }

    //! gets the shader manager event listener
    SourceCodeManagerEventListener * GetSourceCodeManagerEventListener() { return mSourceCodeManagerEventListener;}

    //! updates any layout requiring an update
    void UpdateAssetViewLayout(QString objName);

    //! a user has closed a code object, so delete the user data. 
    void UpdateRemovedCodeObject(void* userData, QString objName);

private:
    //! toggle the buttons on this widget on / off
    void ActivateButtons(bool activation);

    //! inject all user data inside shaders and program nodes 
    void InitializeInternalUserData();

    //! delete all user data inside shaders and program nodes
    void UninitializeInternalUserData();

    //! updates all the program user data (fills in for programs lacking of user data).
    void UpdateProgramUserData();

    //! updates all the shader user data (fills in for shaders lacking of user data).
    void UpdateShaderUserData();

    //! updates all the blockscript user data (fills in for blockscripts lacking of user data).
    void UpdateBlockScriptUserData();

    //! ui components
    Ui::AssetLibraryWidget ui;

    //! reference to the program tree data model
    ProgramTreeModel * mProgramTreeModel;

    //! reference to the program tree selection model (used to determine what is selected)
    QItemSelectionModel * mProgramSelectionModel;

    //! reference to the shader list data model
    SourceCodeListModel * mShaderListModel;

    //! reference to the block script list data model
    SourceCodeListModel * mBlockScriptListModel;

    //! reference to the shader list selection model (used to determine what shader is selected)
    QItemSelectionModel * mShaderListSelectionModel;

    //! reference to the shader list selection model (used to determine what shader is selected)
    QItemSelectionModel * mBlockScriptListSelectionModel;
    
    //! reference to the shader text editor widget
    CodeEditorWidget * mCodeEditorWidget;

    //! shader manager event listener
    SourceCodeManagerEventListener * mSourceCodeManagerEventListener;

};

#endif
