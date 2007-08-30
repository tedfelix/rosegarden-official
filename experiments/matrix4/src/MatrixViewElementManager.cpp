#include "base/Event.h"
#include "MatrixViewElementManager.h"

MatrixViewElementManager::MatrixViewElementManager()
{
}

MatrixViewElementManager::~MatrixViewElementManager()
{
}

bool MatrixViewElementManager::wrapEvent(Event* e)
{
    // Changed from "Note or Time signature" to just "Note" because
    // there should be no time signature events in any ordinary
    // segments, they're only in the composition's ref segment

    return e->isa(Note::EventType) &&
           AbstractViewElementManager::wrapEvent(e);
}

ViewElement*
MatrixViewElementManager::makeViewElement(Event* e)
{
    return new MatrixElement(e, m_view->isDrumMode());
}

