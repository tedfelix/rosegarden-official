/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INPUT_DIALOG_H
#define RG_INPUT_DIALOG_H

#include "gui/widgets/LineEdit.h"

#include <QDialog>


class LineEdit;
class QDialog;

/**This class provides an API-compatible drop-in replacement for
 * certain QInputDialog methods that stubbornly resisted styling any other way.
 *
 * The normal styling process has involved painting broad "widgetless" areas
 * with QWidget hacks.  These are necessary to paint certain void areas in the
 * main window background and in various other places that don't belong to
 * anything that can be addressed via a QSS selector.  These QWidget hacks
 * affect all child widgets, but it is normally possible to override the
 * background with highly specific selectors.  Not so for QInputDialog, it turns
 * out.  The only way to control its background after a QWidget hack upstream is
 * to use a local stylesheet directly in the code.
 *
 * This phenomenon affects us in two important ways.  Most critically, we have
 * had to resort to replacing QLineEdit with our own Rosegarden::LineEdit
 * subclass in order to allow for styling the text background, and we need a way
 * to use our own LineEdit inside these dialogs.  Less critically, but still
 * annoying, all these random, simple dialogs want to take their backgrounds
 * from whatever widget they launched off of, and this means we have a sea of
 * little superficial dialogs taking any of about three different backgrounds
 * unpredictably.  Afer much unsuccessful stumbling about, I elected to
 * re-implement part of QInputDialog's functionality in order for us to have a
 * place to solve these problems with local stylesheet hacks.  Local stylesheets
 * are the most specific selector of all, and trump anything that can be
 * achieved through any kind of external stylesheet.
 *
 * \author D. Michael McIntyre
 */
namespace Rosegarden
{

class InputDialog : public QDialog
{
    Q_OBJECT
private:
    /**Constructs the dialog. The \a title is the text which is displayed in
     * the title bar of the dialog. The \a label is the text which is shown to
     * the user (it should tell the user what they are expected to enter).
     * The \a parent is the dialog's parent widget. The \a input parameter
     * is the dialog to be used. The \a f parameter is passed on to the
     * QDialog constructor.
     *
     * \sa getText()
     *
     * \author Adapted from Qt-X11-OpenSource 4.4.3, Copyright (C) 2009 Nokia Corporation
     */
    InputDialog(const QString &title, const QString &label, QWidget *parent,
                QWidget *input, Qt::WindowFlags f);

    /**Destroys the input dialog
     */
    ~InputDialog() override;

public:
    /* Static convenience function to get a string from the user. \a
     * title is the text which is displayed in the title bar of the
     * dialog. \a label is the text which is shown to the user (it should
     * say what should be entered). \a text is the default text which is
     * placed in the line edit. The \a mode is the echo mode the line
     * edit will use. If \a ok is non-null \e *\a ok will be set to true
     * if the user pressed \gui OK and to false if the user pressed
     * \gui Cancel. The dialog's parent is \a parent. The dialog will be
     * modal and uses the widget flags \a f.
     *
     * This function returns the text which has been entered in the line
     * edit. It will not return an empty string.
     * 
     * \author Adapted from Qt-X11-OpenSource 4.4.3, Copyright (C) 2009 Nokia Corporation
     */
    static QString getText(QWidget *parent, const QString &title, const QString &label,
                           LineEdit::EchoMode echo = LineEdit::Normal,
                           const QString &text = QString(), bool *ok = nullptr, Qt::WindowFlags f = nullptr);

    // Let's see if we can do without reinventing getInteger et al.  We use
    // getInteger() in one place.  It probably has a bad background, but it's
    // just one place.  We'll see.

};

}

#endif
