
#include "../FastVector.h"
#include <iostream>

class Thing
{
public:
    Thing(int x) : m_x(x) { }
    ~Thing() { }

    int getX() const { return m_x; }

private:
    int m_x;
};

main()
{
    FastVector<Thing> v;

    for (int i = 0; i < 20; ++i) {
        v.push_back(Thing(i * 2));
        v.push_front(Thing(i));
    }

    for (int i = 0; i < v.size(); ++i) {
        std::cout << "v[" << i << "] is " << v[i].getX() << std::endl;
    }

    const FastVector<Thing> *w = &v;

    for (FastVector<Thing>::const_iterator j = w->begin(); j != w->end(); ++j) {
        std::cout << "item is " << j->getX() << std::endl;
    }

    std::cout << std::endl;

    for (FastVector<Thing>::reverse_iterator rj = v.rbegin();
         rj != v.rend(); ++rj) {
        std::cout << "item is " << rj->getX() << std::endl;
    }
}

