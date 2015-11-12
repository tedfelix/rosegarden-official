/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

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
    // QSettings group names
    //
    const char* const GeneralOptionsConfigGroup = "General_Options";
    const char* const LatencyOptionsConfigGroup = "Latency_Options";
    const char* const SequencerOptionsConfigGroup = "Sequencer_Options";
    const char* const NotationViewConfigGroup = "Notation_Options";
    const char* const PitchTrackerConfigGroup = "Pitch_Tracker_Options";
    const char* const AudioManagerDialogConfigGroup = "AudioManagerDialog";
    const char* const SynthPluginManagerConfigGroup = "Synth_Plugin_Manager";
    const char* const BankEditorConfigGroup = "Bank_Editor";
    const char* const ColoursConfigGroup = "Colours";
    const char* const ControlEditorConfigGroup = "Control_Editor";
    const char* const DeviceManagerConfigGroup = "Device_Manager";
    const char* const EventFilterDialogConfigGroup = "EventFilter_Dialog";
    const char* const EventViewLayoutConfigGroupName = "EventList_Layout";
    const char* const EventViewConfigGroup = "EventList_Options";
    const char* const MarkerEditorConfigGroup = "Marker_Editor";
    const char* const MatrixViewConfigGroup = "Matrix_Options";
    const char* const PlayListConfigGroup = "Playlist";
    const char* const MainWindowConfigGroup = "Main_Window";
    const char* const TransportDialogConfigGroup = "Transport_Controls";
    const char* const TempoViewLayoutConfigGroupName = "TempoView_Layout";
    const char* const TempoViewConfigGroup = "TempoView_Options";
    const char* const TriggerManagerConfigGroup = "Trigger_Editor";
    const char* const EditViewConfigGroup = "Edit_View";
    const char* const PresetDialogConfigGroup = "Parameter_Presets";
    const char* const ExternalApplicationsConfigGroup = "External_Applications";
    const char* const LilyPondExportConfigGroup = "LilyPond_Export";
    const char* const MusicXMLExportConfigGroup = "MusicXML_Export";
    const char* const LastUsedPathsConfigGroup = "Last_Used_Paths";
    const char* const WindowGeometryConfigGroup = "Window_Geometry";
    const char* const TempDirectoryConfigGroup = "TempDirectory";
    const char* const NotationOptionsConfigGroup = "Notation_Options";
    const char* const DialogSuppressorConfigGroup = "DialogSuppressor";
    const char* const RecentDirsConfigGroup = "RecentDirs";
    const char* const CollapsingFrameConfigGroup = "CollapsingFrame";
    const char* const PitchBendSequenceConfigGroup = "PitchBendSequence";
    const char* const CheckButtonConfigGroup = "CheckButton_Memory";
    const char* const SelectDialogConfigGroup = "SelectDialog_Memory";
    const char* const GridQuantizeConfigGroup = "Grid_Quantize_Parameters_Memory";
    const char* const NotationQuantizeConfigGroup = "Notation_Quantize_Parameters_Memory";
    const char* const TextEventDialogConfigGroup = "TextEvent_Dialog";
}
