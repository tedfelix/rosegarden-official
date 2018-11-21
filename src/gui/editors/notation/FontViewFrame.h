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

#ifndef RG_FONTVIEWFRAME_H
#define RG_FONTVIEWFRAME_H

#include <QFrame>
#include <QSize>
#include <QString>


class QWidget;
class QPaintEvent;


namespace Rosegarden
{



class FontViewFrame : public QFrame
{
    Q_OBJECT

public:
    FontViewFrame(int pixelSize, QWidget *parent = nullptr);
    virtual ~FontViewFrame();

    QSize sizeHint() const override;
    bool hasRow(int row) const;

public slots:
    void setFont(QString name);
    void setRow(int);
    void setGlyphs(bool glyphs);

protected:
    QSize cellSize() const;
    void paintEvent( QPaintEvent* ) override;
    void loadFont();

private:
    QString m_fontName;
    int m_fontSize;
    QFont *m_tableFont;
    int m_ascent;
    int m_row;
    bool m_glyphs;
};


}

#endif
