
#include "FastVector_new.h"
#include <iostream>

class Thing
{
public:
    Thing(int x) : m_x(x) { }
    ~Thing() { }

    int getX() { return m_x; }

private:
    int m_x;
};

main()
{
    FastVector<Thing> v;

    for (int i = 0; i < 20; ++i) {
        v.push_back(Thing(i));
        v.push_front(Thing(i));
    }

    for (int i = 0; i < v.size(); ++i) {
        std::cout << "v[" << i << "] is " << v[i].getX() << std::endl;
    }
}

