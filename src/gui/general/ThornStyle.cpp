/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ThornStyle.h"
#include "gui/general/IconLoader.h"
#include <QStyleFactory>


namespace Rosegarden
{

ThornStyle::ThornStyle()
{
    // Qt 5 removes QPlastiqueStyle and defaults to a new style called "Fusion."
    // This style forces combo boxes to do bad things.  The alternatives are:
    //
    // * replace data/rosegarden.qss with a fully custom QStyle
    // * replace QComboBox with a custom version that respects style differently
    // * replace the default style with something that causes less damage
    //
    // I concluded that the "windows" style causes an acceptable amount of
    // damage while leaving things largely intact.
    //
    setBaseStyle(QStyleFactory::create("windows"));
}

ThornStyle::~ThornStyle()
{
}

QIcon
ThornStyle::standardIconImplementation(StandardPixmap standardIcon,
                                        const QStyleOption *option,
                                        const QWidget *parent) const
{
    // NOTE: see src/gui/styles/qcommonstyle.cpp in the Qt source for examples
    // of how to extend this whenever more custom icons are called for
    switch (standardIcon) {

    // custom icons for QMessageBox
    case SP_MessageBoxInformation:
        return IconLoader().loadPixmap("messagebox-information");

    case SP_MessageBoxWarning:
        return IconLoader().loadPixmap("warning");

    case SP_MessageBoxCritical:
        return IconLoader().loadPixmap("messagebox-critical");

    case SP_MessageBoxQuestion:
        return IconLoader().loadPixmap("messagebox-question");

    // All fall thru to default
    case SP_TitleBarMenuButton:
    case SP_TitleBarMinButton:
    case SP_TitleBarMaxButton:
    case SP_TitleBarCloseButton:
    case SP_TitleBarNormalButton:
    case SP_TitleBarShadeButton:
    case SP_TitleBarUnshadeButton:
    case SP_TitleBarContextHelpButton:
    case SP_DockWidgetCloseButton:
    case SP_DesktopIcon:
    case SP_TrashIcon:
    case SP_ComputerIcon:
    case SP_DriveFDIcon:
    case SP_DriveHDIcon:
    case SP_DriveCDIcon:
    case SP_DriveDVDIcon:
    case SP_DriveNetIcon:
    case SP_DirOpenIcon:
    case SP_DirClosedIcon:
    case SP_DirLinkIcon:
    case SP_FileIcon:
    case SP_FileLinkIcon:
    case SP_ToolBarHorizontalExtensionButton:
    case SP_ToolBarVerticalExtensionButton:
    case SP_FileDialogStart:
    case SP_FileDialogEnd:
    case SP_FileDialogToParent:
    case SP_FileDialogNewFolder:
    case SP_FileDialogDetailedView:
    case SP_FileDialogInfoView:
    case SP_FileDialogContentsView:
    case SP_FileDialogListView:
    case SP_FileDialogBack:
    case SP_DirIcon:
    case SP_DialogOkButton:
    case SP_DialogCancelButton:
    case SP_DialogHelpButton:
    case SP_DialogOpenButton:
    case SP_DialogSaveButton:
    case SP_DialogCloseButton:
    case SP_DialogApplyButton:
    case SP_DialogResetButton:
    case SP_DialogDiscardButton:
    case SP_DialogYesButton:
    case SP_DialogNoButton:
    case SP_ArrowUp:
    case SP_ArrowDown:
    case SP_ArrowLeft:
    case SP_ArrowRight:
    case SP_ArrowBack:
    case SP_ArrowForward:
    case SP_DirHomeIcon:
    case SP_CommandLink:
    case SP_VistaShield:
    case SP_BrowserReload:
    case SP_BrowserStop:
    case SP_MediaPlay:
    case SP_MediaStop:
    case SP_MediaPause:
    case SP_MediaSkipForward:
    case SP_MediaSkipBackward:
    case SP_MediaSeekForward:
    case SP_MediaSeekBackward:
    case SP_MediaVolume:
    case SP_MediaVolumeMuted:
    case SP_CustomBase:

#if QT_VERSION >= 0x050000
    case SP_DirLinkOpenIcon:
    case SP_LineEditClearButton:
#endif

    default:
    // let the base class handle the rest
    return QProxyStyle::standardPixmap(standardIcon, option, parent);
    }
}


}

