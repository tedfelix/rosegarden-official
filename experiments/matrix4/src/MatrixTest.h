#ifndef MATRIXTEST_H_
#define MATRIXTEST_H_

#include <QMainWindow>

#include "generalgui/ZoomSlider.h"

namespace Rosegarden {

class Composition;
class Segment;
class MatrixViewElementManager;
class MatrixStaff;
class MatrixHLayout;
class MatrixVLayout;

}

class QGraphicsView;

class MatrixTest : public QMainWindow 
{
    Q_OBJECT
public:
	MatrixTest(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	virtual ~MatrixTest();
	
	void layout();

public slots:
    void zoomChanged(int);
    
protected:
    
    void buildTestComposition();
    void buildMatrixStaff();
    void buildToolBar();
    
    QGraphicsView* m_view;
    
    Rosegarden::Composition* m_composition;
    Rosegarden::Segment* m_segment;
    
    Rosegarden::MatrixViewElementManager* m_viewElementManager;
	Rosegarden::MatrixStaff* m_matrixStaff;
	Rosegarden::MatrixHLayout* m_hLayout;
	Rosegarden::MatrixVLayout* m_vLayout;

	Rosegarden::ZoomSlider<double>* m_zoomSlider;

};

#endif /*MATRIXTEST_H_*/
