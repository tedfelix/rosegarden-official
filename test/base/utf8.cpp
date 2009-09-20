/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

#include "XmlExportable.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace Rosegarden;
using std::cout;
using std::cerr;
using std::endl;
using std::string;


string binary(unsigned char c)
{
    string s;
    for (int i = 0; i < 8; ++i) {
	s = ((c & 0x1) ? '1' : '0') + s;
	c >>= 1;
    }
    return s;
}


int main(int argc, char **argv)
{
    string valid[] = {
	"ニュース",
	"주요 뉴스",
	"Nyheter",
	"天气",
	"Notícias",
    };

    string escapable[] = {
	"ニュ&ース",
	"주요 <뉴스>",
	"\"Nyheter\"",
	"\'Notícias\'",
    };

    string invalid[] = {
	"����ース",
	"주� � 뉴스",
	"Nyhe\004ter",
	"�天气",
	"Not�cias",
    };

    cout << "Testing valid strings -- should be no errors here" << endl;

    for (size_t i = 0; i < sizeof(valid)/sizeof(valid[0]); ++i) {
	string encoded = XmlExportable::encode(valid[i]);
	if (encoded != valid[i]) {
	    cerr << "Encoding failed:" << endl;
	    for (size_t j = 0; j < valid[i].length(); ++j) {
		cerr << (char)valid[i][j] << " ("
		     << binary(valid[i][j]) << ")" << endl;
	    }
	    exit(1);
	}
    }

    cout << "Testing escapable strings -- should be no errors here" << endl;

    for (size_t i = 0; i < sizeof(escapable)/sizeof(escapable[0]); ++i) {
	string encoded = XmlExportable::encode(escapable[i]);
	if (encoded == escapable[i]) {
	    cerr << "Escaping failed:" << endl;
	    for (size_t j = 0; j < escapable[i].length(); ++j) {
		cerr << (char)escapable[i][j] << " ("
		     << binary(escapable[i][j]) << ")" << endl;
	    }
	    exit(1);
	}
    }

    cout << "Testing invalid strings -- should be "
	 << (sizeof(invalid)/sizeof(invalid[0]))
	 << " errors here (but no fatal ones)" << endl;

    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
	string encoded = XmlExportable::encode(invalid[i]);
	if (encoded == invalid[i]) {
	    cerr << "Encoding succeeded but should have failed:" << endl;
	    for (size_t j = 0; j < invalid[i].length(); ++j) {
		cerr << (char)invalid[i][j] << " ("
		     << binary(invalid[i][j]) << ")" << endl;
	    }
	    exit(1);
	}
    }

    exit(0);
}

