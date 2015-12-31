/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "Resampler.h"
#include "base/Profiler.h"

#include <cstdlib>
#include <cmath>

#include <iostream>

#include <samplerate.h>

namespace Rosegarden {

class ResamplerImpl
{
public:
    virtual ~ResamplerImpl() { }
    
    virtual int resample(const float *const *const in, 
                         float *const *const out,
                         int incount,
                         float ratio,
                         bool final) = 0;
    
    virtual int resampleInterleaved(const float *const in, 
                                    float *const out,
                                    int incount,
                                    float ratio,
                                    bool final) = 0;

    virtual int getChannelCount() const = 0;

    virtual void reset() = 0;
};

namespace Resamplers {

class D_SRC : public ResamplerImpl
{
public:
    D_SRC(Resampler::Quality quality, int channels, int maxBufferSize,
          int m_debugLevel);
    ~D_SRC();

    int resample(const float *const *const in,
                 float *const *const out,
                 int incount,
                 float ratio,
                 bool final);

    int resampleInterleaved(const float *const in,
                            float *const out,
                            int incount,
                            float ratio,
                            bool final = false);

    int getChannelCount() const { return m_channels; }

    void reset();

protected:
    SRC_STATE *m_src;
    float *m_iin;
    float *m_iout;
    float m_lastRatio;
    int m_channels;
    int m_iinsize;
    int m_ioutsize;
    int m_debugLevel;
};

D_SRC::D_SRC(Resampler::Quality quality, int channels, int maxBufferSize,
             int debugLevel) :
    m_src(0),
    m_iin(0),
    m_iout(0),
    m_lastRatio(1.f),
    m_channels(channels),
    m_iinsize(0),
    m_ioutsize(0),
    m_debugLevel(debugLevel)
{
    if (m_debugLevel > 0) {
        std::cerr << "Resampler::Resampler: using libsamplerate implementation"
                  << std::endl;
    }

    int err = 0;
    m_src = src_new(quality == Resampler::Best ? SRC_SINC_BEST_QUALITY :
                    quality == Resampler::Fastest ? SRC_LINEAR :
                    SRC_SINC_FASTEST,
                    channels, &err);

    if (err) {
        std::cerr << "Resampler::Resampler: failed to create libsamplerate resampler: " 
                  << src_strerror(err) << std::endl;
        throw Resampler::ImplementationError; //!!! of course, need to catch this!
    }

    if (maxBufferSize > 0 && m_channels > 1) {
        m_iinsize = maxBufferSize * m_channels;
        m_ioutsize = maxBufferSize * m_channels * 2;
        m_iin = (float *)malloc(sizeof(float) * m_iinsize);
        m_iout = (float *)malloc(sizeof(float) * m_ioutsize);
    }

    reset();
}

D_SRC::~D_SRC()
{
    src_delete(m_src);
    if (m_iin) free(m_iin);
    if (m_iout) free(m_iout);
}

int
D_SRC::resample(const float *const *const in,
                float *const *const out,
                int incount,
                float ratio,
                bool final)
{
    SRC_DATA data;

    int outcount = lrintf(ceilf(incount * ratio));

    if (m_channels == 1) {
        data.data_in = const_cast<float *>(*in); //!!!???
        data.data_out = *out;
    } else {
        if (incount * m_channels > m_iinsize) {
            m_iin = (float *)realloc(m_iin, sizeof(float) * incount * m_channels);
            m_iinsize = incount * m_channels;
        }
        if (outcount * m_channels > m_ioutsize) {
            m_iout = (float *)realloc(m_iout, sizeof(float) * outcount * m_channels);
            m_ioutsize = outcount * m_channels;
        }
        int idx = 0;
        for (int i = 0; i < incount; ++i) {
            for (int j = 0; j < m_channels; ++j) {
                m_iin[idx++] = in[j][i];
            }
        }
        data.data_in = m_iin;
        data.data_out = m_iout;
    }

    data.input_frames = incount;
    data.output_frames = outcount;
    data.src_ratio = ratio;
    data.end_of_input = (final ? 1 : 0);

    int err = src_process(m_src, &data);

    if (err) {
        std::cerr << "Resampler::process: libsamplerate error: "
                  << src_strerror(err) << std::endl;
        throw Resampler::ImplementationError; //!!! of course, need to catch this!
    }

    if (m_channels > 1) {
        int idx = 0;
        for (int i = 0; i < data.output_frames_gen; ++i) {
            for (int j = 0; j < m_channels; ++j) {
                out[j][i] = m_iout[idx++];
            }
        }
    }

    m_lastRatio = ratio;

    return data.output_frames_gen;
}

int
D_SRC::resampleInterleaved(const float *const in,
                           float *const out,
                           int incount,
                           float ratio,
                           bool final)
{
    SRC_DATA data;

    int outcount = lrintf(ceilf(incount * ratio));

    data.data_in = const_cast<float *>(in);
    data.data_out = out;

    data.input_frames = incount;
    data.output_frames = outcount;
    data.src_ratio = ratio;
    data.end_of_input = (final ? 1 : 0);

    int err = src_process(m_src, &data);

    if (err) {
        std::cerr << "Resampler::process: libsamplerate error: "
                  << src_strerror(err) << std::endl;
        throw Resampler::ImplementationError; //!!! of course, need to catch this!
    }

    m_lastRatio = ratio;

    return data.output_frames_gen;
}

void
D_SRC::reset()
{
    src_reset(m_src);
}


} /* end namespace Resamplers */

Resampler::Resampler(Resampler::Quality quality, int channels,
                     int maxBufferSize, int debugLevel)
{
    d = new Resamplers::D_SRC(quality, channels, maxBufferSize, debugLevel);
}

Resampler::~Resampler()
{
    delete d;
}

int 
Resampler::resample(const float *const *const in,
                    float *const *const out,
                    int incount, float ratio, bool final)
{
    Profiler profiler("Resampler::resample");
    return d->resample(in, out, incount, ratio, final);
}

int 
Resampler::resampleInterleaved(const float *const in,
                               float *const out,
                               int incount, float ratio, bool final)
{
    Profiler profiler("Resampler::resample");
    return d->resampleInterleaved(in, out, incount, ratio, final);
}

int
Resampler::getChannelCount() const
{
    return d->getChannelCount();
}

void
Resampler::reset()
{
    d->reset();
}

}
