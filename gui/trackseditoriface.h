#ifndef TRACKSEDITORIFACE_H
#define TRACKSEDITORIFACE_H

#include <dcopobject.h>

/**
 * TracksEditor DCOP Interface
 *
 * @see TracksEditor
 */
class TracksEditorIface : virtual public DCOPObject
{
    K_DCOP
public:
k_dcop:
    virtual void addTrack(int instrument, int start, unsigned int length) = 0;
};

#endif
