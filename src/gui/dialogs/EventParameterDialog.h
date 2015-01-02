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

#ifndef RG_EVENTPARAMETERDIALOG_H
#define RG_EVENTPARAMETERDIALOG_H

#include "base/PropertyName.h"
#include "commands/edit/SelectionPropertyCommand.h"
#include <QDialog>
#include <vector>

class QWidget;
class QSpinBox;
class QString;
class QLabel;
class QComboBox;
class QHBoxLayout;
class QLayout;

namespace Rosegarden
{

class ParameterPattern;

// @class EventParameterDialog Dialog about setting a property for a group of
// events.  Ultimately makes a ParameterPattern::Result to be passed
// to SelectionPropertyCommand.
// @author Tom Breton (Tehom) (adapted)
// @author Chris Cannam (originally)
class EventParameterDialog : public QDialog
{
    Q_OBJECT

     // Typedefs so we can refer to types defined in ParameterPattern
     // concisely.  We don't try to abbreviate everything, just the
     // names that are used often.
    typedef ParameterPattern::BareParams          BareParams;
    typedef ParameterPattern::SliderSpec          SliderSpec;
    typedef ParameterPattern::ParameterPatternVec ParameterPatternVec;

protected:
    // @class ParamWidget A group of widgets corresponding to one
    // parameter.  This class is internal to EventParameterDialog.
    // @author Tom Breton (Tehom)
    class ParamWidget
    {
    public:
      ParamWidget(QLayout *parent);
      void showByArgs(const SliderSpec* args);
      void hide(void);
      int getValue(void);
      
    private:
      // We only include the widgets that we may want to interact with
      // in other code.
      QSpinBox          *m_spinBox;
      QLabel            *m_label;
    };

    // Typedefs about ParamWidget
    typedef std::vector<ParamWidget> ParamWidgetVec;
    typedef ParamWidgetVec::iterator WidIterator;

public:
    EventParameterDialog(QWidget *parent,
                         const QString &name,
			 SelectionSituation *situation,
			 const ParameterPatternVec *patterns);
private:
    // Initialize just the pattern-choosing widgets.
    void    initializePatternBox(void);

public:
    // Get the entire result
    ParameterPattern::Result getResult(void);
private:
    // Get just the BareParams part of the result.
    BareParams getBareParams(void);
    // Get the current pattern index.
    const ParameterPattern * getPattern(int index) const
    { return m_patterns->at(index); }
    
public slots:
    void slotPatternSelected(int value);

protected:
    // The widget that chooses the current pattern.
    QComboBox           *m_patternCombo;
    // The control layout which holds the individual parameter widgets.
    QLayout             *m_controlsLayout;
    // All the parameter widgets.  Not all are used with
    // all patterns.
    ParamWidgetVec       m_paramVec;
    // Helper containing non-gui data, which will outlive
    // EventParameterDialog.
    const SelectionSituation  *m_situation;
    // The available patterns.
    const ParameterPatternVec *m_patterns;
    // Number of parameters currently in use.  Not always the same as
    // m_paramVec.size().
    int                  m_NbParameters;
};

}

#endif
