//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#ifndef __PLUGIN_VST_H__
#define __PLUGIN_VST_H__

#include "plugin.h"
#include "plugingui.h"
#include "song.h"

#include <math.h>

#ifndef kVstVersion
#define kVstVersion 2400
#endif

intptr_t VstHostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    VstPlugin* plugin = (effect && effect->user) ? (VstPlugin*)effect->user : 0;

    switch (opcode)
    {
    case audioMasterAutomate:
        if (plugin)
        {
            plugin->setParameterValue(index, opt);
            if (plugin->gui())
                plugin->gui()->setParameterValue(index, opt);
        }
        return 1; // FIXME?

    case audioMasterVersion:
        return kVstVersion;

    case audioMasterIdle:
        if (effect)
            effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0.0f);
        return 1; // FIXME?

    case audioMasterGetTime:
        // TODO
        return 0;

    case audioMasterTempoAt:
        // Deprecated in VST SDK 2.4
        // TODO
        return 0;

    case audioMasterGetSampleRate:
        return sampleRate;

    case audioMasterGetNumAutomatableParameters:
        // Deprecated in VST SDK 2.4
        // TODO
        return 0;

    case audioMasterIOChanged:
        if (plugin)
        {
            if (plugin->active())
            {
                plugin->dispatcher(effStopProcess, 0, 0, 0, 0.0f);
                plugin->dispatcher(effMainsChanged, 0, 0, 0, 0.0f);
            }

            plugin->reload();

            if (plugin->active())
            {
                plugin->dispatcher(effMainsChanged, 0, 1, 0, 0.0f);
                plugin->dispatcher(effStartProcess, 0, 0, 0, 0.0f);
            }
        }
        return 1;

    case audioMasterGetBlockSize:
        return 512;

    case audioMasterGetVendorString:
        if (ptr) strcpy((char*)ptr, "OpenOctave");
        return 1;

    case audioMasterGetProductString:
        if (ptr) strcpy((char*)ptr, "OpenOctave MIDI");
        return 1;

    case audioMasterGetVendorVersion:
        return 2012;

    case audioMasterCanDo:
        if (strcmp((char*)ptr, "sendVstEvents") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstMidiEvent") == 0)
            return 1;
        else if (strcmp((char*)ptr, "sendVstTimeInfo") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstEvents") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstMidiEvent") == 0)
            return 1;
        else if (strcmp((char*)ptr, "receiveVstTimeInfo") == 0)
            return -1;
        else if (strcmp((char*)ptr, "sizeWindow") == 0)
            return 1;
        else if (strcmp((char*)ptr, "acceptIOChanges") == 0)
            return 1;
        else
            return 0;

    case audioMasterGetLanguage:
        return kVstLangEnglish;

    default:
        return 0;
    }

    Q_UNUSED(effect);
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(opt);
}

VstPlugin::VstPlugin()
{
    m_type = PLUGIN_VST;

    effect = 0;
    events.numEvents = 0;
    events.reserved  = 0;
    isOldSdk = false;
}

VstPlugin::~VstPlugin()
{
}

void VstPlugin::initPluginI(PluginI* plugi, const QString& filename, const QString& label, const void* nativeHandle)
{
    AEffect* effect = (AEffect*)nativeHandle;

    plugi->m_audioInputCount = effect->numInputs;
    plugi->m_audioOutputCount = effect->numOutputs;

    char buf_str[255] = { 0 };
    effect->dispatcher(effect, effGetEffectName, 0, 0, buf_str, 0.0f);

    if (buf_str[0] != 0)
        plugi->m_name = QString(buf_str);
    else
        plugi->m_name = QString(label);

    buf_str[0] = 0;
    effect->dispatcher(effect, effGetVendorString, 0, 0, buf_str, 0.0f);
    if (buf_str[0] != 0)
        plugi->m_maker = QString(buf_str);

    if (effect->flags & effFlagsIsSynth)
        plugi->m_hints |= PLUGIN_IS_SYNTH;
    else if (plugi->m_audioInputCount >= 1 && plugi->m_audioOutputCount >= 1)
        plugi->m_hints |= PLUGIN_IS_FX;

    if (effect->flags & effFlagsHasEditor)
        plugi->m_hints |= PLUGIN_HAS_NATIVE_GUI;

    if (effect->flags & effFlagsProgramChunks)
        plugi->m_hints |= PLUGIN_USES_CHUNKS;

    Q_UNUSED(filename);
}

bool VstPlugin::init(QString filename, QString label)
{
    set_last_error("Not implemented yet");
    return false;
}

void VstPlugin::reload()
{
}

void VstPlugin::reloadPrograms(bool)
{
}

QString VstPlugin::getParameterName(uint32_t index)
{
    return QString("");
}

void VstPlugin::setNativeParameterValue(uint32_t index, double value)
{
}

bool VstPlugin::hasNativeGui()
{
    return false;
}

void VstPlugin::showNativeGui(bool)
{
}

bool VstPlugin::nativeGuiVisible()
{
    return false;
}

void VstPlugin::updateNativeGui()
{
}

intptr_t VstPlugin::dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
    return effect->dispatcher(effect, opcode, index, value, ptr, opt);
}

void VstPlugin::process(uint32_t frames, float** src, float** dst)
{
}

void VstPlugin::bufferSizeChanged(uint32_t)
{
}

bool VstPlugin::readConfiguration(Xml& xml, bool readPreset)
{
    return false;
}

void VstPlugin::writeConfiguration(int level, Xml& xml)
{
}

#endif // __PLUGIN_VST_H__
