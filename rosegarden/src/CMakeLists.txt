# At this level, we only have several subdirectories

# base library: libbase.a
ADD_SUBDIRECTORY(base)

# sound libraries: libRosegardenSequencer.a and libRosegardenSound.a
ADD_SUBDIRECTORY(sound)

# sequencer executable: rosegardensequencer
ADD_SUBDIRECTORY(sequencer)

# gui executable: rosegarden
ADD_SUBDIRECTORY(gui)
