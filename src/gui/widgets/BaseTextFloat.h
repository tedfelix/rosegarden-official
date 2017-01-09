/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
  
/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_BASETEXTFLOAT_H
#define RG_BASETEXTFLOAT_H

#include <QString>
#include <QWidget>



class QPaintEvent;
class QTimer;
class QPoint;


namespace Rosegarden
{

/**

  A "text float" is a box containing some text which can be displayed anywhere
  above the application windows, not necessarily above its parent widget.

  To use a BaseTextFloat :

    1 - Create a BaseTextFloat object, specifying the text float parent widget

    2 - Call setText() to define which text to display

    3 - Call display() to move and show the text float

    4 - Call reparent() every time parent widgets have been moved or resized

    5 - Repeatedly call setText() to change the text and/or call display()
        to move the text float

    6 - call hide() or hideAfterDelay() to hide the text float

*/


class BaseTextFloat : public QWidget
{
    Q_OBJECT

public :

    /**
     * Create a new text float with parent widget \a parent
     * If \a parent is not null, \a reparent() is called to take the context in.
     */
    BaseTextFloat(QWidget *parent);

    virtual ~BaseTextFloat() {}

    /**
     * Move the text float to a new parent widget and/or look at the
     * graphical context.
     * This method has to be called each time one of the parent widgets
     * is moved or resized.
     * \arg newParent New parent, or 0 (default) if parent is not changed.
     */
    void reparent(QWidget *newParent = 0);

    /**
     * Set the text to display inside the TextFloat box.
     */
    void setText(const QString &text);


    /**
     * Move the text float to the \a offset position relative to the top left
     * corner of the parent widget and show it.
     */
    void virtual display(QPoint offset);

    /**
     * Hide the text float after \a delay ms.
     * (\a hide() may be used if no delay is wanted).
     */
    void virtual hideAfterDelay(int delay);

protected slots :

    // Used by hideAfterDelay() internal timer
    void slotTimeout();


protected :

    virtual void paintEvent(QPaintEvent *e);

    // Currently displayed text
    QString  m_text;

    // hideAfterDelay() internal timer
    QTimer  *m_timer;

    // Parent widget
    QWidget *m_widget;

    // Position of top left corner of the parent widget relative to the
    // top left corner of either top level or dialog window. 
    QPoint   m_totalPos;

    // Width and height of text-bounding rectangle.
    int m_width;
    int m_height;
};

}

#endif
