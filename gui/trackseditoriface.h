// -*- c-basic-offset: 4 -*-
#ifndef TRACKEDITORIFACE_H
#define TRACKEDITORIFACE_H

#include <dcopobject.h>

/**
 * TrackEditor DCOP Interface
 *
 * @see TrackEditor
 */
class TrackEditorIface : virtual public DCOPObject
{
    K_DCOP
public:
k_dcop:
    virtual void addSegment(int instrument, int start, unsigned int length) = 0;
};

#endif
