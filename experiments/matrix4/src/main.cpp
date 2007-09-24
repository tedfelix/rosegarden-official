#include <QApplication>
#include <QToolBar>
#include <QSlider>
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
  
  QToolBar *toolbar = mainWindow.addToolBar("mainToolBar");
  
  QSlider* zoomSlider = new QSlider(Qt::Horizontal, toolbar);
  zoomSlider->setRange(1, +100);
  zoomSlider->setValue(10);
  toolbar->addWidget(zoomSlider);
  QObject::connect(zoomSlider, SIGNAL(valueChanged(int)), matrixTest, SLOT(zoomChanged(int)));
  
#endif
  mainWindow.show();
  
  return app.exec();
}
