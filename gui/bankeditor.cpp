
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <klocale.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlineedit.h>

#include "bankeditor.h"
#include "widgets.h"

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   Rosegarden::Studio *studio):
    KDialogBase(parent, "", true, i18n("Bank Editor Dialog"),
                Ok | Cancel),
    m_studio(studio)
{
   QVBox *vBox = makeVBoxMainWidget();

   QHBox *bankHBox = new QHBox(vBox);
   new QLabel(i18n("Bank"), bankHBox);
   new QLabel(i18n("MSB"), bankHBox);
   m_msb = new QSpinBox(bankHBox);
   new QLabel(i18n("LSB"), bankHBox);
   m_lsb = new QSpinBox(bankHBox);

   QHBox *progHBox = new QHBox(vBox);

   QVBox *vbox1 = new QVBox(progHBox);

   QFont font;
   font.setPointSize(6);
   for (unsigned int i = 0; i < 32; i++)
   {
       m_programNames.push_back(new QLineEdit(vbox1));
       m_programNames[i]->setFont(font);
   }

}

void
BankEditorDialog::populateDialog()
{

}


