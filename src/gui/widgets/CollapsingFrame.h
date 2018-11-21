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

#ifndef RG_COLLAPSINGFRAME_H
#define RG_COLLAPSINGFRAME_H

#include <QFrame>
#include <QString>


class QWidget;
class QToolButton;
class QGridLayout;


namespace Rosegarden
{


class CollapsingFrame : public QFrame
{
    Q_OBJECT

public:
    CollapsingFrame(
            QString label,
            QWidget *parent = nullptr,
            const QString &name = "",
            bool defaultExpanded = false);
    ~CollapsingFrame() override;

    QFont font() const;
    void setFont(QFont font);

    /// If true, the widget fills the available space.  Call before setWidget
    void setWidgetFill(bool fill);

    /// This frame contains a single other widget.  Set it here.
    void setWidget(QWidget *w);

public slots:
    void toggle();

protected:
    QGridLayout *m_layout;
    QToolButton *m_toggleButton;
    QWidget *m_widget;
    bool m_fill;
    bool m_collapsed;
};


}

#endif
