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

#ifndef RG_THORN_STYLE_H
#define RG_THORN_STYLE_H

#include "IconLoader.h"

#include <QProxyStyle>
#include <QIcon>
#include <rosegardenprivate_export.h>

namespace Rosegarden
{


/** Subclass a widget style so we can define our own custom icons for
 * QMessageBox and so on and make them look better in the Thorn style.
 */
class ROSEGARDENPRIVATE_EXPORT ThornStyle : public QProxyStyle
{
    Q_OBJECT

public:
    ThornStyle();
    ~ThornStyle() override;

    static void setEnabled(bool b);
    static bool isEnabled();

    QPalette standardPalette() const override;

    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget = nullptr) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget = nullptr) const override;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget = nullptr) const override;

    QIcon standardIcon(StandardPixmap standardIcon,
                       const QStyleOption * option = nullptr,
                       const QWidget * widget = nullptr) const override;

private:
    QSize pixmapSize(const QPixmap &pixmap) const;


    mutable IconLoader m_iconLoader;
    QPalette m_standardPalette;

    QPixmap m_horizontalToolbarSeparatorPixmap;
    QPixmap m_verticalToolbarSeparatorPixmap;
    QPixmap m_checkboxUncheckedPixmap;
    QPixmap m_checkboxUncheckedHoverPixmap;
    QPixmap m_checkboxUncheckedDisabledPixmap;
    QPixmap m_checkboxUncheckedPressedPixmap;
    QPixmap m_checkboxCheckedPixmap;
    QPixmap m_checkboxCheckedHoverPixmap;
    QPixmap m_checkboxCheckedDisabledPixmap;
    QPixmap m_checkboxCheckedPressedPixmap;
    QPixmap m_checkboxIndeterminatePixmap;
    QPixmap m_checkboxIndeterminateHoverPixmap;
    //QPixmap m_checkboxIndeterminateDisabledPixmap;
    QPixmap m_checkboxIndeterminatePressedPixmap;
    QPixmap m_radiobuttonUncheckedPixmap;
    QPixmap m_radiobuttonUncheckedHoverPixmap;
    QPixmap m_radiobuttonUncheckedDisabledPixmap;
    QPixmap m_radiobuttonUncheckedPressedPixmap;
    QPixmap m_radiobuttonCheckedPixmap;
    QPixmap m_radiobuttonCheckedHoverPixmap;
    QPixmap m_radiobuttonCheckedDisabledPixmap;
    QPixmap m_radiobuttonCheckedPressedPixmap;
    QPixmap m_arrowDownSmallPixmap;
    QPixmap m_arrowDownSmallInvertedPixmap;
    QPixmap m_arrowUpSmallPixmap;
    QPixmap m_arrowUpSmallInvertedPixmap;
    QPixmap m_arrowLeftPixmap;
    QPixmap m_arrowRightPixmap;
    QPixmap m_arrowUpPixmap;
    QPixmap m_arrowDownPixmap;
    QPixmap m_spinupPixmap;
    QPixmap m_spinupHoverPixmap;
    QPixmap m_spinupOffPixmap;
    QPixmap m_spinupPressedPixmap;
    QPixmap m_spindownPixmap;
    QPixmap m_spindownHoverPixmap;
    QPixmap m_spindownOffPixmap;
    QPixmap m_spindownPressedPixmap;
    QPixmap m_titleClosePixmap;
    QPixmap m_titleUndockPixmap;
};


}

#endif
