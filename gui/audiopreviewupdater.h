// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef AUDIOPREVIEWUPDATER_H
#define AUDIOPREVIEWUPDATER_H

#include <qobject.h>

#include "Segment.h"
#include "Composition.h"

#include "compositionview.h"

class AudioPreviewUpdater : public QObject
{
    Q_OBJECT

public:
    AudioPreviewUpdater(AudioPreviewThread &thread,
                        const Rosegarden::Composition& c,
                        const Rosegarden::Segment*,
                        const QRect&,
                        CompositionModel::AudioPreviewData& apData,
                        CompositionModelImpl* parent);
    bool update();

signals:
    void audioPreviewComplete(AudioPreviewUpdater*);

protected:
    virtual bool event(QEvent *);

    AudioPreviewThread& m_thread;

    const Rosegarden::Composition& m_composition;
    const Rosegarden::Segment*     m_segment;
    QRect                          m_rect;
    bool                           m_showMinima;

    CompositionModel::AudioPreviewData& m_apData;

    int m_previewToken;

};

#endif
