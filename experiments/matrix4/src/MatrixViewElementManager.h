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
};

}

#endif /*_RG_MATRIXVIEWELEMENTMANAGER_H_*/
