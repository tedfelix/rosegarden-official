#include <QApplication>
#include <QToolBar>
#include <QSlider>
#include <QMainWindow>
#include "gvtest.h"
#include "MatrixTest.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

#if 0
  QMainWindow mainWindow;

  QToolBar* toolbar = new QToolBar(&mainWindow);
  
  
  GVTest* gvtest = new GVTest(&mainWindow);

  toolbar->addAction("fade in",  gvtest, SLOT(fadeIn()));
  toolbar->addAction("fade out", gvtest, SLOT(fadeOut()));
  
  mainWindow.addToolBar(toolbar);
  mainWindow.setCentralWidget(gvtest);
  
  gvtest->setup();

  mainWindow.show();

#else

  MatrixTest* matrixTest = new MatrixTest();
    
  matrixTest->show();
  
#endif
  
  return app.exec();
}
