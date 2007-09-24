#ifndef MATRIXTEST_H_
#define MATRIXTEST_H_

#include <QGraphicsView>

namespace Rosegarden {

class Composition;
class Segment;
class MatrixViewElementManager;
class MatrixStaff;
class MatrixHLayout;
class MatrixVLayout;

}


class MatrixTest : public QGraphicsView
{
    Q_OBJECT
public:
	MatrixTest(QWidget *parent);
	virtual ~MatrixTest();
	
	void layout();

public slots:
    void zoomChanged(int);
    
protected:
    
    void buildTestComposition();
    void buildMatrixStaff();
    
    Rosegarden::Composition* m_composition;
    Rosegarden::Segment* m_segment;
    
    Rosegarden::MatrixViewElementManager* m_viewElementManager;
	Rosegarden::MatrixStaff* m_matrixStaff;
	Rosegarden::MatrixHLayout* m_hLayout;
	Rosegarden::MatrixVLayout* m_vLayout;
};

#endif /*MATRIXTEST_H_*/
