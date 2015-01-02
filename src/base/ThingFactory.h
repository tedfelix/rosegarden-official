/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_THING_FACTORY_H
#define RG_THING_FACTORY_H

#include <QUrl>
#include <QSet>
#include <QHash>
#include <QMutex>
#include <QStringList>

#include "UrlHash.h"

namespace Rosegarden {

template <typename Thing, typename Parameters>
class AbstractThingBuilder;

class UnknownThingException { };
class UnknownTagException { };
class DuplicateThingException { };

/**
 * A factory for objects from classes that share a common base class,
 * have identical single-argument constructors, can be identified by
 * URI, and that can register their existence with the factory (so
 * that the factory does not have to know about all buildable
 * classes).
 *
 * ** How to use these classes **
 *
 * Given a base class A with many subclasses B, C, D, etc, all of
 * which need to be passed parameters class P in their constructor:
 *
 *  -- in a header associated with A,
 *
 *     - Create a template class ABuilder<T> which inherits from
 *       ConcreteThingBuilder<T, A, P>.  This is your class which will
 *       be specialised to provide a builder for each subclass of A.
 *       Its constructor must accept a QUrl containing the URI that
 *       identifies the class of object being built, which is passed
 *       to the parent class's constructor.  Optionally, it may also
 *       accept and pass to the parent class a QStringList of "tags",
 *       which are strings used to identify the sorts of facility this
 *       builder's object supports -- for example, file extensions or
 *       MIME types that the object can parse.  If two builders
 *       register support for the same tag, only the first to register
 *       will be used (note that which one this is may depend on
 *       static object construction ordering, so it's generally better
 *       if tags are unique to a builder).
 * 
 *     - You may also wish to typedef ThingFactory<A, P> to something
 *       like AFactory, for convenience.
 *
 *  -- in a .cpp file associated with each of B, C, D etc,
 *
 *     - Define a static variable of class ABuilder<B>, ABuilder<C>,
 *       ABuilder<D>, etc, passing the class's identifying URI and
 *       optional supported tag list to its constructor.  (If you
 *       like, this could be a static member of some class.)
 *
 * You can then do the following:
 *
 *  -- call AFactory::getInstance()->getURIs() to retrieve a list of
 *     all registered URIs for this factory.
 * 
 *  -- call AFactory::getInstance()->create(uri, parameters), where
 *     parameters is an object of type P, to construct a new object
 *     whose class is that associated with the URI uri.
 *
 *  -- call AFactory::getInstance()->getURIFor(tag), where tag is a
 *     QString corresponding to one of the tags supported by some
 *     builder, to obtain the URI of the first builder to have
 *     registered its support for the given tag.
 *
 *  -- call AFactory::getInstance()->createFor(tag, parameters), where
 *     tag is a QString corresponding to one of the tags supported by
 *     some builder and parameters is an object of type P, to
 *     construct a new object whose class is that built by the first
 *     builder to have registered its support for the given tag.
 */
template <typename Thing, typename Parameters>
class ThingFactory
{
protected:
    typedef AbstractThingBuilder<Thing, Parameters> Builder;
    typedef QHash<QUrl, Builder *> BuilderMap;
    typedef QHash<QString, QUrl> TagURIMap;

public:
    typedef QSet<QUrl> URISet;

    static ThingFactory<Thing, Parameters> *getInstance() {
	static QMutex mutex;
	QMutexLocker locker(&mutex);
	if (!m_instance) m_instance = new ThingFactory<Thing, Parameters>();
	return m_instance;
    }
    
    URISet getURIs() const {
	QList<QUrl> keys = m_registry.keys();
	URISet s;
	for (int i = 0; i < keys.size(); ++i) s.insert(keys[i]);
	return s;
    }
    
    QStringList getTags() const {
        return m_tags.keys();
    }
    
    QUrl getURIFor(QString tag) const {
        if (!m_tags.contains(tag)) {
            throw UnknownTagException();
        }
        return m_tags[tag];
    }

    Thing *create(QUrl uri, Parameters p) const {
	if (!m_registry.contains(uri)) {
	    throw UnknownThingException();
	}
	return m_registry[uri]->build(p);
    }

    Thing *createFor(QString tag, Parameters p) const {
        return create(getURIFor(tag), p);
    }

    void registerBuilder(QUrl uri, Builder *builder) {
        if (m_registry.contains(uri)) {
            throw DuplicateThingException();
        }
	m_registry[uri] = builder;
    }
    
    void registerBuilder(QUrl uri, Builder *builder, QStringList tags) {
        if (m_registry.contains(uri)) {
            throw DuplicateThingException();
        }
	m_registry[uri] = builder;
        for (int i = 0; i < tags.size(); ++i) {
            if (m_tags.contains(tags[i])) continue;
            m_tags[tags[i]] = uri;
        }
    }
    
protected:
    static ThingFactory<Thing, Parameters> *m_instance;
    BuilderMap m_registry;
    TagURIMap m_tags;
};

template <typename Thing, typename Parameters>
class AbstractThingBuilder
{
public:
    virtual ~AbstractThingBuilder() { }
    virtual Thing *build(Parameters) = 0;
};

template <typename ConcreteThing, typename Thing, typename Parameters>
class ConcreteThingBuilder : public AbstractThingBuilder<Thing, Parameters>
{
public:
    ConcreteThingBuilder(QUrl uri) {
	ThingFactory<Thing, Parameters>::getInstance()->registerBuilder(uri, this);
    }
    ConcreteThingBuilder(QUrl uri, QStringList tags) {
	ThingFactory<Thing, Parameters>::getInstance()->registerBuilder(uri, this, tags);
    }
    virtual Thing *build(Parameters p) {
	return new ConcreteThing(p);
    }
};

}

#endif
