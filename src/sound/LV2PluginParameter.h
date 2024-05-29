/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef LV2PLUGINPARAMETER_H
#define LV2PLUGINPARAMETER_H

#include <QString>

#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>

#include "base/AudioPluginInstance.h"

/// The class representing an lv2 plugin parameter
/**
  * This class holds the data for a single lv2 plugin parameter. These
  * may be of different types (see the ParameterType enum). The
  * parameters are set using a patch Set message or values can be read
  * with a patch Get message. Some plugins can only be configured
  * using parameters - gor example the liquidsfz plugin where the sfz
  * file is set as a parameter.
  */

namespace Rosegarden
{

class LV2PluginParameter
{
 public:
    typedef std::map<QString, LV2PluginParameter> Parameters;

    // constructors
    LV2PluginParameter();
    LV2PluginParameter(const QString& uri,
                       AudioPluginInstance::ParameterType type);
    LV2PluginParameter(const LV2PluginParameter&) = default;

    /// get the type of the parameter
    AudioPluginInstance::ParameterType getType() const;

    /// get and set methods for the data elements
    /// is the parameter readable
    bool isReadable() const;
    void setReadable(bool b);

    /// is the parameter writable
    bool isWritable() const;
    void setWritable(bool b);

    /// the parameter label
    QString getLabel() const;
    void setLabel(const QString& label);

    /// the parameter urid
    LV2_URID getParameterUrid() const;

    /// has the value been set - if not the value is undefined
    bool isValueSet() const;

    /// the value as an atom
    const LV2_Atom* getValue() const;
    void setValue(const LV2_Atom* atom);

    /// the get functions for the vaious parameter types
    int getInt() const;
    long getLong() const;
    float getFloat() const;
    double getDouble() const;
    bool getBool() const;
    QString getString() const;
    QString getPath() const;

    /// the set functions for the vaious parameter types
    void setInt(int value);
    void setLong(long value);
    void setFloat(float value);
    void setDouble(double value);
    void setBool(bool value);
    void setString(const QString& stringValue);
    void setPath(const QString& value);

    /// get and set from string
    QString getValueAsString() const;
    void setValueFromString(const QString& string);

    // some urids for the parameter types
    LV2_URID m_intUrid;
    LV2_URID m_longUrid;
    LV2_URID m_floatUrid;
    LV2_URID m_doubleUrid;
    LV2_URID m_boolUrid;
    LV2_URID m_stringUrid;
    LV2_URID m_pathUrid;

 private:

    void initUrids();

    LV2_URID m_parameterUrid;
    bool m_readable;
    bool m_writable;
    QString m_label;
    AudioPluginInstance::ParameterType m_type;
    QByteArray m_value;

};

}

#endif
