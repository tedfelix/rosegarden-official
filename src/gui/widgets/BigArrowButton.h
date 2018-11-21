/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_BIGARROWBUTTON_H
#define RG_BIGARROWBUTTON_H

#include <QWidget>
#include <QPushButton>

namespace Rosegarden {


class BigArrowButton : public QPushButton
{
public:
    BigArrowButton(Qt::ArrowType arrow = Qt::RightArrow) :
                       QPushButton()
    { 
        QIcon icon;
        const char *fileName;

        switch (arrow) {
            case Qt::RightArrow :
                fileName = ":/pixmaps/misc/arrow-right.png";
                break;
            case Qt::LeftArrow :
                fileName = ":/pixmaps/misc/arrow-left.png";
                break;
            case Qt::UpArrow :
                fileName = ":/pixmaps/misc/arrow-up.png";
                break;
            case Qt::DownArrow :
                fileName = ":/pixmaps/misc/arrow-down.png";
                break;
            case Qt::NoArrow :
            default :
                fileName = nullptr;
        }

        if (fileName) {
            icon.addPixmap(QPixmap(QString::fromUtf8(fileName)),
                                          QIcon::Normal, QIcon::Off);
            setIcon(icon);
        } else {
            setText("???");
        }
    }

    ~BigArrowButton() override { } 

    QSize sizeHint() const override {
        return QSize(20, 20);
    }
};

	
}

#endif /*BIGARROWBUTTON_H_*/
