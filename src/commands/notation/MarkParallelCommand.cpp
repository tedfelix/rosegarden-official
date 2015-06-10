/*
 * MarkParallelCommand.cpp
 *
 *  Created on: Mar 25, 2015
 *      Author: lambache
 */


#include "MarkParallelCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"

#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

MarkParallelCommand::MarkParallelCommand(Segment &segment, timeT begin, timeT end) : BasicCommand(tr("Mark Parallel"), segment, begin, end)
{
    // nothing
}

//void
//MarkParallelCommand::registerCommand(CommandRegistry *r)
//{
//    r->registerCommand
//        ("mark_parallel",
//         new SelectionCommandBuilder<MarkParallelCommand>());
//}

void
MarkParallelCommand::modifySegment()
{
}

}

/*
 * MarkParallelCommand.cpp
 *
 *  Created on: Mar 25, 2015
 *      Author: lambache
 */


#include "MarkParallelCommand.h"

#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "gui/editors/notation/NotationProperties.h"
#include "base/BaseProperties.h"
#include "document/BasicCommand.h"

#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

MarkParallelCommand::MarkParallelCommand(Segment &segment, timeT begin, timeT end) : BasicCommand(tr("Mark Parallel"), segment, begin, end)
{
    // nothing
}

//void
//MarkParallelCommand::registerCommand(CommandRegistry *r)
//{
//    r->registerCommand
//        ("mark_parallel",
//         new SelectionCommandBuilder<MarkParallelCommand>());
//}

void
MarkParallelCommand::modifySegment()
{
}

}

