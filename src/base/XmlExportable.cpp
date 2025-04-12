/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[XmlExportable]"

#include "XmlExportable.h"

#include "misc/Debug.h"

#include <cstdlib>
#include <cstring>

namespace
{
    class Deleter
    {
    public:
        explicit Deleter(char *p) : m_p(p)  { }
        ~Deleter()
        {
            std::free(m_p);
        }
    private:
        char *m_p;
    };
}

namespace Rosegarden
{


std::string XmlExportable::encode(const std::string &s0)
{
    static char *buffer = nullptr;
    // Make sure we don't leak.  This will free(buffer) at static
    // destruction time.
    static Deleter deleter(buffer);
    static size_t bufsiz = 0;

    size_t buflen = 0;

    static char multibyte[20];
    size_t mblen = 0;

    size_t len = s0.length();

    if (bufsiz < len * 2 + 10) {
        bufsiz = len * 2 + 10;
        buffer = (char *)std::realloc(buffer, bufsiz);
    }

    // Escape any xml special characters, and also make sure we have
    // valid utf8 -- otherwise we won't be able to re-read the xml.
    // Amazing how complicated this gets.

    // ??? QString::toHtmlEscaped() does part of this.  There are some
    //     QString UTF-8 functions that might be able to do the validation.
    //     Regardless, we should probably redo this with QString at some
    //     point.

    bool warned = false; // no point in warning forever for long bogus strings

    for (size_t i = 0; i < len; ++i) {

        unsigned char c = s0[i];

        if (((c & 0xc0) == 0xc0) || !(c & 0x80)) {

            // 11xxxxxx or 0xxxxxxx: first byte of a character sequence

            // ??? Duplicated code.  See below.
            if (mblen > 0) {

                // does multibyte contain a valid sequence?
                size_t length =
                    (!(multibyte[0] & 0x20)) ? 2 :
                    (!(multibyte[0] & 0x10)) ? 3 :
                    (!(multibyte[0] & 0x08)) ? 4 :
                    (!(multibyte[0] & 0x04)) ? 5 : 0;

                if (length == 0 || mblen == length) {
                    if (bufsiz < buflen + mblen + 1) {
                        bufsiz = 2 * buflen + mblen + 1;
                        buffer = (char *)std::realloc(buffer, bufsiz);
                    }
                    std::memcpy(buffer + buflen, multibyte, mblen);
                    buflen += mblen;
                } else {
                    if (!warned) {
                        RG_WARNING
                            << "WARNING: Invalid utf8 char width in string \""
                            << s0 << "\" at index " << i << " ("
                            << mblen << " octet"
                            << (mblen != 1 ? "s" : "")
                            << ", expected " << length << ")";
                        warned = true;
                    }
                    // and drop the character
                }
            }

            mblen = 0;

            if (!(c & 0x80)) { // ascii

                if (bufsiz < buflen + 10) {
                    bufsiz = 2 * buflen + 10;
                    buffer = (char *)std::realloc(buffer, bufsiz);
                }

                switch (c) {
                case '&' :  std::memcpy(buffer + buflen, "&amp;", 5); buflen += 5;  break;
                case '<' :  std::memcpy(buffer + buflen, "&lt;", 4); buflen += 4;  break;
                case '>' :  std::memcpy(buffer + buflen, "&gt;", 4); buflen += 4;  break;
                case '"' :  std::memcpy(buffer + buflen, "&quot;", 6); buflen += 6;  break;
                case '\'' : std::memcpy(buffer + buflen, "&apos;", 6); buflen += 6;  break;

                case 0x9:  // tab, \t
                case 0xa:  // line feed, \n
                case 0xd:  // carriage return, \r
                    // convert these special cases to plain whitespace:
                    buffer[buflen++] = ' ';
                    break;

                default:
                    if (c >= 32) buffer[buflen++] = c;
                    else {
                        if (!warned) {
                            RG_WARNING
                                << "WARNING: Invalid utf8 octet in string \""
                                << s0 << "\" at index " << i << " ("
                                << (int)c << " < 32)";
                        }
                        warned = true;
                    }
                }

            } else {

                // store in multibyte rather than straight to buffer, so
                // that we know we're in the middle of something
                // (below).  At this point we know mblen == 0.
                multibyte[mblen++] = c;
            }

        } else {

            // second or subsequent byte

            if (mblen == 0) { // ... without a first byte!
                if (!warned) {
                    RG_WARNING
                        << "WARNING: Invalid utf8 octet sequence in string \""
                        << s0 << "\" at index " << i;
                    warned = true;
                }
            } else {

                if (mblen >= sizeof(multibyte)-1) {
                    if (!warned) {
                        RG_WARNING
                            << "WARNING: Character too wide in string \""
                            << s0 << "\" at index " << i << " (reached width of "
                            << mblen << ")";
                    }
                    warned = true;
                    mblen = 0;
                } else {
                    multibyte[mblen++] = c;
                }
            }
        }
    }

    // ??? Duplicated code from above.  Either pull out a function or
    //     restructure the loop so that this is not needed.  E.g. use
    //     while instead of for and detect the terminating NULL.  Or
    //     do both if possible.

    if (mblen > 0) {
        // does multibyte contain a valid sequence?
        size_t length =
            (!(multibyte[0] & 0x20)) ? 2 :
            (!(multibyte[0] & 0x10)) ? 3 :
            (!(multibyte[0] & 0x08)) ? 4 :
            (!(multibyte[0] & 0x04)) ? 5 : 0;

        if (length == 0 || mblen == length) {
            if (bufsiz < buflen + mblen + 1) {
                bufsiz = 2 * buflen + mblen + 1;
                buffer = (char *)std::realloc(buffer, bufsiz);
            }
            std::memcpy(buffer + buflen, multibyte, mblen);
            buflen += mblen;
        } else {
            if (!warned) {
                RG_WARNING
                    << "WARNING: Invalid utf8 char width in string \""
                    << s0 << "\" at index " << len << " ("
                    << mblen << " octet"
                    << (mblen != 1 ? "s" : "")
                    << ", expected " << length << ")";
                warned = true;
            }
            // and drop the character
        }
    }

    buffer[buflen] = '\0';

    return buffer;
}


}
