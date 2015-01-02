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


#ifndef RG_EDITVIEWTIMESIGNOTIFIER_H
#define RG_EDITVIEWTIMESIGNOTIFIER_H

namespace Rosegarden {
        
class EditViewTimeSigNotifier : public Rosegarden::CompositionObserver
{
public:
    EditViewTimeSigNotifier(RosegardenDocument *doc) :
        m_composition(&doc->getComposition()),
        m_timeSigChanged(false) {
        m_composition->addObserver(this);
    }
    virtual ~EditViewTimeSigNotifier() {
        if (!isCompositionDeleted()) m_composition->removeObserver(this);
    }
    virtual void timeSignatureChanged(const Rosegarden::Composition *c) {
        if (c == m_composition) m_timeSigChanged = true;
    }

    bool hasTimeSigChanged() const { return m_timeSigChanged; }
    void reset() { m_timeSigChanged = false; }

protected:
    Rosegarden::Composition *m_composition;
    bool m_timeSigChanged;
};

}

#endif /*EDITVIEWTIMESIGNOTIFIER_H_*/
