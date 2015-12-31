/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PARAMETERPATTERN_H
#define RG_PARAMETERPATTERN_H

#include "base/PropertyName.h"
#include "base/parameterpattern/SelectionSituation.h"
#include <QMainWindow>
#include <QString>
#include <map>
#include <vector>

namespace Rosegarden
{

class EventParameterDialog;
class SelectionPropertyCommand;

// @typedef ParameterPatternBareParameters The bare parameters.
// modifySegment uses this type as well as other data to determine
// what to do.  Declared here because the compiler doesn't like it
// when declared inside ParameterPattern, so we just typedef it into
// there.
typedef std::vector<int> ParameterPatternBareParameters;

// @class ParameterPattern Base class of parameter patterns.  This
// class has no data members.  It is used for its static functions,
// its nested types, and its descendants' vtables.  Only a static
// instance of each subclass exists.
// @author Tom Breton (Tehom)
class ParameterPattern
{
public:
    // Many of the nested classes had to be all public so that derived
    // classes could see them, so in order to have degree of
    // encapsulation, they are protected and the classes that would
    // use them are friended.  Not perfect, but until C++ lets us
    // friend derived classes, it'll have to do.
    friend class EventParameterDialog;
    friend class SelectionPropertyCommand;
    friend class ControlRulerWidget;

    typedef EventSelection::eventcontainer::iterator iterator;
    typedef std::pair<timeT,timeT> StartAndDuration;
    typedef ParameterPatternBareParameters BareParams;
    typedef std::vector<ParameterPattern *> ParameterPatternVec;
    
    /*** Nested class SliderSpec  ***/

protected:
    // @class SliderSpec The arguments to make a slider that are of
    // interest here, grouped in one class.
    // @author Tom Breton (Tehom)
    struct SliderSpec
    {
    SliderSpec(QString label, int defaultValue,
               const SelectionSituation *situation,
               int minValue = 0)
    : m_label(label),
            m_defaultValue(defaultValue),
            m_minValue(minValue),
            m_maxValue(situation->maxValue())
        {}
        QString m_label;
        int m_defaultValue;
        int m_minValue;
        int m_maxValue;
    };

    typedef std::vector<SliderSpec>  SliderSpecVector;

    /*** Nested class Result  ***/
    
protected:
    // @class EventParameter::Result The result of an EventParameterDialog
    // or similar, to inform for SelectionPropertyCommand.  It's open
    // because there's no way to friend all derived classes of
    // ParameterPattern.
    // @author Tom Breton (Tehom)
    struct Result
    {
    Result(const SelectionSituation *situation,
           const ParameterPattern *pattern,
           const BareParams        parameters)
    : m_situation(situation),
            m_pattern(pattern),
            m_parameters(parameters)
        {}

    Result(const SelectionSituation *situation,
           const ParameterPattern *pattern,
           int                     soleParameter)
    : m_situation(situation),
            m_pattern(pattern),
            m_parameters(1, soleParameter)
        {}

        EventSelection *getSelection(void);
        void            modifySegment(void);
        
        const SelectionSituation *m_situation;
        const ParameterPattern   *m_pattern;
        const BareParams          m_parameters;
    };

    /*** End-to-end methods that do the complete operation  ***/

public:
    // Set velocities flat, no dialog. 
    static void setVelocitiesFlat(EventSelection *selection,
                                  int targetVelocity);
    
    // Set some property flat, no dialog. 
    static void setPropertyFlat(EventSelection *selection,
                                const std::string eventType,
                                int targetValue);
    
    // Set velocities, with a dialog
    static void setVelocities(QMainWindow *parent,
                              EventSelection *selection,
                              int normVelocity = -1);

    // Set some property, with a dialog
    static void setProperties(QMainWindow *parent,
                              EventSelection *selection,
                              const std::string eventType,
                              const ParameterPatternVec *patterns,
                              int normValue = -1);

    static void setProperties(QMainWindow *parent,
                              SelectionSituation   *situation,
                              const ParameterPatternVec *patterns);

    /*** The abstract virtual methods ***/

protected:
    virtual ~ParameterPattern();

    virtual QString getText(QString propertyName) const =0;

    // Get the arguments that inform gui sliders or spinboxes.  This
    // makes whatever number the parameter pattern think is
    // appropriate, it's up to EventParameterDialog to handle it.
    virtual SliderSpecVector
    getSliderSpec(const SelectionSituation *situation) const =0;

    // Set the properties of events from begin to end
    // @param result is the result of an EventParameterDialog.
    virtual void
    setEventProperties(iterator begin, iterator end,
                       Result *result) const =0;

    /*** Static helper functions  ***/

    static StartAndDuration getTimes (iterator begin, iterator end);

    /*** Static objects of interest  ***/
    
public:
    // All the ParameterPatterns that are useful with velocity.
    static ParameterPatternVec VelocityPatterns;
    // The flat pattern, for setPropertyFlat
    static ParameterPattern *FlatPattern;
    // All the ParameterPatterns that are useful with controllers
    static ParameterPatternVec ControllerPatterns;

};



}

#endif
