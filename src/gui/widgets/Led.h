/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    This file is based on KLed from the KDE libraries
    Copyright (C) 1998 Jörg Habenicht (j.habenicht@europemail.com)

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LED_H
#define RG_LED_H

#include <QWidget>
#include <QColor>

class QPixmap;

namespace Rosegarden
{

/// An LED class based on KLed from KDE.
class Led : public QWidget
{
    Q_OBJECT

public:

  Led(const QColor &color, QWidget *parent = nullptr);
  ~Led() override;

  enum State { Off, On };
  void setState(State state);
  State state() const  { return m_state; }

  void setColor(const QColor &color);
  QColor color() const  { return m_color; }

  // QWidget overrides
  QSize sizeHint() const  override { return QSize(16, 16); }
  QSize minimumSizeHint() const  override { return QSize(16, 16); }

public slots:
  void toggle()  { setState((m_state == On) ? Off : On); }
  void on()  { setState(On); }
  void off()  { setState(Off); }

protected:
  // QWidget override
  void paintEvent(QPaintEvent *) override;

private:
  State m_state;

  QColor m_backgroundColor;
  QColor m_color;
  const int m_darkFactor;
  QColor m_offColor;

  void draw(QPainter &painter);
};


}

#endif
