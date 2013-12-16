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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <QObject>
#include <QVector>

#include "PitchDetector.h"

#define DEBUG_PT 0

namespace Rosegarden
{

const PitchDetector::Method PitchDetector::PARTIAL =
        QObject::tr("Partial", "Frequency Component (DSP)");
const PitchDetector::Method PitchDetector::AUTOCORRELATION =
        QObject::tr("Autocorrelation", "DSP operation");
const PitchDetector::Method PitchDetector::HPS =
        QObject::tr("Harmonic Product Spectrum", "Pitch determination (DSP)");
const double PitchDetector::NOSIGNAL = -1;
const double PitchDetector::NONE = -2;
const int PitchDetector::defaultFrameSize = 1024;
const int PitchDetector::defaultStepSize = 256;

const PitchDetector::MethodVector PitchDetector::m_methods;

PitchDetector::MethodVector::MethodVector() {
    append( AUTOCORRELATION );
    append( HPS );
    append( PARTIAL );
}

PitchDetector::PitchDetector( int fs, int ss, int sr ) {

    m_frameSize = fs;
    m_stepSize = ss;
    m_sampleRate = sr;

    m_frame = (float *)malloc( sizeof(float) * (m_frameSize+m_stepSize) );

    // allocate fft buffers
    m_in1 = (float *)fftwf_malloc(sizeof(float) * (m_frameSize) );
    m_in2 = (float *)fftwf_malloc(sizeof(float) * (m_frameSize) );
    m_ft1 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_frameSize );
    m_ft2 = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_frameSize );

    //for cepstrum
    m_cepstralIn = (float *)fftwf_malloc(sizeof(float) * m_frameSize );
    m_cepstralOut = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * m_frameSize );

    // create fft plans
    m_p1= fftwf_plan_dft_r2c_1d( m_frameSize, m_in1, m_ft1, FFTW_MEASURE );
    m_p2= fftwf_plan_dft_r2c_1d( m_frameSize, m_in2, m_ft2, FFTW_MEASURE );

    //for autocorrelation
    m_pc= fftwf_plan_dft_r2c_1d( m_frameSize, m_cepstralIn, m_cepstralOut, FFTW_MEASURE );

    //set default method
    m_method = AUTOCORRELATION;
}

const QVector<PitchDetector::Method>* PitchDetector::getMethods() {
    return &m_methods;
}

void PitchDetector::setMethod( PitchDetector::Method method ) {
#if DEBUG_PT
    std::cout << "PitchDetector::setMethod " << method << std::endl;
#endif

    if (m_methods.contains(method)) {
        m_method = method;
    }
    else {
#if DEBUG_PT
        std::cout << "PitchDetector::setMethod Not a method!\n";
#endif
    }
}

double PitchDetector::getPitch() {
//     return 446.0;
    double freq = 0;
//    double f2 = 0;   // not used?

    // Fill input buffers with data for two overlapping frames.
    for ( int c=0; c<m_frameSize; c++ ) {
        double window = 0.5 - 0.5*( cos(2*M_PI*c/m_frameSize) );
        m_in1[c] = m_frame[c] * window;
        m_in2[c] = m_frame[c+m_stepSize] *window ;
    }
    // Perform DFT
    fftwf_execute( m_p1 );
    fftwf_execute( m_p2 );
    if ( m_method == AUTOCORRELATION )
        freq = autocorrelation();
    else if ( m_method == HPS )
        freq = hps();
    else if ( m_method == PARTIAL )
        freq = partial();
    else {
        return 0;
    }

#if DEBUG_PT
    std::cout << "Freq " << freq << std::endl;
#endif
    return freq;
}

float *PitchDetector::getInBuffer() {
    return m_frame;
}

PitchDetector::~PitchDetector() {
    fftwf_free(m_in1);
    fftwf_free(m_in2);
    fftwf_free(m_ft1);
    fftwf_free(m_ft2);
    fftwf_free(m_cepstralIn);
    fftwf_free(m_cepstralOut);
    fftwf_destroy_plan(m_p1);
    fftwf_destroy_plan(m_p2);
    fftwf_destroy_plan(m_pc);
}

/**
   Calculates autocorrelation from FFT using Wiener-Khinchin Theorem.
*/

double PitchDetector::autocorrelation() {
    double value = 0;
    int bin = 0;

    /*
      Do 2nd FT on abs of original FT. Cepstrum would use Log
      instead of square
    */
    for ( int c=0; c<m_frameSize/2; c++ ) {
        value = abs(std::complex<double>(m_ft1[c][0], m_ft1[c][1]))/m_frameSize; // normalise
        m_cepstralIn[c] = value;
        m_cepstralIn[(m_frameSize - 1)-c] = 0;//value; //fills second half of fft
    }
    fftwf_execute( m_pc );

    // search for peak after first trough
//    double oldValue = 0;   // not used?
    value = 0;
    double max = 0;

    int c=0;

    double buff[m_frameSize/2];
    //fill buffer with magnitudes
    for ( int i=0; i<m_frameSize/2; i++) {
        buff[i] = abs( std::complex<double>(m_cepstralOut[i][0],m_cepstralOut[i][1]) );
    }


    double smoothed[m_frameSize/2];
    for ( int i=0; i<10; i++) smoothed[i]=0;
    for ( int i=m_frameSize/2-10; i<m_frameSize/2; i++) smoothed[i]=0;

    for (int i=10; i<(m_frameSize/2)-10; i++ ) {
        smoothed[i] = 0;
        for ( int x=-10; x<=10; x++ )
            smoothed[i] += buff[i+x];
        smoothed[i] /=21;
    }

    // find end of peak in smoothed buffer (c must atart after smoothing)
    // starts at 50
    for ( c=30; c<(m_frameSize/4) && smoothed[c] >= smoothed[c+1]; c++ ) {
    }
    if ( c >= m_frameSize/4 ) {
#if DEBUG_PT
        std::cout << "error: no end to first peak\n";
#endif
        return NOSIGNAL;
    }

    max=0;
    //find next peak from bin 30 (1500Hz) to 588 (75Hz)
    for ( int i=0; i<m_frameSize/2; i++ ) {
        value =  smoothed[i];
        if ( i>c && i<588 && value > max ) {
            max = value;
            bin = i;
        }
    }

    //std::cout << "max " << max << std::endl;

    if ( bin == 0 ) { // avoids occaisional exception when bin==0
#if DEBUG_PT
        std::cout << "bin = 0???\n";
#endif
        return NOSIGNAL;
    }

    int FTbin = round((double)m_frameSize/bin);
//    double fpb = (double)m_sampleRate/(double)m_frameSize;  // no used?


    std::complex<double> cValue = 0;
    double fMag = 0;

    fMag = 0;
    //find localised partial
    for ( int c=FTbin-2; c<FTbin+2 && c<m_frameSize/2; c++ ) {
        cValue = std::complex<double>(m_ft1[c][0], m_ft1[c][1]);
        if (fMag < abs(cValue)) {
            fMag = abs(cValue);
        }
    }

#if DEBUG_PT
    std::cout << "ACbin " << bin
              << "\tFTbin " << FTbin
              << "\tPeak FTBin " << fBin
              << std::endl;
#endif

    return unwrapPhase( FTbin );
}


/**
   Performs Harmonic Product Spectrum frequency estimation
*/
double PitchDetector::hps() {
    double max = 0;
    int fBin = 0;

    //calculate max HPS - only covering 1/6 of framesize
    //downsampling by factor of 3 * 1/2 of framesize
    for ( int i=0; i<m_frameSize/6; i++ ) {
        int i2 = 2*i;
        int i3 = 3*i;
        double hps =
            abs( std::complex<double>(m_ft1[i][0], m_ft1[i][1]) ) +
            0.8*abs( std::complex<double>(m_ft1[i2][0], m_ft1[i2][1]) ) +
            0.6*abs( std::complex<double>(m_ft1[i3][0], m_ft1[i3][1]) );

        if ( max < hps ) {
            max = hps;
            fBin = i;
        }
    }

    //std::cout << "bin = " << fBin << std::endl;

    return unwrapPhase( fBin );
}


/**
    Calculates exact frequency of component in fBin.
    Assumes FFT has already been applied to both
*/
double PitchDetector::unwrapPhase( int fBin ) {

    double oldPhase, fPhase;
    if ( abs( std::complex<double>(m_ft1[fBin][0], m_ft1[fBin][1]) ) < MIN_THRESHOLD )
        return NOSIGNAL;

    std::complex<double> cVal = std::complex<double>(m_ft1[fBin][0], m_ft1[fBin][1]);
    oldPhase = arg(cVal);

    cVal = std::complex<double>(m_ft2[fBin][0], m_ft2[fBin][1]);
    fPhase = arg(cVal);


    double freqPerBin = (double)m_sampleRate/(double)m_frameSize;
    double cf = fBin*freqPerBin;
    double phaseChange = fPhase-oldPhase;
    double expected = cf*(double)m_stepSize/(double)m_sampleRate;

    double phaseDiff = phaseChange/(2.0*M_PI) - expected;
    phaseDiff -= floor(phaseDiff);

    if ( (phaseDiff -= floor(phaseDiff)) > 0.5 )
        phaseDiff -= 1;

    phaseDiff *= 2*M_PI;

    double freqDiff = phaseDiff*freqPerBin*((double)m_frameSize/(double)m_stepSize)/(2*M_PI);

    double freq = cf + freqDiff;

#if DEBUG_PT
    std::cout << "Bin=" << fBin
              << "\tFPB="  << freqPerBin
              << "\tcf="   << cf
              << "\tfreq=" << freq
              << "\toPh="  << oldPhase
              << "\tnPh="  << fPhase
              << "\tphdf=" <<phaseChange
              << "\texpc=" << expected
              << std::endl;
#endif

    return freq;
}

/**
   Searches for highest component in frequency domain.
*/

double PitchDetector::partial() {
    int fBin = 0;
    std::complex<double> value = 0;
    double fMag;
    double fPhase = 0;
    double oldPhase = 0;

    fMag = 0;
    // find maximum input for first fft (in range)
    // for ( int c=4; c<200; c++ ) { // Why 200?? Why not m_frameSize/2??
    for (int c = 4 ; c < m_frameSize/2 ; c++ ) {
        value = std::complex<double>(m_ft1[c][0], m_ft1[c][1]);
        if (fMag < abs(value)) {
            fMag = abs(value);
            oldPhase = arg(value);
        }

    }

    fMag = 0;
    for (int c = 4 ; c < m_frameSize/2 ; c++) {
        value = std::complex<double>(m_ft2[c][0], m_ft2[c][1]);
        if (fMag < abs(value)) {
            fMag = abs(value);
            fPhase = arg(value);
        }
    }
    // If magnitude below threshold return -1
    if (fMag < MIN_THRESHOLD)
        return NOSIGNAL;

    double freqPerBin = (double)m_sampleRate/(double)m_frameSize;
    double cf = (double)fBin*freqPerBin;
    double phaseChange = fPhase-oldPhase;
    double expected = cf*(double)m_stepSize/(double)m_sampleRate;

    double phaseDiff = phaseChange/(2.0*M_PI) - expected;
    phaseDiff -= floor(phaseDiff);

    if ( (phaseDiff -= floor(phaseDiff)) > 0.5 )
        phaseDiff -= 1.0;

    phaseDiff *= 2.0*M_PI;

    double freqDiff = phaseDiff*freqPerBin*((double)m_frameSize/(double)m_stepSize)/(2.0*M_PI);

    double freq = cf + freqDiff;

#if DEBUG_PT
    std::cout << "P  bin=" << oldBin
              << "\tFPB="  << freqPerBin
              << "\tcf="   << cf
              << "\tfreq=" << freq
              << "\tphdf=" << phaseChange
              << "\texpc=" << expected
              << "\tur " << unwrapPhase(oldBin)
              << std::endl;
#endif
    return freq;

}

int PitchDetector::getFrameSize() const {
    return m_frameSize;
}

void PitchDetector::setFrameSize(int nextFrameSize) {
    m_frameSize = nextFrameSize;
}

int PitchDetector::getStepSize() const {
    return m_stepSize;
}
void PitchDetector::setStepSize(int nextStepSize) {
    m_stepSize = nextStepSize;
}

int PitchDetector::getBufferSize() const {
    return m_frameSize + m_stepSize;
}

}
