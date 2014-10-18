//=============================================================================================================
/**
* @file     selectionmanagerwindow.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the SelectionManagerWindow class.
*
*/

#ifndef SELECTIONMANAGERWINDOW_H
#define SELECTIONMANAGERWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_selectionmanagerwindow.h"
#include "utils/layoutloader.h"         //MNE-CPP utils
#include "utils/selectionloader.h"         //MNE-CPP utils
#include "../Utils/layoutscene.h"       //MNE Browse Raw QT utils
#include "fiff/fiff.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QMutableStringListIterator>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class LayoutScene;


/**
* DECLARE CLASS SelectionManagerWindow
*
* @brief The SelectionManagerWindow class provides a channel selection window.
*/
class SelectionManagerWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a SelectionManagerWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new SelectionManagerWindow becomes a window. If parent is another widget, SelectionManagerWindow becomes a child window inside parent. SelectionManagerWindow is deleted when its parent is deleted.
    */
    SelectionManagerWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the SelectionManagerWindow.
    * All SelectionManagerWindow's children are deleted first. The application exits if SelectionManagerWindow is the main widget.
    */
    ~SelectionManagerWindow();

    //=========================================================================================================
    /**
    * Sets the currently loaded fiff channels. used to create the group All.
    */
    void setCurrentlyLoadedFiffChannels(FiffInfo loadedFiffInfo);

    //=========================================================================================================
    /**
    * Highlight channels
    * This function highlights channels which were selected outside this selection manager (i.e in the DataWindow's Table View)
    */
    void highlightChannels(QStringList channelList);

    //=========================================================================================================
    /**
    * Select channels
    * This function selects channels which were selected outside this selection manager (i.e in the DataWindow's Table View)
    */
    void selectChannels(QStringList channelList);

    //=========================================================================================================
    /**
    * Current selected channels
    * This function returns the current channel selection
    */
    QStringList getSelectedChannels();

    //=========================================================================================================
    /**
    * gets the item corresponding to text in listWidget
    */
    QListWidgetItem* getItem(QListWidget *listWidget, QString text);

signals:
    void showSelectedChannelsOnly(QStringList selectedChannels);

private:
    //=========================================================================================================
    /**
    * Initialises all tabel widgets in the selection window.
    *
    */
    void initListWidgets();

    //=========================================================================================================
    /**
    * Initialises all graphic views in the selection window.
    *
    */
    void initGraphicsView();

    //=========================================================================================================
    /**
    * Initialises all combo boxes in the selection window.
    *
    */
    void initComboBoxes();

    //=========================================================================================================
    /**
    * Loads a new layout from given file path.
    *
    * @param [in] path holds file pathloll
    */
    bool loadLayout(QString path);

    //=========================================================================================================
    /**
    * Loads a new selection from given file path.
    *
    * @param [in] path holds file path
    */
    bool loadSelectionGroups(QString path);

    //=========================================================================================================
    /**
    * Delete all MEG channels from the selection groups which are not in the loaded layout. This needs to be done to guarantee consistency between the selection files and layout files (the selection files always include ALL MEG channels (gradiometers+magnitometers))
    *
    */
    void cleanUpMEGChannels();

    //=========================================================================================================
    /**
    * Updates selection files table widget in this window.
    *
    */
    void updateSelectionFiles(QString text);

    //=========================================================================================================
    /**
    * Updates selection group widget in this window.
    *
    */
    void updateSelectionGroups(QListWidgetItem *item);

    //=========================================================================================================
    /**
    * Updates the scene regarding the selecting channel QList.
    *
    */
    void updateSceneItems();

    //=========================================================================================================
    /**
    * Updates user defined selections.
    *
    */
    void updateUserDefinedChannels();

    //=========================================================================================================
    /**
    * Updates data view.
    *
    */
    void updateDataView();

    //=========================================================================================================
    /**
    * Reimplemented resize event.
    *
    */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
    * Installed event filter.
    *
    */
    bool eventFilter(QObject *obj, QEvent *event);

    Ui::SelectionManagerWindow*     ui;

    QMap<QString,QVector<double> >  m_layoutMap;
    QMap<QString,QStringList>       m_selectionGroupsMap;

    LayoutScene*                    m_pLayoutScene;

    QStringList                     m_currentlyLoadedFiffChannels;
};

} // NAMESPACE MNEBrowseRawQt

#endif // SELECTIONMANAGERWINDOW_H