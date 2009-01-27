


#ifndef DEVICESMANAGERNEW_H
#define DEVICESMANAGERNEW_H


#include "gui/ui/DevicesManagerNewUi.h"
#include <QWidget>
#include <QDialog>


namespace Rosegarden {

class DevicesManagerNew : public Ui::DevicesManagerNewUi, public QDialog
{

public:
// 	DevicesManagerNew( );
 	DevicesManagerNew( QWidget* parent )
	//*
	{
		setupUi( dynamic_cast<QDialog*>(this) );
	}
	// */
	
	
protected:
	//	
};


} // end namespace Rosegarden

#endif // DEVICESMANAGERNEW_H

