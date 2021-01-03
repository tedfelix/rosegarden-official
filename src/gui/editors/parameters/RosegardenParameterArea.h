/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file Copyright 2006 Martin Shepherd <mcs@astro.caltech.edu>.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENPARAMETERAREA_H
#define RG_ROSEGARDENPARAMETERAREA_H

#include <QScrollArea>
#include <vector>

class QWidget;
class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;

namespace Rosegarden
{

class RosegardenParameterBox;


/**
 * A widget that arranges a set of Rosegarden parameter-box widgets
 * within a frame, in a dynamically configurable manner.
 */
class RosegardenParameterArea : public QScrollArea
{
    Q_OBJECT
public:

    // Create the parameter display area.

    RosegardenParameterArea(QWidget *parent = nullptr);

    // Add a rosegarden parameter box to the list that are to be displayed.

    void addRosegardenParameterBox(RosegardenParameterBox *b);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    // The list of parameter box widgets that are being displayed by this
    // widget.

    std::vector<RosegardenParameterBox *>  m_parameterBoxes;

    // Create a parallel array of group boxes, to be used when the
    // corresponding parameter box widget needs to be enclosed by a
    // titled outline.

    std::vector<QGroupBox *> m_groupBoxes;

    QWidget *m_boxContainer;
    QVBoxLayout *m_boxContainerLayout;
};


}

#endif
