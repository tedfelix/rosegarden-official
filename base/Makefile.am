KDE_CXXFLAGS = $(USE_EXCEPTIONS)

# create a static library with libtool
noinst_LTLIBRARIES = libbase.la

libbase_la_LDFLAGS = $(all_libraries)

INCLUDES= $(all_includes)

libbase_la_SOURCES = AnalysisTypes.C \
	             BaseProperties.C \
		     Composition.C \
		     CompositionTimeSliceAdapter.C \
		     Equation.C \
		     Event.C \
                     Instrument.C \
		     NotationTypes.C \
                     Property.C \
		     PropertyName.C \
		     Quantizer.C \
		     RealTime.C \
		     Segment.C \
		     SegmentMatrixHelper.C \
		     SegmentNotationHelper.C \
		     SegmentPerformanceHelper.C \
                     Track.C \
		     ViewElement.C

