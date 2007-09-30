#include "MatrixTest.h"

#include "MatrixStaff.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QToolBar>
#include <base/Composition.h>
#include <base/Track.h>
#include <base/Segment.h>
#include <base/Event.h>

#include "scale/SnapGrid.h"
#include "MatrixViewElementManager.h"
#include "MatrixHLayout.h"
#include "MatrixVLayout.h"
#include "MatrixStaff.h"

MatrixTest::MatrixTest(QWidget * parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    m_view = new QGraphicsView(this);
    
    setCentralWidget(m_view);
    
    QGraphicsScene *scene = new QGraphicsScene(m_view);
    m_view->setScene(scene);

    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    
    buildToolBar();
    buildTestComposition();
    buildMatrixStaff();
    layout();
}

void MatrixTest::buildToolBar() {
    QToolBar *toolbar = addToolBar("mainToolBar");

    
    std::vector<double> zoomSizes; // in units-per-pixel
    
    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0, 20.0 };
    for (unsigned int i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
        zoomSizes.push_back(factors[i]);
    }

    m_zoomSlider = new Rosegarden::ZoomSlider<double>
                    (zoomSizes, -1, Qt::Horizontal, toolbar);
    m_zoomSlider->setTracking(true);
    m_zoomSlider->setFocusPolicy(Qt::NoFocus);
    
    toolbar->addWidget(m_zoomSlider);
    QObject::connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(zoomChanged(int)));

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
    
    m_matrixStaff = new Rosegarden::MatrixStaff(m_view->scene(), m_segment, snapGrid, m_viewElementManager, id, resolution);
    
}

void MatrixTest::layout() {
    m_hLayout->scanStaff(*m_viewElementManager, *m_matrixStaff);
    m_vLayout->scanStaff(*m_viewElementManager, *m_matrixStaff);
    
    m_hLayout->finishLayout();
    m_vLayout->finishLayout();

    m_matrixStaff->sizeStaff(*m_hLayout);
    
    m_matrixStaff->positionAllElements();
}

void MatrixTest::zoomChanged(int val)
{
    double zoomValue = m_zoomSlider->getCurrentSize();
    
    qDebug("MatrixTest::zoomChanged : %f\n", zoomValue);
    m_view->scale(zoomValue, 1.0);
//    update();
}

MatrixTest::~MatrixTest()
{
}

#include "MatrixTest.moc"
