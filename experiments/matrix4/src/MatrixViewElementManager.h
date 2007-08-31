#ifndef _RG_MATRIXVIEWELEMENTMANAGER_H_
#define _RG_MATRIXVIEWELEMENTMANAGER_H_

#include "viewelement/AbstractViewElementManager.h"

namespace Rosegarden {

class MatrixViewElementManager : public AbstractViewElementManager
{
public:
	virtual ~MatrixViewElementManager();
	
protected:
    
    virtual ViewElement* makeViewElement(Event*);	
    /**
     * Return true if the event should be wrapped
     * Useful for piano roll where we only want to wrap notes
     */
    virtual bool wrapEvent(Event *);

    /**
     * Override from Staff<T>
     * Let tools know if their current element has gone
     */
    virtual void eventRemoved(const Segment *, Event *);

};

}

#endif /*_RG_MATRIXVIEWELEMENTMANAGER_H_*/
