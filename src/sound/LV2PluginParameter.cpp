/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2PluginParameter]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2PluginParameter.h"

#include "sound/LV2URIDMapper.h"

#include <lv2/atom/forge.h>

namespace Rosegarden
{

LV2PluginParameter::LV2PluginParameter() :
    m_parameterUrid(0),
    m_readable(false),
    m_writable(false),
    m_type(AudioPluginInstance::ParameterType::UNKNOWN)
{
    initUrids();
}

LV2PluginParameter::LV2PluginParameter
(const QString& uri,
 AudioPluginInstance::ParameterType type) :
    m_parameterUrid(0),
    m_readable(false),
    m_writable(false),
    m_type(type)
{
    std::string uris = uri.toStdString();
    m_parameterUrid = LV2URIDMapper::uridMap(uris.c_str());
    initUrids();
}

bool LV2PluginParameter::isReadable() const
{
    return m_readable;
}

void LV2PluginParameter::setReadable(bool b)
{
    m_readable = b;
}

bool LV2PluginParameter::isWritable() const
{
    return m_writable;
}

void LV2PluginParameter::setWritable(bool b)
{
    m_writable = b;
}

AudioPluginInstance::ParameterType LV2PluginParameter::getType() const
{
    return m_type;
}

QString LV2PluginParameter::getLabel() const
{
    return m_label;
}

void LV2PluginParameter::setLabel(const QString& label)
{
    m_label = label;
}

LV2_URID LV2PluginParameter::getParameterUrid() const
{
    return m_parameterUrid;
}

bool LV2PluginParameter::isValueSet() const
{
    return (m_value.size() > 0);
}

const LV2_Atom* LV2PluginParameter::getValue() const
{
    LV2_Atom* atom = (LV2_Atom *)(m_value.data());
    return atom;
}

void LV2PluginParameter::setValue(const LV2_Atom* atom)
{
    // store the value atom in the bytearray
    int alen = atom->size + sizeof(LV2_Atom);
    m_value.clear();
    m_value.append((char *)atom, alen);
}

int LV2PluginParameter::getInt() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::INT);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_intUrid);
    int* intp = (int *)(atom + 1);
    return *intp;
}

long LV2PluginParameter::getLong() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::LONG);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_longUrid);
    long* longp = (long *)(atom + 1);
    return *longp;
}

float LV2PluginParameter::getFloat() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::FLOAT);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_floatUrid);
    float* floatp = (float *)(atom + 1);
    return *floatp;
}

double LV2PluginParameter::getDouble() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::DOUBLE);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_doubleUrid);
    double* doublep = (double *)(atom + 1);
    return *doublep;
}

bool LV2PluginParameter::getBool() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::BOOL);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_boolUrid);
    bool* boolp = (bool *)(atom + 1);
    return *boolp;
}

QString LV2PluginParameter::getString() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::STRING);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_stringUrid);
    char* charp = (char *)(atom + 1);
    return QString(charp);
}

QString LV2PluginParameter::getPath() const
{
    Q_ASSERT(m_value.size() > 0);
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::PATH);
    LV2_Atom* atom = (LV2_Atom *)m_value.data();
    Q_ASSERT(atom->type == m_pathUrid);
    char* charp = (char *)(atom + 1);
    return QString(charp);
}

void LV2PluginParameter::setInt(int value)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::INT);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_int(&forge, value);
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

void LV2PluginParameter::setLong(long value)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::LONG);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_long(&forge, value);
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

void LV2PluginParameter::setFloat(float value)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::FLOAT);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_float(&forge, value);
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

void LV2PluginParameter::setDouble(double value)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::DOUBLE);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_double(&forge, value);
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

void LV2PluginParameter::setBool(bool value)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::BOOL);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_double(&forge, value);
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

void LV2PluginParameter::setString(const QString& stringValue)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::STRING);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    std::string svalue = stringValue.toStdString();
    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_string(&forge, svalue.c_str(), svalue.size());
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

void LV2PluginParameter::setPath(const QString& value)
{
    Q_ASSERT(m_type == AudioPluginInstance::ParameterType::PATH);
    LV2_Atom_Forge forge;
    lv2_atom_forge_init(&forge, LV2URIDMapper::getURIDMapFeature());
    uint8_t buffer[2000];
    lv2_atom_forge_set_buffer(&forge, buffer, sizeof(buffer));

    std::string svalue = value.toStdString();
    LV2_Atom_Forge_Ref aref =
        lv2_atom_forge_path(&forge, svalue.c_str(), svalue.size());
    const LV2_Atom *atom = lv2_atom_forge_deref(&forge, aref);

    int size = lv2_atom_total_size(atom);
    m_value.clear();
    m_value.append((char *)atom, size);
}

QString LV2PluginParameter::getValueAsString() const
{
    Q_ASSERT(m_type != AudioPluginInstance::ParameterType::UNKNOWN);
    Q_ASSERT(m_value.size() > 0);
    switch (m_type) {
    case AudioPluginInstance::ParameterType::INT:
        {
            int ival = getInt();
            return QString::number(ival);
        }
        break;
    case AudioPluginInstance::ParameterType::LONG:
        {
            long lval = getLong();
            return QString::number(lval);
        }
        break;
    case AudioPluginInstance::ParameterType::FLOAT:
        {
            float fval = getFloat();
            return QString::number(fval);
        }
        break;
    case AudioPluginInstance::ParameterType::DOUBLE:
        {
            double dval = getDouble();
            return QString::number(dval);
        }
        break;
    case AudioPluginInstance::ParameterType::BOOL:
        {
            bool bval = getBool();
            if (bval) return "true";
            else return "false";
        }
        break;
    case AudioPluginInstance::ParameterType::STRING:
        {
            QString sval = getString();
            return sval;
        }
        break;
    case AudioPluginInstance::ParameterType::PATH:
        {
            QString pval = getPath();
            return pval;
        }
        break;
    case AudioPluginInstance::ParameterType::UNKNOWN:
    default :
        Q_ASSERT(false);
        break;
    }
    return "";
}

void LV2PluginParameter::setValueFromString(const QString& string)
{
    switch (m_type) {
    case AudioPluginInstance::ParameterType::INT:
        {
            int ival = string.toInt();
            setInt(ival);
        }
        break;
    case AudioPluginInstance::ParameterType::LONG:
        {
            long lval = string.toInt();
            setLong(lval);
        }
        break;
    case AudioPluginInstance::ParameterType::FLOAT:
        {
            float fval = string.toDouble();
            setFloat(fval);
        }
        break;
    case AudioPluginInstance::ParameterType::DOUBLE:
        {
            double dval = string.toDouble();
            setDouble(dval);
        }
        break;
    case AudioPluginInstance::ParameterType::BOOL:
        {
            if (string == "true" || string == "1") setBool(true);
            else setBool(false);
        }
        break;
    case AudioPluginInstance::ParameterType::STRING:
        {
            setString(string);
        }
        break;
    case AudioPluginInstance::ParameterType::PATH:
        {
            setPath(string);
        }
        break;
    case AudioPluginInstance::ParameterType::UNKNOWN:
    default :
        Q_ASSERT(false);
        break;
    }
}

void LV2PluginParameter::initUrids()
{
    m_intUrid = LV2URIDMapper::uridMap(LV2_ATOM__Int);
    m_longUrid = LV2URIDMapper::uridMap(LV2_ATOM__Long);
    m_floatUrid = LV2URIDMapper::uridMap(LV2_ATOM__Float);
    m_doubleUrid = LV2URIDMapper::uridMap(LV2_ATOM__Double);
    m_boolUrid = LV2URIDMapper::uridMap(LV2_ATOM__Bool);
    m_stringUrid = LV2URIDMapper::uridMap(LV2_ATOM__String);
    m_pathUrid = LV2URIDMapper::uridMap(LV2_ATOM__Path);
}

}
