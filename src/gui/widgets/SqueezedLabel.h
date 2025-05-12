/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
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
class SqueezedLabel : public QLabel
{
    Q_OBJECT
    //Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode)

public:
    /**
    * Default constructor.
    */
    explicit SqueezedLabel(QWidget *parent = nullptr);
    explicit SqueezedLabel(const QString &text, QWidget *parent = nullptr);

    ~SqueezedLabel() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    /**
    * Overridden for internal reasons; the API remains unaffected.
    */
    virtual void setAlignment(Qt::Alignment);

    /**
    *  Returns the text elide mode.
    */
    // unused Qt::TextElideMode textElideMode() const;

    /**
    * Sets the text elide mode.
    * @param mode The text elide mode.
    */
    // unused void setTextElideMode(Qt::TextElideMode mode);

    /// Do not hijack the tool tip.  Let the client have control.
    void allowToolTip();

signals:
    void clicked();
    void doubleClicked();

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
    void resizeEvent(QResizeEvent *) override;
    /**
    * \reimp
    */
    void contextMenuEvent(QContextMenuEvent*) override;

    void mouseReleaseEvent(QMouseEvent * /*e*/) override
        { emit clicked(); }

    void mouseDoubleClickEvent(QMouseEvent * /*e*/) override
        { emit doubleClicked(); }

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
