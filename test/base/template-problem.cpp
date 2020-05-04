// Some attempts at reproducing the func-template-within-template problem
//
// Probably no longer relevant.

enum FooType {A, B, C};

class Foo
{
public:
    template<FooType T> void func();
};

template<class T>
void Foo::func()
{
    // dummy code
    T j = 0;
    for(T i = 0; i < 100; ++i) j += i;
}

//template void Foo::func<int>();

template <class R>
class FooR
{
public:
    void rfunc();
};

template<class R>
void FooR<R>::rfunc()
{
    // this won't compile
    Foo* foo;
    foo->func<A>();
}

void templateTest()
{
    Foo foo;
    foo.func<A>();

//     FooR<float> foor;
//     foor.rfunc();
}


template <class Element, class Container>
class GenericSet // abstract base
{
public:
    typedef typename Container::iterator Iterator;

    /// Return true if this element, known to test() true, is a set member
    virtual bool sample(const Iterator &i);
};


template <class Element, class Container>
bool
GenericSet<Element, Container>::sample(const Iterator &i)
{
    Event *e;
    long p = e->get<Int>("blah");
}

