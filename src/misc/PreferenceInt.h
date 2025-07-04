/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2023-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#pragma once

#include <QSettings>
#include <QString>

namespace Rosegarden
{


/// Integer Preference
/**
 * Fulfills the following requirements:
 *
 *   - Avoids confusing calls to QSettings::beginGroup()/endGroup().
 *   - Performance.  Caching to avoid hitting the .conf file.
 *     Single bool compare to detect cache validity.
 *   - Avoiding restarts.  By calling the setters, the
 *     Preferences dialogs can have a direct effect on cached
 *     values.  The user need not restart Rosegarden to see
 *     these changes take effect.
 *   - Consistent high-performance "write on first read" behavior to
 *     make it easier to find new .conf values during development.
 *
 * Was thinking about doing a template, but there are finicky little
 * differences between the types (QVariant conversions).
 *
 * ??? rename: SettingsInt?  It's not just for preferences.
 */
class PreferenceInt
{
public:
    PreferenceInt(const QString& group, const QString& key, int defaultValue) :
        m_group(group),
        m_key(key),
        m_defaultValue(defaultValue)
    {
    }

    void set(int value)
    {
        QSettings settings;
        settings.beginGroup(m_group);
        settings.setValue(m_key, value);
        m_cache = value;
    }

    int get() const
    {
        if (!m_cacheValid) {
            m_cacheValid = true;

            QSettings settings;
            settings.beginGroup(m_group);
            m_cache = settings.value(m_key, m_defaultValue).toInt();
            // Write it back out so we can find it if it wasn't there.
            settings.setValue(m_key, m_cache);
        }

        return m_cache;
    }

private:
    QString m_group;
    QString m_key;

    int m_defaultValue;

    mutable bool m_cacheValid = false;
    mutable int m_cache{};
};


}
