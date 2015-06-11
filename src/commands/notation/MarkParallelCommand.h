/*
 * MarkParallelCommand.h
 *
 *  Created on: Mar 25, 2015
 *      Author: lambache
 */

#ifndef SRC_COMMANDS_NOTATION_MARKPARALLELCOMMAND_H_
#define SRC_COMMANDS_NOTATION_MARKPARALLELCOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include "base/BaseProperties.h"

#include <QString>
#include <QCoreApplication>


namespace Rosegarden
{

using namespace BaseProperties;


class EventSelection;
class CommandRegistry;

class MarkParallelCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MarkParallelCommand)

public:
    MarkParallelCommand(Segment &segment, timeT begin, timeT end);

    static QString getGlobalName() { return tr("Mark Parallel"); }

    //static void registerCommand(CommandRegistry *r);

protected:
    virtual void modifySegment();
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


}

#endif /* SRC_COMMANDS_NOTATION_MARKPARALLELCOMMAND_H_ */
