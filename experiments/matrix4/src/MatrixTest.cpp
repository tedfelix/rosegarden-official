#include "MatrixTest.h"

#include "MatrixStaff.h"

#include <QGraphicsScene>
#include <base/Composition.h>
#include <base/Track.h>
#include <base/Segment.h>
#include <base/Event.h>

#include "scale/SnapGrid.h"
#include "MatrixViewElementManager.h"
#include "MatrixHLayout.h"
#include "MatrixVLayout.h"
#include "MatrixStaff.h"

MatrixTest::MatrixTest(QWidget * parent)
    : QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    setScene(scene);

    setDragMode(RubberBandDrag);
    
    buildTestComposition();
    buildMatrixStaff();
    layout();
}

void MatrixTest::buildTestComposition() {
    m_composition = new Rosegarden::Composition();
    
    Rosegarden::Track* track = new Rosegarden::Track(1);
    m_composition->addTrack(track);
    
    m_segment = new Rosegarden::Segment();
    m_segment->setTrack(track->getId());
    m_segment->setEndTime(3840 * 50);
    m_composition->addSegment(m_segment);
    
    Rosegarden::Event* event = new Rosegarden::Event(Rosegarden::Note::EventType, 3840 * 2, 3840);
    event->set<Rosegarden::Int>("pitch", 61);
    m_segment->insert(event);
}

void MatrixTest::buildMatrixStaff() {
    m_viewElementManager = new Rosegarden::MatrixViewElementManager(*m_segment);
    
    m_hLayout = new Rosegarden::MatrixHLayout(m_composition, 10.0);
    m_vLayout = new Rosegarden::MatrixVLayout();
    
    Rosegarden::SnapGrid* snapGrid = new Rosegarden::SnapGrid(m_hLayout);
    
    int resolution = 8;
    int id = 0;
    
    m_matrixStaff = new Rosegarden::MatrixStaff(scene(), m_segment, snapGrid, m_viewElementManager, id, resolution);
    
}

void MatrixTest::layout() {
    m_hLayout->scanStaff(*m_viewElementManager, *m_matrixStaff);
    m_vLayout->scanStaff(*m_viewElementManager, *m_matrixStaff);
    
    m_hLayout->finishLayout();
    m_vLayout->finishLayout();

    m_matrixStaff->sizeStaff(*m_hLayout);
    
    m_matrixStaff->positionAllElements();
}

MatrixTest::~MatrixTest()
{
}
