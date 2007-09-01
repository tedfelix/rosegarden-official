#include "MatrixTest.h"

#include "MatrixStaff.h"

#include <QGraphicsScene>
#include <base/Composition.h>
#include <base/Track.h>
#include <base/Segment.h>

#include "MatrixViewElementManager.h"
#include "MatrixHLayout.h"
#include "scale/SnapGrid.h"

MatrixTest::MatrixTest(QWidget * parent)
    : QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    setScene(scene);

    setDragMode(RubberBandDrag);
    
    buildTestComposition();
    buildMatrixStaff();
}

void MatrixTest::buildTestComposition() {
    m_composition = new Rosegarden::Composition();
    
    Rosegarden::Track* track = new Rosegarden::Track(1);
    m_composition->addTrack(track);
    
    m_segment = new Rosegarden::Segment();
    m_segment->setTrack(track->getId());
    m_segment->setEndTime(3840 * 50);
    m_composition->addSegment(m_segment);
    
}

void MatrixTest::buildMatrixStaff() {
    m_viewElementManager = new Rosegarden::MatrixViewElementManager(*m_segment);
    
    Rosegarden::MatrixHLayout* hlayout = new Rosegarden::MatrixHLayout(m_composition, 10.0);
    Rosegarden::SnapGrid* snapGrid = new Rosegarden::SnapGrid(hlayout); 
}

MatrixTest::~MatrixTest()
{
}
