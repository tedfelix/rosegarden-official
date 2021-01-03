/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ICONBUTTON_H
#define RG_ICONBUTTON_H

#include <QAbstractButton>
#include <QPixmap>
#include <QString>

class IconButton : public QAbstractButton
{
    Q_OBJECT
    
public:
    IconButton(QWidget* parent, const QPixmap& icon, const QString & name);

    void paintEvent(QPaintEvent*) override;
    void setCheckedColor(QColor color);
    
public slots:
    
signals:

private:
    QPixmap m_pixmap;
    QString m_labelText;
    QFont m_font;
    int m_margin;
    QSize m_labelSize;
    QColor m_textColor;
    QColor m_checkedColor;
};

#endif
