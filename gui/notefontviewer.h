// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _NOTE_FONT_VIEWER_H_
#define _NOTE_FONT_VIEWER_H_

#include <qstringlist.h>
#include <qframe.h>
#include <kdialogbase.h>

class QWidget;
class KComboBox;
class FontViewFrame;
class QString;


class NoteFontViewer : public KDialogBase
{
    Q_OBJECT

public:
    NoteFontViewer(QWidget *parent, QString noteFontName,
		   QStringList systemFontNames, int pixelSize);

protected slots:
    void slotFontChanged(const QString &);
    void slotViewChanged(int);
    void slotRowChanged(const QString &);

private:
    KComboBox *m_font;
    KComboBox *m_view;
    KComboBox *m_rows;
    FontViewFrame *m_frame;
};


class FontViewFrame : public QFrame
{
    Q_OBJECT

public:
    FontViewFrame(int pixelSize, QWidget *parent = 0, const char *name = 0);
    virtual ~FontViewFrame();

    QSize sizeHint() const;
    bool hasRow(int row) const;

public slots:
    void setFont(QString name);
    void setRow(int);
    void setGlyphs(bool glyphs);

protected:
    QSize cellSize() const;
    void paintEvent( QPaintEvent* );
    void loadFont();

private:
    QString m_fontName;
    int m_fontSize;
    void *m_tableFont;
    int m_row;
    bool m_glyphs;
};



#endif

