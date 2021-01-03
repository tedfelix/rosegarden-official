/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TUNING_H_
#define TUNING_H_


#include <vector>
#include <string>
#include <map>

#include <QXmlStreamReader>

#include "base/NotationTypes.h"

//!!! Q: Should this class be using QString and QVector also?
//!!! A: A definite "probably". We use the Qt classes in the
//!!!    PitchDetector, but that is RG specific. This code is
//!!!    more use in other n-ism projects, and is easier to
//!!!    break (more head-scratching, less boiler-plate) so
//!!!    for now it's implemented with the std:: classes. - njb

typedef std::pair< std::string, int > Spelling;
typedef std::map< std::string, int > SpellingList;
typedef SpellingList::iterator SpellingListIterator;
typedef std::vector< double > IntervalList;
typedef std::vector< double >::const_iterator IntervalListIterator;

namespace Rosegarden {
namespace Accidentals {

/**
 * \addtogroup Codicil
 * \@{
 * \brief Read the Codicil's tunings file and create a tunings database
 *
 * This is part of the network for Interdisciplinary research in
 * Science and Music's "Rosegarden Codicil" project.
 *
 * http://www.n-ism.org/Projects/microtonalism.php
 *
 * \author Dougie McGilvray, Nick Bailey
 * \date 2010
 */
class Tuning {

 public: 

  /**
   * \brief Construct a tuning from its name, and interval and spellings.
   *
   * \param name Name of the new tuning
   * \param intervals List of intervals in cents from the root pitch
   * \param spellings List of spellings (enharmonic equivalents) for each pitch
   *        Spellings which do not have associated intervals will be deleted.
   */
  Tuning(const std::string name,
         const IntervalList *intervals, 
	 SpellingList *spellings);
  Tuning(const Tuning *tuning);

  /**
   * \brief Access the vector of tunings known to the system
   */
  static std::vector<Tuning*>* getTunings();

  /**
   * \brief Set the frequency associated with the reference pitch
   *
   * \param pitch The reference pitch
   * \param freq Associated frequency (in Hz)
   */
  void setRefNote(Rosegarden::Pitch pitch, double freq);

  /**
   * \brief Nominate the root pitch for this tuning
   *
   * \param pitch The root pitch
   */
  void setRootPitch(Rosegarden::Pitch pitch);

  /**
   * \brief Calculate a frequency
   *
   * \param pitch The pitch for which a frequency is required
   * \return Frequency in Hz.
   */
  double getFrequency(Rosegarden::Pitch pitch) const;

  const std::string getName() const;      /**< Get the Tuning's name */
  SpellingList *getSpellingList() const;  /**< Get the enharmonic spellings */
  const IntervalList *getIntervalList() const;  /**< Get intervals in cents*/
  Rosegarden::Pitch getRootPitch() const; /**< Get the root pitch */
  Rosegarden::Pitch getRefPitch() const;  /**< Get the reference pitch */
  double getRefFreq() const;              /**< Get the reference frequency */
  void printTuning() const;               /**< Print the Tuning (debugging) */

 protected:

  /** Converts pitch to string */
  std::string getSpelling(Rosegarden::Pitch &pitch) const;
  /** An interval in Scala can be represented as a ratio <int>/<int>
      or as a number of cents (must contain a "."). Convert such a
      represntation to a (double)number of cents */
  static double scalaIntervalToCents(const QString & interval,
                                     const qint64 lineNumber);
  /** Parse a note and associate it in the spelling list
      with the most recent interval */
  static void parseSpelling(QString note,
                            IntervalList *intervals,
                            SpellingList *spellings);
  /** Create and cache a new Tuning */
  static void saveTuning(const QString &tuningName,
                         const IntervalList *intervals,
                         SpellingList *spellings);
  const std::string m_name;
  Rosegarden::Pitch m_rootPitch;
  int m_rootPosition;
  Rosegarden::Pitch m_refPitch;
  int m_refOctave;
  int m_cPosition;
  double  m_refFreq;
  double m_cRefFreq;
  const IntervalList *m_intervals;
  int m_size;
  SpellingList *m_spellings;

  typedef std::map<const int, const Accidental *> AccMap;
  static AccMap accMap;
  static const unsigned int accMapSize;
  static const AccMap::value_type accMapData[];
  static std::vector<Tuning*> m_tunings;
  
};

} // end of namespace Accidentals
} // end of namespace Rosegarden

/**\@}*/

#endif
