#include <QApplication>
#include <QToolBar>
#include <QMainWindow>
#include "gvtest.h"
#include "MatrixTest.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  QMainWindow mainWindow;

#if 0
  QToolBar* toolbar = new QToolBar(&mainWindow);
  
  
  GVTest* gvtest = new GVTest(&mainWindow);

  toolbar->addAction("fade in",  gvtest, SLOT(fadeIn()));
  toolbar->addAction("fade out", gvtest, SLOT(fadeOut()));
  
  mainWindow.addToolBar(toolbar);
  mainWindow.setCentralWidget(gvtest);
  
  gvtest->setup();
#else

  MatrixTest* matrixTest = new MatrixTest(&mainWindow);
  
  mainWindow.setCentralWidget(matrixTest);
  
#endif
  mainWindow.show();
  
  return app.exec();
}
