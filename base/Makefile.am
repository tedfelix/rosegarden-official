KDE_CXXFLAGS = $(USE_EXCEPTIONS)

# create a static library with libtool
noinst_LTLIBRARIES = libbase.la

libbase_la_LDFLAGS = $(all_libraries)

INCLUDES= $(all_includes)

libbase_la_SOURCES = BaseProperties.C \
		     Composition.C \
		     Equation.C \
		     Event.C \
                     Instrument.C \
		     NotationTypes.C \
                     Property.C \
		     PropertyName.C \
		     Quantizer.C \
		     Segment.C \
		     SegmentNotationHelper.C \
		     SegmentPerformanceHelper.C \
		     StaffLayout.C \
                     Track.C \
		     ViewElement.C

