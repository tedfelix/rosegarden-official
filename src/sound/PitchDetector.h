/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef PITCHTRACKER_H
#define PITCHTRACKER_H


#include <math.h>
#include <complex>
#include <fftw3.h>
#include <fstream>

#include <QString>
#include <QVector>

#define MIN_THRESHOLD 1

namespace Rosegarden
{


//!!! I don't understand much of this class, so I've only updated the
//!!! member variables.  I don't vouch for it meeting your code standards. -gp

// We have made a design decision to use QString/QVector now rather than
// the equivalent types in std::. Originally it was a pain to use the Qt
// types; rather we'd link a library of microtonal stuff, and it would
// mean that our library had to depend on Qt classes. But RG seems to be
// moving towards a minimum dependence culture, and there are no libraries
// built from the source tree, so let's go with the Qt classes now. - njb


/**
 * \addtogroup Codicil
 * \@{
 * \brief Stores audio data from jack into a ring buffer
 *
 * This is part of the network for Interdisciplinary research in
 * Science and Music's "Rosegarden Codicil" project.
 * http://www.n-ism.org/Projects/microtonalism.php
 *
 * The most recent audio samples are read into a buffer. Unlike most audio
 * applications, it is more important to reduce latency by dropping samples
 * than to eventually process all of them. This means the estimated pitch
 * is as up-to-date as possible.
 *
 * The reference to "frame" in the cotext of a PitchDector
 * means an analysis frame (e.g. 1024 samples)
 * and not the more usual usage of the number of samples presented
 * simultaneously (1=mono, 2=stereo etc).
 *
 * \author Doug McGilvray (original author)
 * \author Graham Percival (slightly rewritten to match Rosegarden standards)
 * \author Nick Bailey (changed to use QSting, QVector instead of std::...)
 * \date 2004, rewrite 2009, 2010
 *
 */
class PitchDetector {

public:

    typedef QString Method;               // was std::string.
    
    static const double NOSIGNAL;         /**< No input signal available */
    static const double NONE;             /**< No pitch (e.g. in a rest ) */
    static const Method PARTIAL;          /**< Use lowest freq max */
    static const Method AUTOCORRELATION;  /**< Use autocorrelation delay */
    static const Method HPS;              /**< Use Harmonic Prd Spectrum */
    // The following not yet implemented
    //static const Method AMDF;
    //static const Method CEPSTRUM;
    
    /** 
     * Analysis frame size
     * A sensible default size of analysis buffer for all methods
     */
    static const int defaultFrameSize;

    /**
     * Analysis buffer overlap
     * Frequency resolution may be increased by considering the phase
     * drift between overlapping analysis frames. This is a sensible
     * default size for the overlap for all methods
     */
    static const int defaultStepSize;
    
    PitchDetector( int frameSize, int stepSize, int sampleRate );
    virtual ~PitchDetector();

    float *getInBuffer();                 /**< Get audio data buffer ref */
    double getPitch();                    /**< Get pitch; use current method */

    int getFrameSize() const;             /**< Get current audio buf size */
    void setFrameSize( int frameSize );   /**< Set current audio buf size */
    int getStepSize() const;              /**< Get no. samples between anals */
    void setStepSize( int stepSize );     /**< Set no, samples between anals */
    int getBufferSize() const;            /**< Get size of audio buffer */

    void setMethod( Method method );
    Method getCurrentMethod() {
        return m_method;
    }
    static const QVector<Method> *getMethods(); // was std::vector
    
private:

    float *m_frame;
    double partial();
    double amdf();
    double autocorrelation();
    double hps();
    double unwrapPhase( int fBin );

    class MethodVector : public QVector<Method> {
    public:
        MethodVector();
    };
    
    static const MethodVector m_methods;   // was std::vector
    
    float *m_cepstralIn, *m_in1, *m_in2;
    int m_frameSize;
    int m_stepSize;
    int m_sampleRate;

    Method m_method;
    fftwf_complex *m_ft1, *m_ft2, *m_cepstralOut;
    fftwf_plan m_p1, m_p2, m_pc;

};

/** \@} */
}
#endif
