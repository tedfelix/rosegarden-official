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

#define RG_MODULE_STRING "[AudioSplitDialog]"

#include "AudioSplitDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Exception.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenApplication.h"
#include "sound/AudioFileManager.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QGroupBox>
#include <QLabel>
#include <QPalette>
#include <QScrollArea>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QList>
#include <QGraphicsSimpleTextItem>


namespace Rosegarden
{


AudioSplitDialog::AudioSplitDialog(QWidget *parent,
                                   Segment *segment,
                                   RosegardenDocument *doc):
        QDialog(parent),
        m_doc(doc),
        m_segment(segment),
        m_sceneWidth(500),
        m_sceneHeight(200),
        m_previewWidth(400),
        m_previewHeight(100)
{
    if (!segment || segment->getType() != Segment::Audio)
        reject();

    setModal(true);
    // pre-pend "Rosegarden" to title in a way that preserves existing
    // translations unchanged:
    QString title = QString("%1 - %2").arg(tr("Rosegarden")).arg(tr("Autosplit Audio Segment"));
    setWindowTitle(title);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    //!!! Use "Autosplit" or "AutoSplit" but not both.  I'm ignoring this to
    // avoid string changes, but for 10.04 this should change to "Auto-split"
    // throughout, I think.
    QLabel *label = new QLabel(tr("AutoSplit Segment \"") + strtoqstr(m_segment->getLabel()) + QString("\""));
    layout->addWidget(label);

    QGroupBox *box = new QGroupBox;
    QVBoxLayout *boxLayout = new QVBoxLayout;
    box->setLayout(boxLayout);
    layout->addWidget(box);

    m_scene = new QGraphicsScene;
	
    m_view = new QGraphicsView(m_scene);
    boxLayout->addWidget(m_view);

    QWidget *hbox = new QWidget;
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hbox->setLayout(hboxLayout);
    boxLayout->addWidget(hbox);

    label = new QLabel(tr("Threshold"));
    hboxLayout->addWidget(label);
    m_thresholdSpin = new QSpinBox;
    hboxLayout->addWidget(m_thresholdSpin);
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

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &AudioSplitDialog::slotHelpRequested);
}

void
AudioSplitDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:audioSplitDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:audioSplitDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
AudioSplitDialog::noPreviewMsg()
{
    QGraphicsSimpleTextItem *text = 
        new QGraphicsSimpleTextItem(tr("<no preview generated for this audio file>"));
    text->setBrush(Qt::black);
    m_scene->addItem(text);
    text->setPos(30, 30);
}

void
AudioSplitDialog::drawPreview()
{
    // Delete everything in the scene
    //
    QList<QGraphicsItem *> list = m_scene->items();
    for (QList<QGraphicsItem *>::Iterator it = list.begin();
         it != list.end(); it++) {
        delete *it;
    }

    // empty the preview boxes
    m_previewBoxes.erase(m_previewBoxes.begin(), m_previewBoxes.end());
  
    // Draw a bounding box
    //
    int border = 5;
    m_scene->addRect(0, 0,
                     m_sceneWidth - border * 2,
                     m_sceneHeight - border * 2,
                     QPen(Qt::black, border),
                     Qt::white);

    // Get preview in vector form
    //
    AudioFileManager &aFM = m_doc->getAudioFileManager();
    AudioFile *aF = aFM.getAudioFile(m_segment->getAudioFileId());
    if (aF == nullptr) {
        noPreviewMsg();
        return;
    }
    int channels = aF->getChannels();

    std::vector<float> values;

    try {
        values = aFM.getPreview(m_segment->getAudioFileId(),
                                m_segment->getAudioStartTime(),
                                m_segment->getAudioEndTime(),
                                m_previewWidth,
                                false);
    } catch (const Exception &e) {
        noPreviewMsg();
        return ;
    }

    qreal startX = (m_sceneWidth - m_previewWidth) / 2;
    qreal halfHeight = m_sceneHeight / 2;
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


        qreal startY = halfHeight + h1 * float(m_previewHeight / 2);
        qreal endY = halfHeight - h2 * float(m_previewHeight / 2);

        if (startY < 0) {
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

        m_scene->addLine(startX + qreal(i),
                         startY,
                         startX + qreal(i),
                         endY,
                         QPen(Qt::black));
    }

    // Draw zero dc line
    //
    m_scene->addRect(startX,
                     halfHeight - 1,
                     qreal(m_previewWidth),
                     2,
                     QPen(Qt::black),
                     QBrush(Qt::gray));

    // Start time
    //
    char msecs[100];
    sprintf(msecs, "%03d", m_segment->getAudioStartTime().msec());
    QString startText = QString("%1.%2s")
                        .arg(m_segment->getAudioStartTime().sec)
                        .arg(msecs);
    QGraphicsSimpleTextItem *text = new QGraphicsSimpleTextItem(startText);
    text->setBrush(Qt::black);
    m_scene->addItem(text);
    text->setPos(startX - 20,
                 qreal(m_sceneHeight) / 2 - qreal(m_previewHeight) / 2 - 35);

    m_scene->addRect(startX - 1,
                     qreal(m_sceneHeight) / 2 - qreal(m_previewHeight) / 2 - 14,
                     1,
                     qreal(m_previewHeight) + 28,
                     QPen(Qt::black),
                     QBrush(Qt::gray));

    // End time
    //
    sprintf(msecs, "%03d", m_segment->getAudioEndTime().msec());
    QString endText = QString("%1.%2s")
                      .arg(m_segment->getAudioEndTime().sec)
                      .arg(msecs);
    text = new QGraphicsSimpleTextItem(endText);
    text->setBrush(Qt::black);
    m_scene->addItem(text);
    text->setPos(startX + qreal(m_previewWidth) - 20,
                 qreal(m_sceneHeight) / 2 - qreal(m_previewHeight) / 2 - 35);

    m_scene->addRect(startX + qreal(m_previewWidth) - 1,
                     qreal(m_sceneHeight) / 2 - qreal(m_previewHeight) / 2 - 14,
                     1,
                     qreal(m_previewHeight) + 28,
                     QPen(Qt::black),
                     QBrush(Qt::gray));
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
    std::vector<QGraphicsRectItem*> tempRects;

    RealTime length = endTime - startTime;
    double ticksPerUsec = double(m_previewWidth) /
                          double((length.sec * 1000000.0) + length.usec());

    qreal startX = (m_sceneWidth - m_previewWidth) / 2;
    qreal halfHeight = m_sceneHeight / 2;
    qreal x1, x2;
    qreal overlapHeight = 10;

    for (it = splitPoints.begin(); it != splitPoints.end(); ++it) {
        RealTime splitStart = it->first - startTime;
        RealTime splitEnd = it->second - startTime;

        x1 = ticksPerUsec * double(double(splitStart.sec) * 1000000.0 + (double)splitStart.usec());

        x2 = ticksPerUsec * double(double(splitEnd.sec) * 1000000.0 + double(splitEnd.usec()));

        QGraphicsRectItem *rect = m_scene->addRect(startX + x1,
                                                   halfHeight - qreal(m_previewHeight) / 2 - overlapHeight / 2,
                                                   x2 - x1, 
                                                   qreal(m_previewHeight) + overlapHeight,
                                                   QPen(Qt::red),
                                                   QBrush(Qt::blue));
        tempRects.push_back(rect);
    }

    std::vector<QGraphicsRectItem*>::iterator pIt;

    // We've written the new Rects, now delete the old ones
    //
    if (m_previewBoxes.size()) {
        // clear any previous preview boxes
        //
        for (pIt = m_previewBoxes.begin(); pIt != m_previewBoxes.end(); ++pIt) {
            //(*pIt)->setVisible(false);
            delete (*pIt);
        }
        m_previewBoxes.erase(m_previewBoxes.begin(), m_previewBoxes.end());
//        m_scene->update();
    }
//    m_scene->update();

    // Now store the new ones
    //
    for (pIt = tempRects.begin(); pIt != tempRects.end(); ++pIt)
        m_previewBoxes.push_back(*pIt);

}

void
AudioSplitDialog::slotThresholdChanged(int threshold)
{
    drawSplits(threshold);
}

}
