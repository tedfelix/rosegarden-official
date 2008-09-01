
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_AUDIOSPLITDIALOG_H_
#define _RG_AUDIOSPLITDIALOG_H_

#include <Q3Canvas>
#include <Q3CanvasRectangle>
#include <Q3CanvasView>
#include <QDialog>
#include <QDialogButtonBox>
#include <vector>
#include <QSpinBox>


class QWidget;
class Q3CanvasView;
class Q3CanvasRectangle;
class Q3Canvas;


namespace Rosegarden
{

class Segment;
class RosegardenGUIDoc;


class AudioSplitDialog : public QDialog
{
    Q_OBJECT
public:
    AudioSplitDialog(QDialogButtonBox::QWidget *parent,
                     Segment *segment,
                     RosegardenGUIDoc *doc);

    // Draw an audio preview over the segment and draw
    // the potential splits along it.
    //
    void drawPreview();
    void drawSplits(int threshold);

    // Get the threshold
    //
    int getThreshold() { return m_thresholdSpin->value(); }

public slots:
    void slotThresholdChanged(int);

protected:
    RosegardenGUIDoc              *m_doc;
    Segment           *m_segment;
    Q3Canvas                       *m_canvas;
    Q3CanvasView                   *m_canvasView;
    QSpinBox                      *m_thresholdSpin;

    int                            m_canvasWidth;
    int                            m_canvasHeight;
    int                            m_previewWidth;
    int                            m_previewHeight;

    std::vector<Q3CanvasRectangle*> m_previewBoxes;

};



}

#endif
