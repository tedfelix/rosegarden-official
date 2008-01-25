/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_AUDIOPREVIEWPAINTER_H_
#define _RG_AUDIOPREVIEWPAINTER_H_

#include "CompositionModel.h"

#include <qpainter.h>
#include <qcolor.h>

namespace Rosegarden {

class CompositionModelImpl;
class Composition;
class Segment;
class CompositionRect;

class AudioPreviewPainter {
public:
    AudioPreviewPainter(CompositionModelImpl& model,
			CompositionModel::AudioPreviewData* apData,
			const Composition &composition,
			const Segment* segment);

    void paintPreviewImage();
    PixmapArray getPreviewImage();
    const CompositionRect& getSegmentRect() { return m_rect; }

    static int tileWidth();

protected:
    void finalizeCurrentSlice();

    //--------------- Data members ---------------------------------
    CompositionModelImpl& m_model;
    CompositionModel::AudioPreviewData* m_apData;
    const Composition &m_composition;
    const Segment* m_segment;
    CompositionRect m_rect;

    QImage m_image;
    PixmapArray m_previewPixmaps;

    QPainter m_p;
    QPainter m_pb;
    QColor m_defaultCol;
    int m_penWidth;
    int m_height;
    int m_halfRectHeight;
    int m_sliceNb;

};

}

#endif

