/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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
    Q_ENUMS(State)
    Q_PROPERTY(State state READ state WRITE setState)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(int darkFactor READ darkFactor WRITE setDarkFactor)

public:

  Led(const QColor &color, QWidget *parent = NULL);
  ~Led();

  enum State { Off, On };
  void setState(State state);
  State state() const;

  void setColor(const QColor &color);
  QColor color() const;

  void setDarkFactor(int darkfactor);
  int darkFactor() const;

  // QWidget overrides
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

public slots:
  void toggle();
  void on();
  void off();

protected:
  // QWidget override
  void paintEvent(QPaintEvent *);

private:
  State m_state;

  QColor m_color;
  int m_darkFactor;
  QColor m_offColor;
  bool m_thorn;

  // Cached pixmaps
  QPixmap *m_offPixmap;
  QPixmap *m_onPixmap;
  bool paintCachedPixmap();
};


}

#endif
