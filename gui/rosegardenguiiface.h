#ifndef ROSEGARDENGUIIFACE_H
#define ROSEGARDENGUIIFACE_H

#include <dcopobject.h>

class RosegardenGUIIface : virtual public DCOPObject
{
    K_DCOP
public:
k_dcop:

    virtual void quit() = 0;
};

#endif
