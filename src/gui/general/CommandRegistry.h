/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_COMMANDREGISTRY_H_
#define _RG_COMMANDREGISTRY_H_

#include "document/BasicSelectionCommand.h"
#include "EditView.h"

#include <qobject.h>
#include <qstring.h>
#include <qnamespace.h>

#include <kshortcut.h>

#include <map>
#include <vector>

#include <iostream>

namespace Rosegarden {

class EditView;

//!!! constness

class AbstractCommandBuilder
{
public:
    virtual KCommand *build(QString actionName, EventSelection &s) = 0;
};

template <typename Command>
class BasicSelectionCommandBuilder : public AbstractCommandBuilder
{
public:
    virtual KCommand *build(QString /* actionName */, EventSelection &s) {
        return new Command(s);
    }
};

template <typename Command>
class ArgumentAndSelectionCommandBuilder : public AbstractCommandBuilder
{
public:
    virtual KCommand *build(QString actionName, EventSelection &s) {
        return new Command(Command::getArgument(actionName), s);
    }
};

class CommandRegistry : public QObject
{
    Q_OBJECT

public:
    CommandRegistry(EditView *v);

    virtual ~CommandRegistry();

//    template <typename Builder>
    void registerCommand(QString title,
                         QString icon,
                         const KShortcut &shortcut,
                         QString actionName,
                         AbstractCommandBuilder *builder) {
        addAction(title,
                  icon,
                  shortcut,
                  actionName);

//        m_builders[identifier] = new Builder();
        m_builders[actionName] = builder;
    }

//    static void addRegistrar(AbstractCommandRegistrar *);

public slots:
    void slotInvokeCommand();

protected:
//    typedef std::vector<AbstractCommandRegistrar *> RegistrarList;
//    typedef std::map<QString, RegistrarList> ViewRegistrarMap;
//    static ViewRegistrarMap m_registrars;

    typedef std::map<QString, AbstractCommandBuilder *> ActionBuilderMap;
    ActionBuilderMap m_builders;

    void addAction(QString title,
                   QString icon,
                   const KShortcut &shortcut, 
                   QString actionName);

    EditView *m_view;

private:
    CommandRegistry(const CommandRegistry &);
    CommandRegistry &operator=(const CommandRegistry &);
    
};
/*
template <typename C>
class NotationCommandRegistrar : public AbstractCommandRegistrar
{
public:
    virtual QString getViewName() { return "notationview"; }
    virtual void registerCommand(CommandRegistry *r) {
        C::registerCommand(r);
    }
};

class CommandActivator { };

template <typename C>
class NotationCommandActivator : public CommandActivator
{
public:
    NotationCommandActivator() {
        std::cerr << "NotationCommandActivator" << std::endl;
        CommandRegistry::addRegistrar(new NotationCommandRegistrar<C>);
        m_identifier = 0xdeafbeef;
    }

    int m_identifier;
};

template <typename C>
class MatrixCommandRegistrar : public AbstractCommandRegistrar
{
public:
    virtual QString getViewName() { return "matrixview"; }

    virtual void registerCommand(CommandRegistry *r) {
        C::registerCommand(r);
    }
};

template <typename C>
class EventListCommandRegistrar : public AbstractCommandRegistrar
{
public:
    virtual QString getViewName() { return "eventview"; }

    virtual void registerCommand(CommandRegistry *r) {
        C::registerCommand(r);
    }
};
*/
//!!! main view is not an EditView, but it's probably less relevant here

}

#endif


