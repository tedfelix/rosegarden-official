# the next line sets up include and link directories and defines some variables that we will use.
# you can modify the behavior by setting some variables, e.g.
#   set(QT_USE_OPENGL TRUE)
# -> this will cause cmake to include and link against the OpenGL module
include(${QT_USE_FILE})


set(matrix4_SRCS
    main.cpp
    gvtest.cpp
    GraphicsItemPin.cpp
    FadeItem.cpp
    LinedStaff.cpp
    MatrixElement.cpp
    MatrixViewElementManager.cpp
    MatrixHLayout.cpp
    MatrixVLayout.cpp
    BarLine.cpp
    MatrixStaff.cpp
    MatrixTest.cpp
)

SET(base_SRCS
	base/AudioLevel.cpp
	base/AudioPluginInstance.cpp
	base/BaseProperties.cpp
	base/BasicQuantizer.cpp
	base/Colour.cpp
	base/ColourMap.cpp
	base/ControlParameter.cpp
	base/Composition.cpp
	base/CompositionTimeSliceAdapter.cpp
	base/Configuration.cpp
	base/Device.cpp
	base/Event.cpp
	base/Exception.cpp
	base/Instrument.cpp
	base/Marker.cpp
	base/MidiDevice.cpp
	base/MidiProgram.cpp
	base/MidiTypes.cpp
	base/NotationQuantizer.cpp
	base/NotationTypes.cpp
	base/Profiler.cpp
	base/Property.cpp
	base/PropertyMap.cpp
	base/PropertyName.cpp
	base/Quantizer.cpp
	base/RealTime.cpp
	base/Segment.cpp
	base/SegmentNotationHelper.cpp
	base/Selection.cpp
	base/Sets.cpp
	base/Track.cpp
	base/TriggerSegment.cpp
	base/XmlExportable.cpp
)

SET(viewelement_SRCS
	viewelement/AbstractViewElementManager.cpp
	viewelement/ViewElement.cpp
)

SET(scale_SRCS
	scale/HZoomable.cpp
	scale/LayoutEngine.cpp
	scale/RulerScale.cpp
	scale/SnapGrid.cpp
)

SET(helpers_SRCS
	helpers/SegmentMatrixHelper.cpp
	helpers/SegmentNotationHelper.cpp
)

SET(generalgui_SRCS
	generalgui/DefaultVelocityColour.cpp
    generalgui/GUIPalette.cpp
	generalgui/VelocityColour.cpp
	generalgui/ZoomSlider.cpp
)

ADD_LIBRARY(RosegardenBase STATIC
    ${base_SRCS}
)

ADD_LIBRARY(RosegardenViewElement STATIC
    ${viewelement_SRCS}
)

ADD_LIBRARY(RosegardenScale STATIC
    ${scale_SRCS}
)

ADD_LIBRARY(RosegardenHelpers STATIC
    ${helpers_SRCS}
)

ADD_LIBRARY(RosegardenGeneralGui STATIC
    ${generalgui_SRCS}
)


ADD_DEPENDENCIES(RosegardenBase        precompiled_headers)
ADD_DEPENDENCIES(RosegardenViewElement precompiled_headers)
ADD_DEPENDENCIES(RosegardenScale       precompiled_headers)
ADD_DEPENDENCIES(RosegardenHelpers     precompiled_headers)
ADD_DEPENDENCIES(RosegardenGeneralGui  precompiled_headers)

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc(${matrix4_SRCS})

# create an executable file named "matrix4" from the source files in the variable "matrix4_SRCS".
add_executable(matrix4 ${matrix4_SRCS})

# link the "matrix4" target against the Qt libraries. which libraries exactly, is defined by the "include(${QT_USE_FILE})" line above, which sets up this variable.
target_link_libraries(matrix4
					  ${QT_LIBRARIES}
					  RosegardenScale
					  RosegardenViewElement
					  RosegardenHelpers
					  RosegardenBase
					  RosegardenGeneralGui
					  )
