#ifndef ROSEGARDENGUIIFACE_H
#define ROSEGARDENGUIIFACE_H

#include <MappedEvent.h>
#include <dcopobject.h>

class RosegardenGUIIface : virtual public DCOPObject
{
    K_DCOP
public:
k_dcop:
    virtual int  openFile(const QString &url, int mode) = 0;
    virtual void fileNew()                       = 0;
    virtual void fileSave()                      = 0;
    virtual void fileClose()                     = 0;
    virtual void quit()                          = 0;

    virtual const Rosegarden::MappedComposition&
            getSequencerSlice(const int &sliceStart, const int &sliceEnd) = 0;
};

#endif
