/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPreviewPainter]"

#include "AudioPreviewPainter.h"

#include "CompositionModelImpl.h"
#include "CompositionColourCache.h"
#include "base/Composition.h"
#include "base/Track.h"
#include "base/AudioLevel.h"
#include "base/Studio.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "gui/application/RosegardenMainWindow.h"

#include <QImage>
#include <QApplication>
#include <QSettings>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#include <QScreen>
#else
#include <QDesktopWidget>
#endif

namespace Rosegarden {

AudioPreviewPainter::AudioPreviewPainter(CompositionModelImpl& model,
					 CompositionModelImpl::AudioPeaks* apData,
					 const Composition &composition,
					 const Segment* segment)
    : m_model(model),
      m_apData(apData),
      m_composition(composition),
      m_segment(segment),
      m_rect(),
      m_defaultCol(CompositionColourCache::getInstance()->SegmentAudioPreview),
      m_height(model.grid().getYSnap()/2),
      m_sliceNb(0)
{
    model.getSegmentRect(*m_segment, m_rect);

    int pixWidth = std::min(m_rect.baseWidth, tileWidth());

    //NB. m_image used to be created as an 8-bit image with 4 bits per pixel.
    // QImage::Format_Indexed8 seems to be close enough, since we manipulate the
    // pixels directly by index, rather than employ drawing tools.
    m_image = QImage(pixWidth, m_rect.rect.height(), QImage::Format_Indexed8);
    m_penWidth = (std::max(1U, (unsigned int)m_rect.pen.width()) * 2);
    m_halfRectHeight = m_model.grid().getYSnap()/2 - m_penWidth / 2 - 2;
}

int AudioPreviewPainter::tileWidth()
{
    static int tw = -1;
    // Cached value available?  Return it.
    if (tw != -1)
        return tw;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QScreen* screen = RosegardenMainWindow::self()->screen();
    tw = screen->availableGeometry().width();
#else
    tw = QApplication::desktop()->width();
#endif
    return tw;
}

void AudioPreviewPainter::paintPreviewImage()
{
    const CompositionModelImpl::AudioPeaks::Values &values =
            m_apData->values;

    if (values.empty())
        return;

    float gain[2] = { 1.0, 1.0 };
    int instrumentChannels = 2;
    TrackId trackId = m_segment->getTrack();
    Track *track = m_model.getComposition().getTrackById(trackId);
    if (track) {
        Instrument *instrument = m_model.getStudio().getInstrumentById(track->getInstrument());
        if (instrument) {
            float level = AudioLevel::dB_to_multiplier(instrument->getLevel());
            float pan = instrument->getPan() - 100.0;
            gain[0] = level * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
            gain[1] = level * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
	    instrumentChannels = instrument->getNumAudioChannels();
        }
    }

    // This was always false.
    bool showMinima = false;  //m_apData->showsMinima();

    unsigned int channels = m_apData->channels;
    if (channels == 0) {
        RG_WARNING << "paintPreviewImage(): WARNING: problem with audio file for segment " << m_segment->getLabel().c_str();
        return;
    }

    int samplePoints = int(values.size()) / (channels * (showMinima ? 2 : 1));
    float h1, h2, l1 = 0, l2 = 0;
    double sampleScaleFactor = samplePoints / double(m_rect.baseWidth);
    m_sliceNb = 0;

    initializeNewSlice();

    int centre = m_image.height() / 2;

    //RG_DEBUG << "paintPreviewImage(): width = " << m_rect.baseWidth << ", height = " << m_rect.rect.height() << ", halfRectHeight = " << m_halfRectHeight;
    //RG_DEBUG << "paintPreviewImage(): channels = " << channels << ", gain left = " << gain[0] << ", right = " << gain[1];

    // double audioDuration = double(m_segment->getAudioEndTime().sec) +
    //     double(m_segment->getAudioEndTime().nsec) / 1000000000.0;

    // We need to take each pixel value and map it onto a point within
    // the preview.  We have samplePoints preview points in a known
    // duration of audioDuration.  Thus each point spans a real time
    // of audioDuration / samplePoints.  We need to convert the
    // accumulated real time back into musical time, and map this
    // proportionately across the segment width.

    RealTime startRT =
	m_model.getComposition().getElapsedRealTime(m_segment->getStartTime());
    double startTime = double(startRT.sec) + double(startRT.nsec) / 1000000000.0;

    RealTime endRT =
	m_model.getComposition().getElapsedRealTime(m_segment->getEndMarkerTime());
    double endTime = double(endRT.sec) + double(endRT.nsec) / 1000000000.0;

    bool haveTempoChange = false;

    int finalTempoChangeNumber =
	m_model.getComposition().getTempoChangeNumberAt
	(m_segment->getEndMarkerTime());

    if ((finalTempoChangeNumber >= 0) &&

	(finalTempoChangeNumber >
	 m_model.getComposition().getTempoChangeNumberAt
	 (m_segment->getStartTime()))) {

	haveTempoChange = true;
    }

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    bool meterLevels = (settings.value("audiopreviewstyle", 1).toUInt()
			== 1);

    for (int i = 0; i < m_rect.baseWidth; ++i) {

	// i is the x coordinate within the rectangle.  We need to
	// calculate the position within the audio preview from which
	// to draw the peak for this coordinate.  It's possible there
	// may be more than one, in which case we need to find the
	// peak of all of them.

	int position = 0;

	if (haveTempoChange) {

	    // First find the time corresponding to this i.
	    timeT musicalTime =
		m_model.grid().getRulerScale()->getTimeForX(m_rect.rect.x() + i);
	    RealTime realTime =
		m_model.getComposition().getElapsedRealTime(musicalTime);

	    double time = double(realTime.sec) +
		double(realTime.nsec) / 1000000000.0;
	    double offset = time - startTime;

	    if (endTime > startTime) {
		position = offset * m_rect.baseWidth / (endTime - startTime);
		position = int(channels * position);
	    }

	} else {

	    position = int(channels * i * sampleScaleFactor);
	}

        if (position < 0) continue;

        if (position >= int(values.size()) - int(channels)) {
            finalizeCurrentSlice();
            break;
        }

        if (channels == 1) {

            h1 = values[position++];
            h2 = h1;

            if (showMinima) {
                l1 = values[position++];
                l2 = l1;
            }
        } else {

            h1 = values[position++];
            if (showMinima) l1 = values[position++];

            h2 = values[position++];
            if (showMinima) l2 = values[position++];

        }

	if (instrumentChannels == 1 && channels == 2) {
	    h1 = h2 = (h1 + h2) / 2;
	    l1 = l2 = (l1 + l2) / 2;
	}

	h1 *= gain[0];
	h2 *= gain[1];

	l1 *= gain[0];
	l2 *= gain[1];

        // int width = 1;
	int pixel;

        // h1 left, h2 right
        if (h1 >= 1.0) { h1 = 1.0; pixel = 2; }
	else { pixel = 1; }

        int h;

	if (meterLevels) {
	    h = AudioLevel::multiplier_to_preview(h1, m_height);
	} else {
	    h = h1 * m_height;
	}
        if (h <= 0) h = 1;
	if (h > m_halfRectHeight) h = m_halfRectHeight;

        int rectX = i % tileWidth();

	for (int py = 0; py < h; ++py) {
	    m_image.setPixel(rectX, centre - py, pixel);
	}

        if (h2 >= 1.0) { h2 = 1.0; pixel = 2; }
        else { pixel = 1; }

	if (meterLevels) {
	    h = AudioLevel::multiplier_to_preview(h2, m_height);
	} else {
	    h = h2 * m_height;
	}
        if (h < 0) h = 0;

	for (int py = 0; py < h; ++py) {
	    m_image.setPixel(rectX, centre + py, pixel);
	}

        if (((i+1) % tileWidth()) == 0 || i == (m_rect.baseWidth - 1)) {
            finalizeCurrentSlice();
            initializeNewSlice();
        }
    }

/* Auto-fade not yet implemented.

    if (m_segment->isAutoFading()) {

        Composition &comp = m_model.getComposition();

        int audioFadeInEnd = int(
                                 m_model.grid().getRulerScale()->getXForTime(comp.
                                                                     getElapsedTimeForRealTime(m_segment->getFadeInTime()) +
                                                                     m_segment->getStartTime()) -
                                 m_model.grid().getRulerScale()->getXForTime(m_segment->getStartTime()));

        m_p.setPen(QColor(Qt::blue));
        m_p.drawRect(0,  m_apData->getSegmentRect().height() - 1, audioFadeInEnd, 1);
        m_pb.drawRect(0, m_apData->getSegmentRect().height() - 1, audioFadeInEnd, 1);
    }

    m_p.end();
    m_pb.end();
*/

    settings.endGroup();
}

void AudioPreviewPainter::initializeNewSlice()
{
    // transparent background
    m_image.setColor(0, qRgba(255, 255, 255, 0));

    // foreground from getPreviewColour()
    QColor c = m_segment->getPreviewColour();
    QRgb rgba = qRgba(c.red(), c.green(), c.blue(), 255);
    m_image.setColor(1, rgba);

    // red for clipping
    m_image.setColor(2, qRgba(255, 0, 0, 255));

    m_image.fill(0);
}

void AudioPreviewPainter::finalizeCurrentSlice()
{
//     RG_DEBUG << "AudioPreviewPainter::finalizeCurrentSlice : copying pixmap to image at " << m_sliceNb * tileWidth();

    m_previewPixmaps.push_back(m_image.copy());
    ++m_sliceNb;
}

CompositionModelImpl::QImageVector AudioPreviewPainter::getPreviewImage() const
{
    return m_previewPixmaps;
}

}
