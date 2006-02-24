#include "barrelist.h"

namespace Guitar {
    /*---------------------------------------------------------------
                  BarreList
      ---------------------------------------------------------------*/

    void BarreList::push_back (Barre* obj)
    {
        if (obj != 0)
        {
            m_data.push_back(obj);
        }
    }

    BarreList::iterator BarreList::begin (void)
    {
        return m_data.begin();
    }

    BarreList::iterator BarreList::end (void)
    {
        return m_data.end();
    }

    bool BarreList::empty (void) const
    {
        return (m_data.size() == 0);
    }

    unsigned int const BarreList::size (void) const
    {
        return m_data.size();
    }

    void BarreList::erase (BarreList::iterator& pos)
    {
        m_data.erase (pos);
    }

    void BarreList::erase (Barre* bar_ptr)
    {
        std::vector<Barre*>::iterator pos = find (m_data.begin(), m_data.end(), bar_ptr);
        if (pos != m_data.end())
        {
            m_data.erase (pos);
        }
    }

}
