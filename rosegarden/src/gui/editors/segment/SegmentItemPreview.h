
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_SEGMENTITEMPREVIEW_H_
#define _RG_SEGMENTITEMPREVIEW_H_

#include <qrect.h>


class QWMatrix;
class QPainter;


namespace Rosegarden
{

class Segment;
class RulerScale;


//////////////////////////////////////////////////////////////////////
class SegmentItemPreview 
{
public:
    SegmentItemPreview(Segment& parent,
                       RulerScale* scale);
    virtual ~SegmentItemPreview();

    enum PreviewState {
        PreviewChanged,
        PreviewCalculating,
        PreviewCurrent
    };

    virtual void drawShape(QPainter&) = 0;

    PreviewState getPreviewState() const { return m_previewState; }

    /**
     * Sets whether the preview shape shown in the segment needs
     * to be refreshed
     */
    void setPreviewCurrent(bool c)
    { m_previewState = (c ? PreviewCurrent : PreviewChanged); }

    /**
     * Clears out the preview entirely so that it will be regenerated
     * next time
     */
    virtual void clearPreview() = 0;

    QRect rect();
    
protected:
    virtual void updatePreview(const QWMatrix &matrix) = 0;

    //--------------- Data members ---------------------------------

    Segment *m_segment;
    RulerScale *m_rulerScale;

    PreviewState m_previewState;
};



}

#endif
