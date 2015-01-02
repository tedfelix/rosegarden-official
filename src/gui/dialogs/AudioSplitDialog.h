/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOSPLITDIALOG_H
#define RG_AUDIOSPLITDIALOG_H

#include <QDialog>
#include <vector>
#include <QSpinBox>


class QWidget;
class QGraphicsScene;
class QGraphicsView;
class QGraphicsRectItem;


namespace Rosegarden
{

class Segment;
class RosegardenDocument;


class AudioSplitDialog : public QDialog
{
    Q_OBJECT
public:
    AudioSplitDialog(QWidget *parent,
                     Segment *segment,
                     RosegardenDocument *doc);

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

    void slotHelpRequested();

protected:
    RosegardenDocument             *m_doc;
    Segment                        *m_segment;
    QGraphicsScene                 *m_scene;
    QGraphicsView                  *m_view;
    QSpinBox                       *m_thresholdSpin;

    int                             m_sceneWidth;
    int                             m_sceneHeight;
    int                             m_previewWidth;
    int                             m_previewHeight;

    std::vector<QGraphicsRectItem*> m_previewBoxes;

    void noPreviewMsg();
};


}
#endif
