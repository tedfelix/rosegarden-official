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


#include <Q3Canvas>
#include <Q3CanvasItemList>
#include <Q3CanvasLine>
#include <Q3CanvasRectangle>
#include <Q3CanvasText>
#include <Q3CanvasView>
#include "AudioSplitDialog.h"
#include <QApplication>

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Exception.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/application/RosegardenApplication.h"
#include "sound/AudioFileManager.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <Q3Canvas>
#include <QLabel>
#include <QPalette>
#include <qscrollview.h>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

AudioSplitDialog::AudioSplitDialog(QDialogButtonBox::QWidget *parent,
                                   Segment *segment,
                                   RosegardenGUIDoc *doc):
        QDialog(parent),
        m_doc(doc),
        m_segment(segment),
        m_canvasWidth(500),
        m_canvasHeight(200),
        m_previewWidth(400),
        m_previewHeight(100)
{
    if (!segment || segment->getType() != Segment::Audio)
        reject();

    setModal(true);
    setWindowTitle(i18n("Autosplit Audio Segment"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *w = new QWidget(this);
    QVBoxLayout *wLayout = new QVBoxLayout;
    metagrid->addWidget(w, 0, 0);


    new QLabel(i18n("AutoSplit Segment \"") +
               strtoqstr(m_segment->getLabel()) + QString("\""), w);

    m_canvas = new Q3Canvas( w );
    wLayout->addWidget(m_canvas);
    m_canvas->resize(m_canvasWidth, m_canvasHeight);
    m_canvasView = new Q3CanvasView(m_canvas, w );
    wLayout->addWidget(m_canvasView);
    m_canvasView->setFixedWidth(m_canvasWidth);
    m_canvasView->setFixedHeight(m_canvasHeight);

    m_canvasView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_canvasView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_canvasView->setDragAutoScroll(false);

    QWidget *hbox = new QWidget( w );
    wLayout->addWidget(hbox);
    w->setLayout(wLayout);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    QLabel *child_3 = new QLabel(i18n("Threshold"), hbox );
    hboxLayout->addWidget(child_3);
    m_thresholdSpin = new QSpinBox( hbox );
    hboxLayout->addWidget(m_thresholdSpin);
    hbox->setLayout(hboxLayout);
    m_thresholdSpin->setSuffix(" %");
    connect(m_thresholdSpin, SIGNAL(valueChanged(int)),
            SLOT(slotThresholdChanged(int)));

    // ensure this is cleared
    m_previewBoxes.clear();

    // Set thresholds
    //
    int threshold = 1;
    m_thresholdSpin->setValue(threshold);
    drawPreview();
    drawSplits(1);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
AudioSplitDialog::drawPreview()
{
    // Delete everything on the canvas
    //
    Q3CanvasItemList list = m_canvas->allItems();
    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); it++)
        delete *it;

    // empty the preview boxes
    m_previewBoxes.erase(m_previewBoxes.begin(), m_previewBoxes.end());

    // Draw a bounding box
    //
    int border = 5;
    Q3CanvasRectangle *rect = new Q3CanvasRectangle(m_canvas);
    rect->setSize(m_canvasWidth - border * 2, m_canvasHeight - border * 2);
    rect->setX(border);
    rect->setY(border);
    rect->setZ(1);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));
    rect->setBrush(kapp->palette().color(QPalette::Active, QColorGroup::Base));
    rect->setVisible(true);

    // Get preview in vector form
    //
    AudioFileManager &aFM = m_doc->getAudioFileManager();
    int channels = aFM.getAudioFile(m_segment->getAudioFileId())->getChannels();

    std::vector<float> values;

    try {
        values = aFM.getPreview(m_segment->getAudioFileId(),
                                m_segment->getAudioStartTime(),
                                m_segment->getAudioEndTime(),
                                m_previewWidth,
                                false);
    } catch (Exception e) {
        Q3CanvasText *text = new Q3CanvasText(m_canvas);
        text->setColor(kapp->palette().
                       color(QPalette::Active, QColorGroup::Shadow));
        text->setText(i18n("<no preview generated for this audio file>"));
        text->setX(30);
        text->setY(30);
        text->setZ(4);
        text->setVisible(true);
        m_canvas->update();
        return ;
    }

    int startX = (m_canvasWidth - m_previewWidth) / 2;
    int halfHeight = m_canvasHeight / 2;
    float h1, h2;
    std::vector<float>::iterator it = values.begin();

    // Draw preview
    //
    for (int i = 0; i < m_previewWidth; i++) {
        if (channels == 1) {
            h1 = *(it++);
            h2 = h1;
        } else {
            h1 = *(it++);
            h2 = *(it++);
        }


        int startY = halfHeight + int(h1 * float(m_previewHeight / 2));
        int endY = halfHeight - int(h2 * float(m_previewHeight / 2));

        if ( startY < 0 ) {
            RG_DEBUG << "AudioSplitDialog::AudioSplitDialog - "
            << "startY - out of negative range"
            << endl;
            startY = 0;
        }

        if (endY < 0) {
            RG_DEBUG << "AudioSplitDialog::AudioSplitDialog - "
            << "endY - out of negative range"
            << endl;
            endY = 0;
        }

        Q3CanvasLine *line = new Q3CanvasLine(m_canvas);
        line->setPoints(startX + i,
                        startY,
                        startX + i,
                        endY);
        line->setZ(3);
        line->setPen(kapp->
                     palette().color(QPalette::Active, QColorGroup::Shadow));
        line->setBrush(kapp->
                       palette().color(QPalette::Active, QColorGroup::Shadow));
        line->setVisible(true);

    }

    // Draw zero dc line
    //
    rect = new Q3CanvasRectangle(m_canvas);
    rect->setX(startX);
    rect->setY(halfHeight - 1);
    rect->setSize(m_previewWidth, 2);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setBrush(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setZ(4);
    rect->setVisible(true);

    // Start time
    //
    char msecs[100];
    sprintf(msecs, "%03d", m_segment->getAudioStartTime().msec());
    QString startText = QString("%1.%2s")
                        .arg(m_segment->getAudioStartTime().sec)
                        .arg(msecs);
    Q3CanvasText *text = new Q3CanvasText(m_canvas);
    text->setColor(
        kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    text->setText(startText);
    text->setX(startX - 20);
    text->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 35);
    text->setZ(3);
    text->setVisible(true);

    rect = new Q3CanvasRectangle(m_canvas);
    rect->setX(startX - 1);
    rect->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 14);
    rect->setSize(1, m_previewHeight + 28);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setZ(3);
    rect->setVisible(true);

    // End time
    //
    sprintf(msecs, "%03d", m_segment->getAudioEndTime().msec());
    QString endText = QString("%1.%2s")
                      .arg(m_segment->getAudioEndTime().sec)
                      .arg(msecs);
    text = new Q3CanvasText(m_canvas);
    text->setColor(
        kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    text->setText(endText);
    text->setX(startX + m_previewWidth - 20);
    text->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 35);
    text->setZ(3);
    text->setVisible(true);

    rect = new Q3CanvasRectangle(m_canvas);
    rect->setX(startX + m_previewWidth - 1);
    rect->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 14);
    rect->setSize(1, m_previewHeight + 28);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setZ(3);
    rect->setVisible(true);

    m_canvas->update();
}

void
AudioSplitDialog::drawSplits(int threshold)
{
    // Now get the current split points and paint them
    //
    RealTime startTime = m_segment->getAudioStartTime();
    RealTime endTime = m_segment->getAudioEndTime();

    AudioFileManager &aFM = m_doc->getAudioFileManager();
    std::vector<SplitPointPair> splitPoints =
        aFM.getSplitPoints(m_segment->getAudioFileId(),
                           startTime,
                           endTime,
                           threshold);

    std::vector<SplitPointPair>::iterator it;
    std::vector<Q3CanvasRectangle*> tempRects;

    RealTime length = endTime - startTime;
    double ticksPerUsec = double(m_previewWidth) /
                          double((length.sec * 1000000.0) + length.usec());

    int startX = (m_canvasWidth - m_previewWidth) / 2;
    int halfHeight = m_canvasHeight / 2;
    int x1, x2;
    int overlapHeight = 10;

    for (it = splitPoints.begin(); it != splitPoints.end(); it++) {
        RealTime splitStart = it->first - startTime;
        RealTime splitEnd = it->second - startTime;

        x1 = int(ticksPerUsec * double(double(splitStart.sec) *
                                       1000000.0 + (double)splitStart.usec()));

        x2 = int(ticksPerUsec * double(double(splitEnd.sec) *
                                       1000000.0 + double(splitEnd.usec())));

        Q3CanvasRectangle *rect = new Q3CanvasRectangle(m_canvas);
        rect->setX(startX + x1);
        rect->setY(halfHeight - m_previewHeight / 2 - overlapHeight / 2);
        rect->setZ(2);
        rect->setSize(x2 - x1, m_previewHeight + overlapHeight);
        rect->setPen(kapp->
                     palette().color(QPalette::Active, QColorGroup::Mid));
        rect->setBrush(kapp->
                       palette().color(QPalette::Active, QColorGroup::Mid));
        rect->setVisible(true);
        tempRects.push_back(rect);
    }

    std::vector<Q3CanvasRectangle*>::iterator pIt;

    // We've written the new Rects, now delete the old ones
    //
    if (m_previewBoxes.size()) {
        // clear any previous preview boxes
        //
        for (pIt = m_previewBoxes.begin(); pIt != m_previewBoxes.end(); pIt++) {
            //(*pIt)->setVisible(false);
            delete (*pIt);
        }
        m_previewBoxes.erase(m_previewBoxes.begin(), m_previewBoxes.end());
        m_canvas->update();
    }
    m_canvas->update();

    // Now store the new ones
    //
    for (pIt = tempRects.begin(); pIt != tempRects.end(); pIt++)
        m_previewBoxes.push_back(*pIt);
}

void
AudioSplitDialog::slotThresholdChanged(int threshold)
{
    drawSplits(threshold);
}

}
#include "AudioSplitDialog.moc"
