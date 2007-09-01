#ifndef MATRIXTEST_H_
#define MATRIXTEST_H_

#include <QGraphicsView>

namespace Rosegarden { class Composition ; class Segment; class MatrixViewElementManager; }


class MatrixTest : public QGraphicsView
{
public:
	MatrixTest(QWidget *parent);
	virtual ~MatrixTest();
	
	
protected:
    
    void buildTestComposition();
    void buildMatrixStaff();
    
    Rosegarden::Composition* m_composition;
    Rosegarden::Segment* m_segment;
    
    Rosegarden::MatrixViewElementManager* m_viewElementManager;
	
};

#endif /*MATRIXTEST_H_*/
