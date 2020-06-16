/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTPARAMETERDIALOG_H
#define RG_EVENTPARAMETERDIALOG_H

#include "commands/edit/SelectionPropertyCommand.h"  // for SelectionSituation

#include <QDialog>

#include <vector>

class QWidget;
class QSpinBox;
class QString;
class QLabel;
class QComboBox;
class QLayout;

namespace Rosegarden
{


class ParameterPattern;

// @class EventParameterDialog Dialog about setting a property for a group of
//        events.  Ultimately makes a ParameterPattern::Result to be passed
//        to SelectionPropertyCommand.
// @author Tom Breton (Tehom) (adapted)
// @author Chris Cannam (originally)
class EventParameterDialog : public QDialog
{
    Q_OBJECT

public:
    // @author Tom Breton (Tehom) (some)
    // @author Chris Cannam (most)
    EventParameterDialog(
            QWidget *parent,
            const QString &name,
            SelectionSituation *situation,
            const ParameterPattern::ParameterPatternVec *patterns);

    ParameterPattern::Result getResult();

public slots:
    // React to selecting a pattern: Set up the parameter widgets in
    // accordance with what the pattern tells us it needs.
    void slotPatternSelected(int value);

private:
    // Helper containing non-gui data, which will outlive
    // EventParameterDialog.
    const SelectionSituation  *m_situation;

    // The available patterns.
    const ParameterPattern::ParameterPatternVec *m_patterns;
    // Get the current pattern index.
    const ParameterPattern * getPattern(int index) const
    { return m_patterns->at(index); }

    // The widget that chooses the current pattern.
    QComboBox *m_patternCombo;
    // Initialize m_patternCombo.
    void initPatternCombo();

    // The control layout which holds the individual parameter widgets.
    QLayout             *m_controlsLayout;

    /// A QLabel and a QSpinBox.
    /**
     * @author Tom Breton (Tehom)
     */
    class ParamWidget
    {
    public:
        ParamWidget(QLayout *parent);

        void showByArgs(const ParameterPattern::SliderSpec *args);
        void hide();

        int getValue();

    private:
        // We only include the widgets that we may want to interact with
        // in other code.
        QSpinBox *m_spinBox;
        QLabel *m_label;
    };
    typedef std::vector<ParamWidget> ParamWidgetVec;
    // All the parameter widgets.  Not all are used with
    // all patterns.
    ParamWidgetVec       m_paramVec;
    // Number of parameters currently in use.  Not always the same as
    // m_paramVec.size().
    int                  m_NbParameters;
    // Get a vector of the current parameters.  This makes part of our
    // final result object.
    ParameterPattern::BareParams getBareParams();
};


}

#endif
