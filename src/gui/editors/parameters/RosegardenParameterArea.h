/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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

#include <QStackedWidget>
#include <vector>


class QWidget;
//class QVGroupBox;
class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QTabWidget;


namespace Rosegarden
{

class RosegardenParameterBox;


/**
 * A widget that arranges a set of Rosegarden parameter-box widgets
 * within a frame, in a dynamically configurable manner.
 */
class RosegardenParameterArea : public QStackedWidget
{
    Q_OBJECT
public:

    // Create the parameter display area.

    RosegardenParameterArea(QWidget *parent=0, const char *name=0 ); //, WFlags f=0);

    // Add a rosegarden parameter box to the list that are to be displayed.

    void addRosegardenParameterBox(RosegardenParameterBox *b);
    void setScrollAreaWidget();

    // List the supported methods of arranging the various parameter-box
    // widgets within the parameter area.

    enum Arrangement {
        CLASSIC_STYLE,  // A simple vertical tiling of parameter-box widgets.
        TAB_BOX_STYLE   // A horizontal list of tabs, displaying one box at a time.
    };

    // Redisplay the widgets with a different layout style.

    void setArrangement(Arrangement style);

protected:
    virtual void hideEvent(QHideEvent *);

signals:
    void hidden();

private:
    Arrangement m_style;                // The current layout style.

    // The list of parameter box widgets that are being displayed by this
    // widget.

    std::vector<RosegardenParameterBox *>  m_parameterBoxes;

    // Create a parallel array of group boxes, to be used when the
    // corresponding parameter box widget needs to be enclosed by a
    // titled outline.

    std::vector<QGroupBox *> m_groupBoxes;

    // Move a RosegardenParameterBox widget from one container to another.

    void moveWidget(QWidget *old_container, QWidget *new_container,
                    RosegardenParameterBox *box);

    QScrollArea *m_scrollArea;    
    
    QWidget *m_classic;        // The container widget for m_style==CLASSIC_STYLE.
    QVBoxLayout *m_classicLayout;
    
    QTabWidget *m_tabBox;     // The container widget for m_style==TAB_BOX_STYLE.
    QWidget *m_active;         // The current container widget.
    QWidget *m_spacing;
};


}

#endif
