
#include "notationgroup.h"
#include <cstring>

NotationGroup::NotationGroup(const NotationElementList &nel,
                             NELIterator elementInGroup) :
    m_nel(nel),
    m_type(Beamed)
{
    if (i == nel.end()) return;

    NotationElementList::IteratorPair pair
        (nel.findContainingSet<GroupMembershipTest>
         (i, GroupMembershipTest(i)));

    for (i = pair.first; i != pair.second; ++i) {
        push_back(i);
    }

    if (size() > 0) {

        i = (*this)[0];

        try {
            string t = (*i)->event()->get<String>("GroupType");
            if (strcasecmp(t.c_str(), "beamed")) {
                m_type = Beamed;
            } else if (strcasecmp(t.c_str(), "tupled")) {
                m_type = Tupled;
            } else if (strcasecmp(t.c_str(), "grace")) {
                m_type = Grace;
            } else {
                kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: Unknown GroupType \"" << t << "\", defaulting to Beamed" << endl;
            }
        } catch (NoData) {
            kdDebug(KDEBUG_AREA) << "NotationGroup::NotationGroup: Warning: No GroupType in grouped element, defaulting to Beamed" << endl;
        }
    }
}

NotationGroup::~NotationGroup() { }

