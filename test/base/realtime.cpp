/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "RealTime.h"

#include <iostream>

using namespace Rosegarden;

bool test(int frame, int rate)
{
    RealTime rt = RealTime::frame2RealTime(frame, rate);
    int testframe = RealTime::realTime2Frame(rt, rate);
    if (testframe != frame) {
	std::cerr << "ERROR: at rate " << rate << ", frame " << frame
		  << " -> " << rt << " -> frame "
		  << testframe << std::endl;
	return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    int rates[] = { 7, 11025, 22050, 44100, 48000, 88200, 192000, 384000, 65521 };

    int good = 0, bad = 0;

    for (size_t i = 0; i < sizeof(rates)/sizeof(rates[0]); ++i) {

	int rate = rates[i];

	for (int v = -10; v < 10; ++v) {
	    if (test(v, rate)) ++good; else ++bad;
	}
	for (int v = -10; v < 10; ++v) {
	    if (test(v + rate, rate)) ++good; else ++bad;
	}
	for (int v = -10; v < 10; ++v) {
	    if (test(v * rate + v, rate)) ++good; else ++bad;
	}
    }

    std::cerr << "Passed: " << good << " / " << (good+bad) << " ("
	      << (good * 100) / (good+bad) << "%)" << std::endl;
}

