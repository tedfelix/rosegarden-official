# Building the rosegarden executable

# Define where to build the targets : at the top of the build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compiler definitions
configure_file(rosegarden-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/rosegarden-version.h )

# Export macros
configure_file(rosegardenprivate_export.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/rosegardenprivate_export.h)
add_definitions(-DROSEGARDENPRIVATE_LIB_MAKEDLL)

# Define the include paths (-I...)
include_directories(
    ${ZLIB_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
    ${LIBLO_INCLUDE_DIRS}
    ${LRDF_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
    ${SAMPLERATE_INCLUDE_DIRS}
)

set(rg_OPTIONAL_LIBS )

# For each optional dependency: if found, set a -D definition, a -I include path, and add lib to the list
if(SNDFILE_FOUND)
    include_directories(${SNDFILE_INCLUDE_DIRS})
    add_definitions(-DHAVE_LIBSNDFILE)
    set(rg_OPTIONAL_LIBS ${rg_OPTIONAL_LIBS} ${SNDFILE_LIBRARIES})
endif()

if(JACK_FOUND)
    include_directories(${JACK_INCLUDE_DIRS})
    add_definitions(-DHAVE_LIBJACK)
    set(rg_OPTIONAL_LIBS ${rg_OPTIONAL_LIBS} ${JACK_LIBRARIES})
endif()

if(LIRCCLIENT_FOUND)
    include_directories(${LIRCCLIENT_INCLUDE_DIR})
    add_definitions(-DHAVE_LIRC)
    set(CMAKE_AUTOMOC_MOC_OPTIONS ${CMAKE_AUTOMOC_MOC_OPTIONS} -DHAVE_LIRC) # for cmake 2.8.12, fixed in 2.8.12.1 (https://cmake.org/gitweb?p=cmake.git;a=commitdiff;h=a1b9465b)
    set(rg_OPTIONAL_LIBS ${rg_OPTIONAL_LIBS} ${LIRCCLIENT_LIBRARY})
endif()

if(ALSA_FOUND)
    include_directories(${ALSA_INCLUDE_DIR})
    add_definitions(-DHAVE_ALSA)
    set(rg_OPTIONAL_LIBS ${rg_OPTIONAL_LIBS} ${ALSA_LIBRARIES})
endif()

set(rg_CPPS
  document/GzipFile.cpp
  document/LinkedSegmentsCommand.cpp
  document/Command.cpp
  document/BasicCommand.cpp
  document/RoseXmlHandler.cpp
  document/CommandRegistry.cpp
  document/DocumentGet.cpp
  document/io/PercussionMap.cpp
  document/io/MusicXmlExporter.cpp
  document/io/LilyPondLanguage.cpp
  document/io/MusicXMLLoader.cpp
  document/io/MupExporter.cpp
  document/io/LilyPondSegmentsContext.cpp
  document/io/HydrogenLoader.cpp
  document/io/MusicXMLImportHelper.cpp
  document/io/LilyPondExporter.cpp
  document/io/MusicXMLXMLHandler.cpp
  document/io/HydrogenXMLHandler.cpp
  document/io/MusicXmlExportHelper.cpp
  document/io/CsoundExporter.cpp
  document/io/RG21Loader.cpp
  document/RosegardenDocument.cpp
  document/XmlStorableEvent.cpp
  document/CommandHistory.cpp
  document/BasicSelectionCommand.cpp
  document/XmlSubHandler.cpp
  document/MetadataHelper.cpp
  misc/ConfigGroups.cpp
  misc/TempDir.cpp
  misc/AppendLabel.cpp
  misc/Debug.cpp
  misc/Version.cpp
  misc/Strings.cpp
  gui/dialogs/PasteNotationDialog.cpp
  gui/dialogs/ConfigureDialogBase.cpp
  gui/dialogs/PitchDialog.cpp
  gui/dialogs/InterpretDialog.cpp
  gui/dialogs/ImportDeviceDialog.cpp
  gui/dialogs/UnusedAudioSelectionDialog.cpp
  gui/dialogs/TrackLabelDialog.cpp
  gui/dialogs/TupletDialog.cpp
  gui/dialogs/ManageMetronomeDialog.cpp
  gui/dialogs/MarkerModifyDialog.cpp
  gui/dialogs/MakeOrnamentDialog.cpp
  gui/dialogs/ClefDialog.cpp
  gui/dialogs/AudioPluginDialog.cpp
  gui/dialogs/ExportDeviceDialog.cpp
  gui/dialogs/DialogSuppressor.cpp
  gui/dialogs/CountdownDialog.cpp
  gui/dialogs/LyricEditDialog.cpp
  gui/dialogs/CountdownBar.cpp
  gui/dialogs/CompositionLengthDialog.cpp
  gui/dialogs/BeatsBarsDialog.cpp
  gui/dialogs/PitchBendSequenceDialog.cpp
  gui/dialogs/TimeSignatureDialog.cpp
  gui/dialogs/SimpleEventEditDialog.cpp
  gui/dialogs/TempoDialog.cpp
  gui/dialogs/AudioPlayingDialog.cpp
  gui/dialogs/TextEventDialog.cpp
  gui/dialogs/IdentifyTextCodecDialog.cpp
  gui/dialogs/EventFilterDialog.cpp
  gui/dialogs/MusicXMLOptionsDialog.cpp
  gui/dialogs/TriggerSegmentDialog.cpp
  gui/dialogs/FileMergeDialog.cpp
  gui/dialogs/PitchPickerDialog.cpp
  gui/dialogs/ConfigureDialog.cpp
  gui/dialogs/AudioSplitDialog.cpp
  gui/dialogs/KeySignatureDialog.cpp
  gui/dialogs/FileLocateDialog.cpp
  gui/dialogs/SplitByRecordingSrcDialog.cpp
  gui/dialogs/DocumentConfigureDialog.cpp
  gui/dialogs/IntervalDialog.cpp
  gui/dialogs/InsertTupletDialog.cpp
  gui/dialogs/TransportDialog.cpp
  gui/dialogs/ShowSequencerStatusDialog.cpp
  gui/dialogs/EventEditDialog.cpp
  gui/dialogs/QuantizeDialog.cpp
  gui/dialogs/CheckForParallelsDialog.cpp
  gui/dialogs/FloatEdit.cpp
  gui/dialogs/TimeDialog.cpp
  gui/dialogs/SelectDialog.cpp
  gui/dialogs/RescaleDialog.cpp
  gui/dialogs/EventParameterDialog.cpp
  gui/dialogs/AboutDialog.cpp
  gui/dialogs/LilyPondOptionsDialog.cpp
  gui/dialogs/SplitByPitchDialog.cpp
  gui/dialogs/AddTracksDialog.cpp
  gui/dialogs/GeneratedRegionDialog.cpp
  gui/dialogs/AudioManagerDialog.cpp
  gui/dialogs/UseOrnamentDialog.cpp
  gui/dialogs/CommentsPopupDialog.cpp
  gui/widgets/IconStackedWidget.cpp
  gui/widgets/TristateCheckBox.cpp
  gui/widgets/FileDialog.cpp
  gui/widgets/FontRequester.cpp
  gui/widgets/TimeWidget.cpp
  gui/widgets/WarningGroupBox.cpp
  gui/widgets/MidiFaderWidget.cpp
  gui/widgets/ProgressBar.cpp
  gui/widgets/Panned.cpp
  gui/widgets/AudioVUMeter.cpp
  gui/widgets/ZoomSlider.cpp
  gui/widgets/SqueezedLabel.cpp
  gui/widgets/Rotary.cpp
  gui/widgets/ScrollBoxDialog.cpp
  gui/widgets/AudioListView.cpp
  gui/widgets/LedButton.cpp
  gui/widgets/InputDialog.cpp
  gui/widgets/LilyVersionAwareCheckBox.cpp
  gui/widgets/Fader.cpp
  gui/widgets/Panner.cpp
  gui/widgets/LineEdit.cpp
  gui/widgets/CheckButton.cpp
  gui/widgets/PitchDragLabel.cpp
  gui/widgets/PluginControl.cpp
  gui/widgets/AudioFaderBox.cpp
  gui/widgets/QuantizeParameters.cpp
  gui/widgets/ColourTableItem.cpp
  gui/widgets/VUMeter.cpp
  gui/widgets/TmpStatusMsg.cpp
  gui/widgets/BaseTextFloat.cpp
  gui/widgets/Label.cpp
  gui/widgets/InstrumentAliasButton.cpp
  gui/widgets/IconButton.cpp
  gui/widgets/StartupLogo.cpp
  gui/widgets/DeferScrollArea.cpp
  gui/widgets/Thumbwheel.cpp
  gui/widgets/CollapsingFrame.cpp
  gui/widgets/PluginPushButton.cpp
  gui/widgets/WheelyButton.cpp
  gui/widgets/DiatonicPitchChooser.cpp
  gui/widgets/Led.cpp
  gui/widgets/WarningDialog.cpp
  gui/widgets/AudioRouteMenu.cpp
  gui/widgets/ScrollBox.cpp
  gui/widgets/ColourTable.cpp
  gui/widgets/PitchChooser.cpp
  gui/widgets/WarningWidget.cpp
  gui/widgets/TextFloat.cpp
  gui/editors/pitchtracker/PitchHistory.cpp
  gui/editors/pitchtracker/PitchTrackerView.cpp
  gui/editors/pitchtracker/PitchGraphWidget.cpp
  gui/editors/guitar/GuitarChordSelectorDialog.cpp
  gui/editors/guitar/FingeringListBoxItem.cpp
  gui/editors/guitar/ChordXmlHandler.cpp
  gui/editors/guitar/ChordMap.cpp
  gui/editors/guitar/Chord.cpp
  gui/editors/guitar/Fingering.cpp
  gui/editors/guitar/NoteSymbols.cpp
  gui/editors/guitar/GuitarChordEditorDialog.cpp
  gui/editors/guitar/FingeringBox.cpp
  gui/editors/parameters/RosegardenParameterBox.cpp
  gui/editors/parameters/AudioInstrumentParameterPanel.cpp
  gui/editors/parameters/TrackParameterBox.cpp
  gui/editors/parameters/InstrumentParameterBox.cpp
  gui/editors/parameters/MIDIInstrumentParameterPanel.cpp
  gui/editors/parameters/RosegardenParameterArea.cpp
  gui/editors/parameters/InstrumentParameterPanel.cpp
  gui/editors/parameters/SegmentParameterBox.cpp
  gui/editors/notation/NotationCommandRegistry.cpp
  gui/editors/notation/NotationEraser.cpp
  gui/editors/notation/NoteFontFactory.cpp
  gui/editors/notation/NoteItem.cpp
  gui/editors/notation/NotationStrings.cpp
  gui/editors/notation/SystemFontQt.cpp
  gui/editors/notation/NotationChord.cpp
  gui/editors/notation/NotationToolBox.cpp
  gui/editors/notation/BarLineItem.cpp
  gui/editors/notation/NoteFontMap.cpp
  gui/editors/notation/NoteCharacter.cpp
  gui/editors/notation/StaffHeader.cpp
  gui/editors/notation/TextInserter.cpp
  gui/editors/notation/NotationElement.cpp
  gui/editors/notation/NoteCharacterNames.cpp
  gui/editors/notation/NotationSelector.cpp
  gui/editors/notation/NoteStyle.cpp
  gui/editors/notation/NotationStaff.cpp
  gui/editors/notation/SymbolInserter.cpp
  gui/editors/notation/ClefKeyContext.cpp
  gui/editors/notation/ClefInserter.cpp
  gui/editors/notation/NotationWidget.cpp
  gui/editors/notation/StaffLayout.cpp
  gui/editors/notation/NotationHLayout.cpp
  gui/editors/notation/GuitarChordInserter.cpp
  gui/editors/notation/HeadersGroup.cpp
  gui/editors/notation/FontViewFrame.cpp
  gui/editors/notation/NotationGroup.cpp
  gui/editors/notation/NotationView.cpp
  gui/editors/notation/NotationVLayout.cpp
  gui/editors/notation/NoteFont.cpp
  gui/editors/notation/NotationScene.cpp
  gui/editors/notation/NoteRestInserter.cpp
  gui/editors/notation/NoteStyleFactory.cpp
  gui/editors/notation/NoteStyleFileReader.cpp
  gui/editors/notation/NotationTool.cpp
  gui/editors/notation/NotationProperties.cpp
  gui/editors/notation/NoteFontViewer.cpp
  gui/editors/notation/NotePixmapFactory.cpp
  gui/editors/notation/SystemFont.cpp
  gui/editors/notation/NotePixmapParameters.cpp
  gui/editors/tempo/TempoListItem.cpp
  gui/editors/tempo/TempoView.cpp
  gui/editors/matrix/MatrixView.cpp
  gui/editors/matrix/MatrixElement.cpp
  gui/editors/matrix/PianoKeyboard.cpp
  gui/editors/matrix/MatrixMover.cpp
  gui/editors/matrix/MatrixWidget.cpp
  gui/editors/matrix/MatrixCommandRegistry.cpp
  gui/editors/matrix/MatrixScene.cpp
  gui/editors/matrix/MatrixEraser.cpp
  gui/editors/matrix/MatrixTool.cpp
  gui/editors/matrix/MatrixPainter.cpp
  gui/editors/matrix/MatrixResizer.cpp
  gui/editors/matrix/MatrixVelocity.cpp
  gui/editors/matrix/MatrixViewSegment.cpp
  gui/editors/matrix/MatrixSelector.cpp
  gui/editors/matrix/MatrixToolBox.cpp
  gui/editors/eventlist/TrivialVelocityDialog.cpp
  gui/editors/eventlist/EventView.cpp
  gui/editors/eventlist/EventViewItem.cpp
  gui/editors/segment/TriggerManagerItem.cpp
  gui/editors/segment/PlayListView.cpp
  gui/editors/segment/TrackButtons.cpp
  gui/editors/segment/TrackVUMeter.cpp
  gui/editors/segment/TriggerSegmentManager.cpp
  gui/editors/segment/MarkerEditorViewItem.cpp
  gui/editors/segment/TrackLabel.cpp
  gui/editors/segment/PlayListDialog.cpp
  gui/editors/segment/compositionview/SegmentRect.cpp
  gui/editors/segment/compositionview/SegmentToolBox.cpp
  gui/editors/segment/compositionview/SegmentOrderer.cpp
  gui/editors/segment/compositionview/SegmentEraser.cpp
  gui/editors/segment/compositionview/SegmentPencil.cpp
  gui/editors/segment/compositionview/SegmentItemPreview.cpp
  gui/editors/segment/compositionview/CompositionModelImpl.cpp
  gui/editors/segment/compositionview/CompositionView.cpp
  gui/editors/segment/compositionview/AudioPeaksReadyEvent.cpp
  gui/editors/segment/compositionview/SegmentSelector.cpp
  gui/editors/segment/compositionview/ChangingSegment.cpp
  gui/editors/segment/compositionview/SegmentTool.cpp
  gui/editors/segment/compositionview/AudioPreviewPainter.cpp
  gui/editors/segment/compositionview/CompositionColourCache.cpp
  gui/editors/segment/compositionview/SegmentResizer.cpp
  gui/editors/segment/compositionview/AudioPeaksThread.cpp
  gui/editors/segment/compositionview/SegmentSplitter.cpp
  gui/editors/segment/compositionview/AudioPeaksGenerator.cpp
  gui/editors/segment/compositionview/SegmentMover.cpp
  gui/editors/segment/compositionview/SegmentJoiner.cpp
  gui/editors/segment/PlayList.cpp
  gui/editors/segment/PlayListViewItem.cpp
  gui/editors/segment/MarkerEditor.cpp
  gui/editors/segment/TrackEditor.cpp
  gui/configuration/ColourConfigurationPage.cpp
  gui/configuration/MatrixConfigurationPage.cpp
  gui/configuration/MIDIConfigurationPage.cpp
  gui/configuration/DocumentMetaConfigurationPage.cpp
  gui/configuration/TabbedConfigurationPage.cpp
  gui/configuration/NotationConfigurationPage.cpp
  gui/configuration/AudioConfigurationPage.cpp
  gui/configuration/AudioPropertiesPage.cpp
  gui/configuration/PitchTrackerConfigurationPage.cpp
  gui/configuration/HeadersConfigurationPage.cpp
  gui/configuration/CommentsConfigurationPage.cpp
  gui/configuration/ConfigurationPage.cpp
  gui/configuration/GeneralConfigurationPage.cpp
  gui/application/RosegardenApplication.cpp
  gui/application/TranzportClient.cpp
  gui/application/RosegardenMainViewWidget.cpp
  gui/application/StartupTester.cpp
  gui/application/SetWaitCursor.cpp
  gui/application/LircCommander.cpp
  gui/application/RosegardenMainWindow.cpp
  gui/application/LircClient.cpp
  gui/general/Spline.cpp
  gui/general/IconLoader.cpp
  gui/general/PixmapFunctions.cpp
  gui/general/EditViewBase.cpp
  gui/general/RecentFiles.cpp
  gui/general/MidiPitchLabel.cpp
  gui/general/AutoSaveFinder.cpp
  gui/general/RosegardenScrollView.cpp
  gui/general/PresetElement.cpp
  gui/general/ActionFileClient.cpp
  gui/general/BaseTool.cpp
  gui/general/ListEditView.cpp
  gui/general/TempDirectory.cpp
  gui/general/DecoyAction.cpp
  gui/general/PresetGroup.cpp
  gui/general/ActionCommandRegistry.cpp
  gui/general/GUIPalette.cpp
  gui/general/HZoomable.cpp
  gui/general/ClefIndex.cpp
  gui/general/ActionFileParser.cpp
  gui/general/LilyPondProcessor.cpp
  gui/general/ResourceFinder.cpp
  gui/general/PresetHandlerDialog.cpp
  gui/general/BaseToolBox.cpp
  gui/general/CategoryElement.cpp
  gui/general/ThornStyle.cpp
  gui/general/ProjectPackager.cpp
  gui/general/FileSource.cpp
  gui/general/EditTempoController.cpp
  gui/rulers/ControlItem.cpp
  gui/rulers/DefaultVelocityColour.cpp
  gui/rulers/ControllerEventsRuler.cpp
  gui/rulers/ControlRulerEventEraseCommand.cpp
  gui/rulers/TextRuler.cpp
  gui/rulers/PropertyControlRuler.cpp
  gui/rulers/ControlRuler.cpp
  gui/rulers/EventControlItem.cpp
  gui/rulers/PropertyBox.cpp
  gui/rulers/ControlChangeCommand.cpp
  gui/rulers/ControlToolBox.cpp
  gui/rulers/PercussionPitchRuler.cpp
  gui/rulers/ControllerEventAdapter.cpp
  gui/rulers/ControlRulerTabBar.cpp
  gui/rulers/VelocityColour.cpp
  gui/rulers/ControlTool.cpp
  gui/rulers/ControlMover.cpp
  gui/rulers/ControlSelector.cpp
  gui/rulers/TempoColour.cpp
  gui/rulers/MarkerRuler.cpp
  gui/rulers/ChordNameRuler.cpp
  gui/rulers/ControlEraser.cpp
  gui/rulers/TempoRuler.cpp
  gui/rulers/ControlRulerWidget.cpp
  gui/rulers/PitchRuler.cpp
  gui/rulers/ControlRulerEventInsertCommand.cpp
  gui/rulers/StandardRuler.cpp
  gui/rulers/LoopRuler.cpp
  gui/rulers/PropertyAdjuster.cpp
  gui/rulers/ControlPainter.cpp
  gui/rulers/ViewElementAdapter.cpp
  gui/rulers/RawNoteRuler.cpp
  gui/rulers/PropertyControlItem.cpp
  gui/studio/MidiMixerWindow.cpp
  gui/studio/OSCMessage.cpp
  gui/studio/AudioPluginClipboard.cpp
  gui/studio/MixerWindow.cpp
  gui/studio/MidiProgramsEditor.cpp
  gui/studio/StudioControl.cpp
  gui/studio/ControlEditorDialog.cpp
  gui/studio/MidiKeyMapTreeWidgetItem.cpp
  gui/studio/AudioPluginOSCGUIManager.cpp
  gui/studio/AudioPlugin.cpp
  gui/studio/DeviceManagerDialog.cpp
  gui/studio/BankEditorDialog.cpp
  gui/studio/MidiBankTreeWidgetItem.cpp
  gui/studio/NameSetEditor.cpp
  gui/studio/ControlParameterItem.cpp
  gui/studio/AudioPluginOSCGUI.cpp
  gui/studio/AudioPluginManager.cpp
  gui/studio/MidiKeyMappingEditor.cpp
  gui/studio/MidiDeviceTreeWidgetItem.cpp
  gui/studio/SynthPluginManagerDialog.cpp
  gui/studio/ControlParameterEditDialog.cpp
  gui/studio/TimerCallbackAssistant.cpp
  gui/studio/AudioMixerWindow.cpp
  gui/studio/RemapInstrumentDialog.cpp
  gui/studio/MidiMixerVUMeter.cpp
  gui/seqmanager/MetronomeMapper.cpp
  gui/seqmanager/TempoSegmentMapper.cpp
  gui/seqmanager/MappedEventBuffer.cpp
  gui/seqmanager/SequenceManager.cpp
  gui/seqmanager/MarkerMapper.cpp
  gui/seqmanager/SegmentMapper.cpp
  gui/seqmanager/MidiFilterDialog.cpp
  gui/seqmanager/TimeSigSegmentMapper.cpp
  gui/seqmanager/ChannelManager.cpp
  gui/seqmanager/CompositionMapper.cpp
  gui/seqmanager/AudioSegmentMapper.cpp
  gui/seqmanager/InternalSegmentMapper.cpp
  gui/seqmanager/SpecialSegmentMapper.cpp
  base/SegmentLinker.cpp
  base/NotationQuantizer.cpp
  base/AnalysisTypes.cpp
  base/Instrument.cpp
  base/Segment.cpp
  base/ControllerContext.cpp
  base/ViewSegment.cpp
  base/parameterpattern/SelectionSituation.cpp
  base/parameterpattern/QuarterSinePattern.cpp
  base/parameterpattern/LinearParameterPattern.cpp
  base/parameterpattern/AlternatingParameterPattern.cpp
  base/parameterpattern/ParameterPattern.cpp
  base/parameterpattern/IncreaseParameterPattern.cpp
  base/parameterpattern/RelativeRamp.cpp
  base/parameterpattern/FlatParameterPattern.cpp
  base/parameterpattern/RingingParameterPattern.cpp
  base/parameterpattern/HalfSinePattern.cpp
  base/Colour.cpp
  base/InstrumentStaticSignals.cpp
  base/Selection.cpp
  base/BaseProperties.cpp
  base/UrlHash.cpp
  base/AudioDevice.cpp
  base/Marker.cpp
  base/Equation.cpp
  base/SegmentMatrixHelper.cpp
  base/ControlParameter.cpp
  base/MidiDevice.cpp
  base/Configuration.cpp
  base/ColourMap.cpp
  base/Sets.cpp
  base/Studio.cpp
  base/figuration/ChordSegment.cpp
  base/figuration/SegmentID.cpp
  base/figuration/SegmentFigData.cpp
  base/figuration/GeneratedRegion.cpp
  base/figuration/FigurationSourceMap.cpp
  base/figuration/FigChord.cpp
  base/ScriptAPI.cpp
  base/SnapGrid.cpp
  base/Exception.cpp
  base/PropertyMap.cpp
  base/Composition.cpp
  base/Track.cpp
  base/Clipboard.cpp
  base/Event.cpp
  base/SoftSynthDevice.cpp
  base/RealTime.cpp
  base/SegmentNotationHelper.cpp
  base/AllocateChannels.cpp
  base/AudioLevel.cpp
  base/Profiler.cpp
  base/RulerScale.cpp
  base/TriggerSegment.cpp
  base/ViewElement.cpp
  base/BasicQuantizer.cpp
  base/MidiTypes.cpp
  base/ChannelInterval.cpp
  base/LegatoQuantizer.cpp
  base/AudioPluginInstance.cpp
  base/Property.cpp
  base/Quantizer.cpp
  base/XmlExportable.cpp
  base/NotationTypes.cpp
  base/PropertyName.cpp
  base/SegmentPerformanceHelper.cpp
  base/Device.cpp
  base/MidiProgram.cpp
  base/CompositionTimeSliceAdapter.cpp
  sound/LADSPAPluginFactory.cpp
  sound/ControlBlock.cpp
  sound/WAVAudioFile.cpp
  sound/MappedEventList.cpp
  sound/AudioTimeStretcher.cpp
  sound/RecordableAudioFile.cpp
  sound/LSCPPatchExtractor.cpp
  sound/RunnablePluginInstance.cpp
  sound/PeakFileManager.cpp
  sound/AudioFile.cpp
  sound/Audit.cpp
  sound/MappedStudio.cpp
  sound/PluginIdentifier.cpp
  sound/MappedEvent.cpp
  sound/JackDriver.cpp
  sound/AlsaDriver.cpp
  sound/AlsaPort.cpp
  sound/SortingInserter.cpp
  sound/MappedBufMetaIterator.cpp
  sound/MappedDevice.cpp
  sound/SF2PatchExtractor.cpp
  sound/AudioProcess.cpp
  sound/LADSPAPluginInstance.cpp
  sound/DSSIPluginInstance.cpp
  sound/MidiEvent.cpp
  sound/SoundFile.cpp
  sound/ImmediateNote.cpp
  sound/SoundDriverFactory.cpp
  sound/audiostream/AudioReadStream.cpp
  sound/audiostream/WavFileWriteStream.cpp
  sound/audiostream/WavFileReadStream.cpp
  sound/audiostream/SimpleWavFileWriteStream.cpp
  sound/audiostream/AudioWriteStreamFactory.cpp
  sound/audiostream/AudioReadStreamFactory.cpp
  sound/audiostream/OggVorbisReadStream.cpp
  sound/MidiInserter.cpp
  sound/MappedEventInserter.cpp
  sound/PluginFactory.cpp
  sound/DummyDriver.cpp
  sound/BWFAudioFile.cpp
  sound/PeakFile.cpp
  sound/RIFFAudioFile.cpp
  sound/AudioFileTimeStretcher.cpp
  sound/SequencerDataBlock.cpp
  sound/MidiFile.cpp
  sound/DSSIPluginFactory.cpp
  sound/MappedInstrument.cpp
  sound/PlayableAudioFile.cpp
  sound/SoundDriver.cpp
  sound/AudioCache.cpp
  sound/JackCaptureClient.cpp
  sound/Tuning.cpp
  sound/AudioFileManager.cpp
  sound/AudioPlayQueue.cpp
  sound/PitchDetector.cpp
  sound/Resampler.cpp
  commands/notation/ResetDisplacementsCommand.cpp
  commands/notation/RemoveNotationQuantizeCommand.cpp
  commands/notation/AddMarkCommand.cpp
  commands/notation/AddTextMarkCommand.cpp
  commands/notation/CycleSlashesCommand.cpp
  commands/notation/ClefInsertionCommand.cpp
  commands/notation/ChangeStyleCommand.cpp
  commands/notation/GraceCommand.cpp
  commands/notation/RespellCommand.cpp
  commands/notation/TieNotesCommand.cpp
  commands/notation/UnGraceCommand.cpp
  commands/notation/KeyInsertionCommand.cpp
  commands/notation/GuitarChordInsertionCommand.cpp
  commands/notation/GeneratedRegionInsertionCommand.cpp
  commands/notation/MakeRegionViableCommand.cpp
  commands/notation/TextInsertionCommand.cpp
  commands/notation/EraseEventCommand.cpp
  commands/notation/AutoBeamCommand.cpp
  commands/notation/RestInsertionCommand.cpp
  commands/notation/BeamCommand.cpp
  commands/notation/MakeAccidentalsCautionaryCommand.cpp
  commands/notation/AdoptSegmentCommand.cpp
  commands/notation/SymbolInsertionCommand.cpp
  commands/notation/NormalizeRestsCommand.cpp
  commands/notation/ChangeSlurPositionCommand.cpp
  commands/notation/ChangeStemsCommand.cpp
  commands/notation/TextChangeCommand.cpp
  commands/notation/TupletCommand.cpp
  commands/notation/SetVisibilityCommand.cpp
  commands/notation/InterpretCommand.cpp
  commands/notation/CollapseRestsCommand.cpp
  commands/notation/SustainInsertionCommand.cpp
  commands/notation/RemoveMarksCommand.cpp
  commands/notation/RestoreTiesCommand.cpp
  commands/notation/MakeChordCommand.cpp
  commands/notation/MarkParallelCommand.cpp
  commands/notation/BreakCommand.cpp
  commands/notation/MakeNotesViableCommand.cpp
  commands/notation/UntieNotesCommand.cpp
  commands/notation/RestoreStemsCommand.cpp
  commands/notation/AddSlashesCommand.cpp
  commands/notation/AddFingeringMarkCommand.cpp
  commands/notation/FixNotationQuantizeCommand.cpp
  commands/notation/RestoreSlursCommand.cpp
  commands/notation/MultiKeyInsertionCommand.cpp
  commands/notation/NoteInsertionCommand.cpp
  commands/notation/AddIndicationCommand.cpp
  commands/notation/ChangeTiePositionCommand.cpp
  commands/notation/RemoveFingeringMarksCommand.cpp
  commands/notation/UnTupletCommand.cpp
  commands/notation/DeCounterpointCommand.cpp
  commands/notation/IncrementDisplacementsCommand.cpp
  commands/edit/MoveCommand.cpp
  commands/edit/SetTriggerCommand.cpp
  commands/edit/PlaceControllersCommand.cpp
  commands/edit/CopyCommand.cpp
  commands/edit/CollapseNotesCommand.cpp
  commands/edit/InvertCommand.cpp
  commands/edit/EventQuantizeCommand.cpp
  commands/edit/PasteEventsCommand.cpp
  commands/edit/RetrogradeInvertCommand.cpp
  commands/edit/EventUnquantizeCommand.cpp
  commands/edit/MaskTriggerCommand.cpp
  commands/edit/EraseCommand.cpp
  commands/edit/EventInsertionCommand.cpp
  commands/edit/InsertTriggerNoteCommand.cpp
  commands/edit/CutCommand.cpp
  commands/edit/CutAndCloseCommand.cpp
  commands/edit/SelectionPropertyCommand.cpp
  commands/edit/AddMarkerCommand.cpp
  commands/edit/AddDotCommand.cpp
  commands/edit/SelectAddEvenNotesCommand.cpp
  commands/edit/RescaleCommand.cpp
  commands/edit/TransposeCommand.cpp
  commands/edit/MoveAcrossSegmentsCommand.cpp
  commands/edit/RemoveMarkerCommand.cpp
  commands/edit/ClearTriggersCommand.cpp
  commands/edit/ChangeVelocityCommand.cpp
  commands/edit/EventEditCommand.cpp
  commands/edit/SetLyricsCommand.cpp
  commands/edit/PasteSegmentsCommand.cpp
  commands/edit/SetNoteTypeCommand.cpp
  commands/edit/ModifyMarkerCommand.cpp
  commands/edit/RetrogradeCommand.cpp
  commands/matrix/MatrixInsertionCommand.cpp
  commands/matrix/MatrixModifyCommand.cpp
  commands/matrix/MatrixEraseCommand.cpp
  commands/matrix/MatrixPercussionInsertionCommand.cpp
  commands/segment/SetTriggerSegmentDefaultTimeAdjustCommand.cpp
  commands/segment/SegmentTransposeCommand.cpp
  commands/segment/PasteToTriggerSegmentCommand.cpp
  commands/segment/SetTriggerSegmentBaseVelocityCommand.cpp
  commands/segment/AudioSegmentResizeFromStartCommand.cpp
  commands/segment/SegmentSyncCommand.cpp
  commands/segment/AddTempoChangeCommand.cpp
  commands/segment/ChangeCompositionLengthCommand.cpp
  commands/segment/EraseTempiInRangeCommand.cpp
  commands/segment/RemoveTempoChangeCommand.cpp
  commands/segment/DeleteRangeCommand.cpp
  commands/segment/RemoveTimeSignatureCommand.cpp
  commands/segment/SetTriggerSegmentDefaultRetuneCommand.cpp
  commands/segment/OpenOrCloseRangeCommand.cpp
  commands/segment/DeleteTracksCommand.cpp
  commands/segment/AudioSegmentDistributeCommand.cpp
  commands/segment/ModifyDefaultTempoCommand.cpp
  commands/segment/DeleteTriggerSegmentCommand.cpp
  commands/segment/SegmentCommand.cpp
  commands/segment/ExpandFigurationCommand.cpp
  commands/segment/CreateTempoMapFromSegmentCommand.cpp
  commands/segment/SegmentLabelCommand.cpp
  commands/segment/SegmentInsertCommand.cpp
  commands/segment/AddTracksCommand.cpp
  commands/segment/SegmentColourMapCommand.cpp
  commands/segment/SegmentRecordCommand.cpp
  commands/segment/SegmentQuickCopyCommand.cpp
  commands/segment/FitToBeatsCommand.cpp
  commands/segment/AudioSegmentInsertCommand.cpp
  commands/segment/AudioSegmentRescaleCommand.cpp
  commands/segment/SegmentLinkToCopyCommand.cpp
  commands/segment/SetTriggerSegmentBasePitchCommand.cpp
  commands/segment/SegmentJoinCommand.cpp
  commands/segment/PasteConductorDataCommand.cpp
  commands/segment/MoveTracksCommand.cpp
  commands/segment/PasteRangeCommand.cpp
  commands/segment/AudioSegmentSplitCommand.cpp
  commands/segment/CutToTriggerSegmentCommand.cpp
  commands/segment/SegmentChangeTransposeCommand.cpp
  commands/segment/SegmentSplitByRecordingSrcCommand.cpp
  commands/segment/UpdateFigurationCommand.cpp
  commands/segment/SegmentLinkTransposeCommand.cpp
  commands/segment/SegmentSplitByPitchCommand.cpp
  commands/segment/SegmentSplitByDrumCommand.cpp
  commands/segment/SegmentSyncClefCommand.cpp
  commands/segment/SegmentReconfigureCommand.cpp
  commands/segment/SegmentCommandRepeat.cpp
  commands/segment/AddTriggerSegmentCommand.cpp
  commands/segment/AddTimeSignatureAndNormalizeCommand.cpp
  commands/segment/EraseSegmentsStartingInRangeCommand.cpp
  commands/segment/SegmentRepeatToCopyCommand.cpp
  commands/segment/SegmentQuickLinkCommand.cpp
  commands/segment/SegmentColourCommand.cpp
  commands/segment/InsertRangeCommand.cpp
  commands/segment/AudioSegmentAutoSplitCommand.cpp
  commands/segment/AddTimeSignatureCommand.cpp
  commands/segment/SegmentSingleRepeatToCopyCommand.cpp
  commands/segment/SegmentResizeFromStartCommand.cpp
  commands/segment/AddLayerCommand.cpp
  commands/segment/SegmentChangeQuantizationCommand.cpp
  commands/segment/RenameTrackCommand.cpp
  commands/segment/SegmentAutoSplitCommand.cpp
  commands/segment/SegmentRescaleCommand.cpp
  commands/segment/SegmentSplitCommand.cpp
  commands/segment/CutRangeCommand.cpp
  commands/segment/SegmentEraseCommand.cpp
  commands/studio/CreateOrDeleteDeviceCommand.cpp
  commands/studio/RemoveControlParameterCommand.cpp
  commands/studio/ModifyControlParameterCommand.cpp
  commands/studio/AddControlParameterCommand.cpp
  commands/studio/ReconnectDeviceCommand.cpp
  commands/studio/ModifyDeviceCommand.cpp
  commands/studio/ModifyInstrumentMappingCommand.cpp
  commands/studio/RenameDeviceCommand.cpp
  commands/studio/ModifyDeviceMappingCommand.cpp
  sequencer/SequencerThread.cpp
  sequencer/RosegardenSequencer.cpp
)

set(rg_UIS
  gui/dialogs/RosegardenTransportUi.ui
  gui/studio/DeviceManagerDialogUi.ui
)

set(rg_SOURCES ${rg_CPPS})

if(USE_QT4)
  qt4_wrap_ui(rg_SOURCES ${rg_UIS})
else()
  qt5_wrap_ui(rg_SOURCES ${rg_UIS})
endif()

# Compile translations (.ts -> .qm)
file(GLOB TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../data/locale/*.ts)
list(REMOVE_ITEM TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../data/locale/rosegarden.ts)
if(USE_QT4)
  qt4_add_translation(QM_FILES ${TS_FILES})
else()
  find_package(Qt5LinguistTools "5.1.0" REQUIRED)
  qt5_add_translation(QM_FILES ${TS_FILES})
endif()

# Generate a .qrc for the .qm files
SET(localeqrc_file locale)
SET(localeqrc_srcfile ${CMAKE_CURRENT_SOURCE_DIR}/../data/locale.qrc)
SET(localeqrc_infile ${CMAKE_CURRENT_BINARY_DIR}/locale.qrc)
SET(localeqrc_outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_locale.cpp)

# Copy the QRC file to the output directory, because the files listed in the
# qrc file are relative to that directory.
ADD_CUSTOM_COMMAND(
    OUTPUT ${localeqrc_infile}
    COMMAND ${CMAKE_COMMAND} -E copy ${localeqrc_srcfile} ${localeqrc_infile}
    MAIN_DEPENDENCY ${localeqrc_srcfile}
)

# Run the resource compiler (rcc_options should already be set). We can't
# use QT4_ADD_RESOURCES because the qrc file may not exist yet.
ADD_CUSTOM_COMMAND(
   OUTPUT ${localeqrc_outfile}
   COMMAND ${QT_RCC_EXECUTABLE}
   ARGS ${rcc_options} -name ${localeqrc_file} -o ${localeqrc_outfile} ${localeqrc_infile}
   MAIN_DEPENDENCY ${localeqrc_infile}
   DEPENDS ${QM_FILES}
)

if(USE_QT4)
qt4_add_resources(rg_SOURCES ../data/data.qrc)
else()
qt5_add_resources(rg_SOURCES ../data/data.qrc)
endif()

# Define a static lib with the whole code
add_library(rosegardenprivate ${RG_LIBRARY_TYPE}
   ${rg_SOURCES}
   ${localeqrc_outfile}
   ${CMAKE_CURRENT_BINARY_DIR}/svnversion.h
)

# Link to the following libraries
target_link_libraries(rosegardenprivate
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${ZLIB_LIBRARY}
  ${CMAKE_DL_LIBS}
  ${LIBLO_LIBRARIES}
  ${LRDF_LIBRARIES}
  ${FFTW3F_LIBRARIES}
  ${SAMPLERATE_LIBRARIES}
  ${rg_OPTIONAL_LIBS}
)

#########################
# Generate list of source files, as input for buildkey.cmake to compute BUILDKEY at compile time
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/source_files_list "")
foreach(source_file ${rg_CPPS} ${rg_UIS})
   get_filename_component(abs_path ${source_file} ABSOLUTE)
   file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/source_files_list "${abs_path} ")
endforeach()

# Magic for SVN revision number, found on
# http://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision

# a custom target that is always built
add_custom_target(svnheader ALL DEPENDS svn_header) # svn_header never gets created, it's just a unique string

# creates svnheader.h using cmake script
add_custom_command(OUTPUT svn_header ${CMAKE_CURRENT_BINARY_DIR}/svnheader.h
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/buildkey.cmake)

# svnversion.h is a generated file
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/svnversion.h
    PROPERTIES GENERATED TRUE
    HEADER_FILE_ONLY TRUE)

# explicitly say that the target depends on the svnheader
add_dependencies(rosegardenprivate svnheader)
#########################


# Define an executable called rosegarden, with just the main.cpp, linking to the lib with all the code
add_executable(rosegarden gui/application/main.cpp)

target_link_libraries(rosegarden
  rosegardenprivate
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${X11_LIBRARIES}
)

# Install executable
install(TARGETS rosegarden RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install shared libs, if any
if(RG_LIBRARY_TYPE STREQUAL "SHARED")
  install(TARGETS rosegardenprivate
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
