// Boost.Test
#include <boost/test/unit_test.hpp>

#include "chordname.h"

using boost::unit_test::test_suite;

void test_constructors (void)
{
    Guitar::ChordName a;

    BOOST_CHECK_EQUAL (a.getName(), QString("C"));
    BOOST_CHECK_EQUAL (a.getModifier(), QString("Major"));
    BOOST_CHECK_EQUAL (a.getSuffix(), QString("None"));
    BOOST_CHECK_EQUAL (a.getVersion(), static_cast<unsigned int>(0));

}

test_suite*
init_unit_test_suite( int, char* [] )
{
    test_suite* test = BOOST_TEST_SUITE( "class Note unit test suite" );
    test->add
    ( BOOST_TEST_CASE (&test_constructors) );

    return test;
}
