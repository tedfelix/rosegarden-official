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

/**
 * Miscellaneous constants
 */


#ifndef RG_CONFIG_GROUPS_H
#define RG_CONFIG_GROUPS_H

#include <QString>

#include <rosegardenprivate_export.h>

namespace Rosegarden
{

    // Note: Use const char * instead of QString to avoid static
    //       init order fiasco.
    extern ROSEGARDENPRIVATE_EXPORT const char* const GeneralOptionsConfigGroup;
    extern const char* const LatencyOptionsConfigGroup;
    extern ROSEGARDENPRIVATE_EXPORT const char* const SequencerOptionsConfigGroup;
    extern const char* const NotationViewConfigGroup;
    extern const char* const PitchTrackerConfigGroup;
    extern const char* const AudioManagerDialogConfigGroup;
    extern const char* const SynthPluginManagerConfigGroup;
    extern const char* const BankEditorConfigGroup;
    extern const char* const ColoursConfigGroup;
    extern const char* const ControlEditorConfigGroup;
    extern const char* const DeviceManagerConfigGroup;
    extern const char* const EventFilterDialogConfigGroup;
    extern const char* const EventViewConfigGroup;
    extern const char* const MarkerEditorConfigGroup;
    extern const char* const MatrixViewConfigGroup;
    extern const char* const PlayListConfigGroup;
    extern const char* const MainWindowConfigGroup;
    extern const char* const TransportDialogConfigGroup;
    extern const char* const TempoViewConfigGroup;
    extern const char* const TriggerManagerConfigGroup;
    extern const char* const EditViewConfigGroup;
    extern const char* const PresetDialogConfigGroup;
    extern const char* const ExternalApplicationsConfigGroup;
    extern ROSEGARDENPRIVATE_EXPORT const char* const LilyPondExportConfigGroup;
    extern const char* const MusicXMLExportConfigGroup;
    extern const char* const LastUsedPathsConfigGroup;
    extern const char* const WindowGeometryConfigGroup;
    extern const char* const TempDirectoryConfigGroup;
    extern const char* const NotationOptionsConfigGroup;
    extern const char* const DialogSuppressorConfigGroup;
    extern const char* const RecentDirsConfigGroup;
    extern const char* const CollapsingFrameConfigGroup;
    extern const char* const PitchBendSequenceConfigGroup;
    extern const char* const CheckButtonConfigGroup;
    extern const char* const SelectDialogConfigGroup;
    extern const char* const GridQuantizeConfigGroup;
    extern const char* const NotationQuantizeConfigGroup;
    extern const char* const TextEventDialogConfigGroup;
    extern const char* const RecentFilesConfigGroup;
    extern const char* const UserShortcutsConfigGroup;
    extern const char* const ShortcutKeyboardConfigGroup;
    extern const char* const ControlRulerConfigGroup;
    extern const char* const ExperimentalConfigGroup;

}

#endif
