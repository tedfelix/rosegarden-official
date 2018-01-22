/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SQUEEZED_LABEL_H
#define RG_SQUEEZED_LABEL_H

#include <QLabel>

namespace Rosegarden
{


class SqueezedLabelPrivate;

/**
 * @short A replacement for QLabel that squeezes its text
 *
 * A label class that squeezes its text into the label
 *
 * If the text is too long to fit into the label it is divided into
 * remaining left and right parts which are separated by three dots.
 *
 * Example:
 * http://www.kde.org/documentation/index.html could be squeezed to
 * http://www.kde...ion/index.html
 *
 * Adapted from KDE 4.2.0, this code originally Copyright (c) 2000 Ronny
 * Standtke
 *
 * @author Ronny Standtke <Ronny.Standtke@gmx.de>
 */

/*
 * QLabel
 */
class SqueezedLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode)

public:
    /**
    * Default constructor.
    */
    explicit SqueezedLabel(QWidget *parent = 0);
    explicit SqueezedLabel(const QString &text, QWidget *parent = 0);

    virtual ~SqueezedLabel();

    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
    /**
    * Overridden for internal reasons; the API remains unaffected.
    */
    virtual void setAlignment(Qt::Alignment);

    /**
    *  Returns the text elide mode.
    */
    Qt::TextElideMode textElideMode() const;

    /**
    * Sets the text elide mode.
    * @param mode The text elide mode.
    */
    void setTextElideMode(Qt::TextElideMode mode);

public Q_SLOTS:
    /**
    * Sets the text. Note that this is not technically a reimplementation of QLabel::setText(),
    * which is not virtual (in Qt 4.3). Therefore, you may need to cast the object to
    * SqueezedLabel in some situations:
    * \Example
    * \code
    * SqueezedLabel* squeezed = new SqueezedLabel("text", parent);
    * QLabel* label = squeezed;
    * label->setText("new text");    // this will not work
    * squeezed->setText("new text");    // works as expected
    * // Dynamic downcast from QLabel to SqueezedLabel.
    * SqueezedLabel *s2 = dynamic_cast<SqueezedLabel *>(label);
    * if (s2) s2->setText("new text");    // works as expected
    * \endcode
    * @param mode The new text.
    */
    void setText(const QString &text);
    /**
    * Clears the text. Same remark as above.
    *
    */
    void clear();

protected:
    /**
    * Called when widget is resized
    */
    void resizeEvent(QResizeEvent *);
    /**
    * \reimp
    */
    void contextMenuEvent(QContextMenuEvent*);
    /**
    * does the dirty work
    */
    void squeezeTextToLabel();

private:
    Q_PRIVATE_SLOT(d, void k_copyFullText())
    SqueezedLabelPrivate * const d;
};


}

#endif
