#include <cstdio>

#include "Event.h"

string
PropertyDefn<Int>::name()
{
    return "Int";
}    

PropertyDefn<Int>::basic_type
PropertyDefn<Int>::parse(string s)
{
    return atoi(s.c_str());
}

string
PropertyDefn<Int>::unparse(PropertyDefn<Int>::basic_type i)
{
    static char buffer[20]; sprintf(buffer, "%ld", i);
    return buffer;
}

string
PropertyDefn<String>::name()
{
    return "String";
}    

PropertyDefn<String>::basic_type
PropertyDefn<String>::parse(string s)
{
    return s;
}

string
PropertyDefn<String>::unparse(PropertyDefn<String>::basic_type i)
{
    return i;
}

string
PropertyDefn<Bool>::name()
{
    return "Bool";
}    

PropertyDefn<Bool>::basic_type
PropertyDefn<Bool>::parse(string s)
{
    return s == "true";
}

string
PropertyDefn<Bool>::unparse(PropertyDefn<Bool>::basic_type i)
{
    return (i ? "true" : "false");
}



PropertyStoreBase::~PropertyStoreBase()
{
}


Event::Event()
    : m_duration(0),
      m_absoluteTime(0),
      m_viewElements(0)
{
}

Event::Event(const string &package, const string &type)
    : m_package(package),
      m_type(type),
      m_duration(0),
      m_absoluteTime(0),
      m_viewElements(0)
{
}

Event::Event(const Event &e)
    : m_package(e.package()),
      m_type(e.type()),
      m_duration(e.duration()),
      m_absoluteTime(e.absoluteTime()),
      m_viewElements(0)
{
    copyFrom(e);
}

Event::~Event()
{
    scrapMap();
    delete m_viewElements;
}

Event&
Event::operator=(const Event &e)
{
    if (&e != this) {
        copyFrom(e);
    }
    return *this;
}    

bool
Event::has(const string &name) const
{
    PropertyMap::const_iterator i = m_properties.find(name);
    return (i != m_properties.end());
}

void
Event::scrapMap()
{
    for (PropertyMap::iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        delete (*i).second;
    }
}

void
Event::copyFrom(const Event &e)
{
    delete m_viewElements;
    scrapMap();

    for (PropertyMap::const_iterator i = e.m_properties.begin();
         i != e.m_properties.end(); ++i) {
        m_properties.insert(PropertyPair((*i).first, (*i).second->clone()));
    }

    if (e.viewElements())
        m_viewElements = new ViewElements(*(e.viewElements()));

}


void
Event::setViewElements(ViewElements *ve)
{
    if (m_viewElements) {
        // this will also delete all elements in m_group
        delete m_viewElements;
    }
    
    m_viewElements = ve;
}

void
Event::dump(ostream& out) const
{
#ifndef NDEBUG
    if (m_package.length()) {
        out << "Event type : " << m_type << " - package : "
            << m_package << '\n';
    } else {    
        out << "Event type : " << m_type.c_str() << '\n';
    }

    out << "\tDuration : " << m_duration
        << "\n\tAbsolute Time : " << m_absoluteTime
        << "\n\tProperties : \n";

    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        out << "\t\t" << (*i).first << '\t'
            << *((*i).second) << '\n';
    }
#endif
}

bool
operator<(const Event &a, const Event &b)
{
    return a.absoluteTime() < b.absoluteTime();
}

//////////////////////////////////////////////////////////////////////

ViewElement::ViewElement(Event *e)
    : m_event(e)
{
}

ViewElement::~ViewElement()
{
}

//////////////////////////////////////////////////////////////////////

Track::Track(unsigned int nbBars, unsigned int startIdx)
    : multiset<Event*, EventCmp>(),
    m_startIdx(startIdx),
    m_nbBars(nbBars)
{
    unsigned int initialTime = m_startIdx;
    
    // fill up with whole-note rests
    //
    for (unsigned int i = 0; i < nbBars; ++i) {
        Event *e = new Event;
        e->setType("rest");
        e->setTimeDuration(384); // TODO : get rid of this magic number
        e->setAbsoluteTime(initialTime);
        insert(e);
        initialTime += 384; // btw, it comes from xmlstorableevent.cpp
    }
}

Track::~Track()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it)
        delete (*it);
}

unsigned int
Track::getNbBars() const
{
    const_iterator lastEl = end();
    --lastEl;
    unsigned int nbBars = ((*lastEl)->absoluteTime() + (*lastEl)->duration()) / 384;

    return nbBars;
}

//////////////////////////////////////////////////////////////////////

ViewElements::~ViewElements()
{
    // delete content
    for(iterator it = begin(); it != end(); ++it) {
        cerr << "ViewElements delete" << endl;
        delete (*it);
    }

}


bool
operator<(const ViewElement &a, const ViewElement &b)
{
    return a.event()->absoluteTime() < b.event()->absoluteTime();
}

//////////////////////////////////////////////////////////////////////

Composition::Composition(unsigned int nbTracks)
    : m_tracks(nbTracks),
      m_nbTicksPerBar(384)
{
//     cerr << "Composition:(" << nbTracks << ") : this = "
//          << this <<  " - size = "
//          << m_tracks.size() << endl;
}

Composition::~Composition()
{
    clear();
}



bool
Composition::addTrack(Track *track = 0, int idx = -1)
{
//     cerr << "Composition::addTrack(track = " << track << ", "
//          << idx << ")\n";

    if (!track) track = new Track;
    
    if (idx < 0) {

        m_tracks.push_back(track);

    } else {

        if (idx >= m_tracks.size()) { // resize if needed

            m_tracks.resize(idx + 2);

        } else if (m_tracks[idx]) {
//             cerr << "Composition::addTrack() : there's already a track at this index\n";
            return false; // there's already a track at
            // that index
        }

//         cerr << "Composition::addTrack() : adding track at idx "
//              << idx << endl;
        m_tracks[idx] = track;
    }

    return true;
}

void
Composition::deleteTrack(int idx)
{
    if (idx < m_tracks.size()) {
        delete m_tracks[idx];
        m_tracks[idx] = 0;
    }
}

unsigned int
Composition::getNbBars() const
{
    unsigned int maxSize = 0,
        maxNbBars = 0;

    for (trackcontainer::const_iterator i = m_tracks.begin();
         i != m_tracks.end(); ++i) {

        if ((*i) && (*i)->size() > maxSize) {
            maxSize = (*i)->size();

            Track::const_iterator lastEl = (*i)->end();
            --lastEl;
            maxNbBars = ((*lastEl)->absoluteTime() + (*lastEl)->duration()) / getNbTicksPerBar();

//             cerr << "Composition::getNbBars() : last el. abs.Time : "
//                  << (*lastEl)->absoluteTime()
//                  << " - nbTicksPerBar : "
//                  << getNbTicksPerBar()
//                  << " - maxNbBars : " << maxNbBars << endl;
        }
    }
    
    return maxNbBars;
}

void
Composition::clear()
{
    for(trackcontainer::iterator i = m_tracks.begin();
        i != m_tracks.end(); ++i) {
        delete (*i);
        (*i) = 0;
    }
}

