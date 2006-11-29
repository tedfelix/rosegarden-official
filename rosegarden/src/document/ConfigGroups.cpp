// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ConfigGroups.h"

namespace Rosegarden 
{

    //
    // KConfig group names
    //
    const char* const GeneralOptionsConfigGroup = "General Options";
    const char* const LatencyOptionsConfigGroup = "Latency Options";
    const char* const SequencerOptionsConfigGroup = "Sequencer Options";
    const char* const NotationViewConfigGroup = "Notation Options";
    const char* const AudioManagerDialogConfigGroup = "AudioManagerDialog";
    const char* const SynthPluginManagerConfigGroup = "Synth Plugin Manager";
    const char* const BankEditorConfigGroup = "Bank Editor";
    const char* const ColoursConfigGroup = "coloursconfiggroup";
    const char* const ControlEditorConfigGroup = "Control Editor";
    const char* const DeviceManagerConfigGroup = "Device Manager";
    const char* const EventFilterDialogConfigGroup = "EventFilter Dialog";
    const char* const EventViewLayoutConfigGroupName = "EventList Layout";
    const char* const EventViewConfigGroup = "EventList Options";
    const char* const MarkerEditorConfigGroup = "Marker Editor";
    const char* const MatrixViewConfigGroup = "Matrix Options";
    const char* const PlayListConfigGroup = "PLAY_LIST";
    const char* const MainWindowConfigGroup = "MainView";
    const char* const TransportDialogConfigGroup = "Transport Controls";
    const char* const TempoViewLayoutConfigGroupName = "TempoView Layout";
    const char* const TempoViewConfigGroup = "TempoView Options";
    const char* const TriggerManagerConfigGroup = "Trigger Editor";
    const char* const EditViewConfigGroup = "Edit View";

}
