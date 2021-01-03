/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Version.h"


namespace Rosegarden
{
      Version::Version() :
            m_iMajor(-1),
            m_iMinor(-1),
            m_iMicro(-1)
      {
      }

      Version::~Version()
      {
      }

      bool Version::qstrtoversion(QString sVersion)
      {
        QString sVersion_major=sVersion.section('.', 0, 0);
        QString sVersion_minor=sVersion.section('.', 1, 1);
        QString sVersion_completeMicro=sVersion.section('.', 2, 2);
        QString sVersion_micro=sVersion_completeMicro.section('-', 0, 0);
        m_iMajor=sVersion_major.toInt();
        m_iMinor=sVersion_minor.toInt();
        m_iMicro=sVersion_micro.toInt();

        return true;
      }

      int Version::Major()
      {
        return m_iMajor;
      }

      int Version::Minor()
      {
        return m_iMinor;
      }

      int Version::Micro()
      {
        return m_iMicro;
      }

      bool Version::operator<=(Version other)
      {
        if (Major()<other.Major()) {
           return true;
        } else if (Major()==other.Major() && Minor()<other.Minor()) {
           return true;
        } else if (Major()==other.Major() && Minor()==other.Minor() && Micro()<=other.Micro()) {
           return true;
        }
        return false;
      }

}

