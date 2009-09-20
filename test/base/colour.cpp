/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden-4
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file is Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
    If you compile this to a test program
     g++ -o colour -I../ ../Colour.C ../ColourMap.C colour.C

    you can then run it like this:
     ./colour > temp.output

    and do a diff to check that it worked:
     diff -u temp.output colour.output

    If there are any differences, there's a problem 
     (or colour.output hasn't been updated when colour.C has been changed)
*/

#include "Colour.h"
#include "ColourMap.h"
#include <iostream>


using namespace Rosegarden;
using std::cout;
using std::string;

// Some printing routines

void printRC(Colour const *temp)
{
    cout << "red:  " << temp->getRed() << " green: " << temp->getGreen() << " blue:  " << temp->getBlue() << "\n";
}

void printSRC(const string *s, const Colour *c)
{
    cout << "name: " << *s << "  ";
    printRC(c);
}

void printSIRC(const unsigned int *i, const string *s, const Colour *c)
{
    cout << "index:  " << *i << "  ";
    printSRC(s, c);
}

void printIteratorContents (ColourMap *input)
{
    RCMap::const_iterator iter = input->begin();
    for ( ; !(iter == input->end()) ; ++iter)
        printSIRC(&(iter->first), &(iter->second.second), &(iter->second.first));
}

// The main test program
int main()
{
    cout << "TEST: Colour.C\n\n";
    cout << "Can we create an Colour with the right default values?\n";
    Colour *red = new Colour;
    printRC(red);

    cout << "Can we set values; green here is invalid - it should be set to 0 instead\n";
    red->setRed(210);
    red->setGreen(276);
    red->setBlue(100);

    cout << "Testing the copy constructor\n";
    Colour *blue = new Colour(*red);
    printRC(blue);

    cout << "Check operator= works\n";
    Colour green;
    green = *red;
    printRC(&green);

    cout << "Check the setColour routine\n";
    green.setColour(1,2,3);
    printRC(&green);

    cout << "Check the getColour routine\n";
    unsigned int r, g, b;
    green.getColour(r, g, b);
    printRC(&green);

    cout << "\nTEST: ColourMap.C\n\n";
    cout << "Can we create a ColourMap with the right default Colour + String\n";
    ColourMap *map = new ColourMap();

    cout << "Can we get the default colour back out of it?\n";
    string s1 = map->getNameByIndex(0);
    green = map->getColourByIndex(0);
    printSRC(&s1, &green);

    cout << "Can we create a ColourMap with a specified default Colour?\n";
    ColourMap *map2 = new ColourMap(*red);

    cout << "Can we get the information back out of it?\n";
    s1 = map2->getNameByIndex(0);
    green = map2->getColourByIndex(0);
    printSRC(&s1, &green);

    cout << "Can we add a Colour\n";
    s1 = "TEST1";
    green.setColour(100, 101, 102);
    map2->addItem(green, s1);

    cout << "Can we get the info back out?\n";
    s1 = "";
    s1 = map2->getNameByIndex(1);
    green = map2->getColourByIndex(1);
    printSRC(&s1, &green);

    cout << "Add a couple more colours\n";
    s1 = "TEST2";
    green.setColour(101, 102, 103);
    map2->addItem(green, s1);
    s1 = "TEST3";
    green.setColour(102, 103, 104);
    map2->addItem(green, s1);
    s1 = "TEST4";
    green.setColour(103, 104, 105);
    map2->addItem(green, s1);

    // From an iterator:
    //        iterator->first         ==> Index
    //        iterator->second.first  ==> Colour
    //        iterator->second.second ==> string
    // This rather unwieldy notation is because we store a pair in the map which is made up of a pair
    //  to start with
    printIteratorContents(map2);

    cout << "Now try deleting the third item\n";
    map2->deleteItemByIndex(3);

    // Print the map again
    printIteratorContents(map2);

    cout << "Make sure we get false when we try and modify item number 3\n";
    s1 = "NO";
    green.setColour(199,199,199);
    bool check = map2->modifyColourByIndex(3, green);
    if (check) cout << "WARNING: Managed to modify colour which doesn't exist\n";
    check = map2->modifyNameByIndex(3, s1);
    if (check) cout << "WARNING: Managed to modify name which doesn't exist\n";

    cout << "Check we can modify a colour which *is* there\n";
    s1 = "YES";
    green.setColour(233,233,233);

    check = map2->modifyColourByIndex(4, green);
    if (!check) cout << "WARNING: Couldn't modify colour which does exist\n";

    check = map2->modifyNameByIndex(4, s1);
    if (!check) cout << "WARNING: Couldn't modify name which does exist\n";

    // Print the map again
    printIteratorContents(map2);

    cout << "Now try adding another item - it should take the place of the one we removed.\n";
    s1 = "NEW";
    green.setColour(211, 212, 213);
    map2->addItem(green, s1);

    // Print the map again
    printIteratorContents(map2);

    cout << "Try swapping two items:\n";
    check = map2->swapItems(3, 4);
    if (!check) cout << "WARNING: Couldn't swap two items which both exist\n";

    // Print the map again
    printIteratorContents(map2);

    cout << "\nTEST: Generic Colour routines\n\n";

    cout << "Try getting a combination colour:\n";
    Colour blah = map2->getColourByIndex(0);
    Colour blah2 = map2->getColourByIndex(1);
    cout << "Original colours:\n";
    printRC(&blah);
    printRC(&blah2);
    cout << "Combination colour:\n";
    blah = getCombinationColour(blah, blah2);
    printRC(&blah);

    // Test the XML output
    cout << "\nTEST: XML Output\n\n";
    cout << "For a single colour:\n";
    cout << blah.toXmlString();

    cout << "For a colourmap:\n";
    cout << map2->toXmlString(std::string("segmentmap"));


    delete map;
    delete map2;
    delete red;
    delete blue;

    return 0;
}
