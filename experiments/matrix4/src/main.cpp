#include <QApplication>
#include <QToolBar>
#include <QMainWindow>
#include "gvtest.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  QMainWindow mainWindow;
  
  QToolBar* toolbar = new QToolBar(&mainWindow);
  
  
  GVTest* gvtest = new GVTest(&mainWindow);

  toolbar->addAction("fade in",  gvtest, SLOT(fadeIn()));
  toolbar->addAction("fade out", gvtest, SLOT(fadeOut()));
  
  mainWindow.addToolBar(toolbar);
  mainWindow.setCentralWidget(gvtest);
  
  gvtest->setup();
  
  mainWindow.show();
  
  return app.exec();
}
