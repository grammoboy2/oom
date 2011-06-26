//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: ladspa_plugin.h $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett 
//  	(info@openoctave.org)
//=========================================================

#ifndef _OOM_LV2_PLUGIN_H_
#define _OOM_LV2_PLUGIN_H_
#define OOM_LV2_MIDI_EVENT_ID 1
#include "plugin.h"
#ifdef LILV_SUPPORT
#include <lilv/lilv.h>
#endif
#ifdef SUIL_SUPPORT
#include <suil/suil.h>
#endif
#ifdef SLV2_SUPPORT
#include <slv2/slv2.h>
#endif
#include "lv2_external_ui.h"
#include "lv2/lv2plug.in/ns/ext/data-access/data-access.h"
#include <QHash>
#include <QList>
#include <QObject>
#define LV2_FIFO_SIZE 1024

#define UITYPE_QT4  1
#define UITYPE_GTK2 2
#define UITYPE_EXT  4

class QX11EmbedContainer;

struct LV2Data
{
	int frame;
	float value;
	unsigned time;
};

class LV2ControlFifo/*{{{*/
{
    LV2Data fifo[LV2_FIFO_SIZE];
    volatile int size;
    int wIndex;
    int rIndex;

public:

    LV2ControlFifo()
    {
        clear();
    }
    bool put(const LV2Data& event); // returns true on fifo overflow
    LV2Data get();
    const LV2Data& peek(int n = 0);
    void remove();

    bool isEmpty() const
    {
        return size == 0;
    }

    void clear()
    {
        size = 0, wIndex = 0, rIndex = 0;
    }

    int getSize() const
    {
        return size;
    }
};/*}}}*/

struct LV2World {/*{{{*/
#ifdef SLV2_SUPPORT
	SLV2World world;
	SLV2Plugins plugins;
	SLV2Instance instance;
	SLV2Value input_class;
	SLV2Value output_class;
	SLV2Value control_class;
	SLV2Value event_class;
	SLV2Value audio_class;
	SLV2Value midi_class;
	SLV2Value opt_class;
	SLV2Value qtui_class;
	SLV2Value gtkui_class;
	SLV2Value in_place_broken;
	SLV2Value toggle_prop;
	SLV2Value integer_prop;
	SLV2Value logarithmic_prop;
	SLV2Value samplerate_prop;
#else
	LilvWorld* world;
	const LilvPlugins* plugins;
	LilvInstance* instance;
	LilvNode* input_class;
	LilvNode* output_class;
	LilvNode* control_class;
	LilvNode* event_class;
	LilvNode* audio_class;
	LilvNode* midi_class;
	LilvNode* opt_class;
	LilvNode* qtui_class;
	LilvNode* gtkui_class;
	LilvNode* in_place_broken;
	LilvNode* toggle_prop;
	LilvNode* integer_prop;
	LilvNode* logarithmic_prop;
	LilvNode* samplerate_prop;
#endif
};/*}}}*/


class LV2Plugin : public Plugin
{
protected:
	void init(const char* uri);

private:
#ifdef SLV2_SUPPORT
	SLV2Plugin m_plugin;
#else
	LilvPlugin* m_plugin;
#endif

public:
	LV2Plugin(const char* uri);
#ifdef SLV2_SUPPORT
	SLV2Plugin getPlugin();
#else
	const LilvPlugin* getPlugin();
#endif
	//const LV2_Feature* const* features () { return m_features; }
    double defaultValue(unsigned int port) const;
    void lv2range(unsigned long i, float*, float*, float*) const;
    int updateReferences(int);
    const char* portName(unsigned long i);

	static const char* lv2_id_to_uri(uint32_t id);
	static uint32_t lv2_uri_to_id(const char *uri);
};

class LV2PluginI;

class LV2EventFilter : public QObject/*{{{*/
{
	Q_OBJECT
private slots:
	void closeWidget();

public:

	// Constructor.
	LV2EventFilter(LV2PluginI *p, QX11EmbedContainer *w);
	bool eventFilter(QObject*, QEvent*);

private:
	
	LV2PluginI *m_plugin;
	QX11EmbedContainer *m_widget;
};/*}}}*/

class LV2PluginI : public PluginI
{
private:
#ifdef SLV2_SUPPORT
	QList<SLV2Instance> m_instance;
	SLV2UIInstance m_slv2_ui_instance;
	LV2UI_Widget   m_lv2_ui_widget;
	lv2_external_ui_host m_lv2_ui_external;
	SLV2UIs        m_slv2_uis;
	SLV2UI         m_slv2_ui;
#else
	QList<LilvInstance*> m_instance;
#endif
#ifdef SUIL_SUPPORT
	QList<SuilInstance*> m_uinstance;
	SuilHost* m_uihost;
#endif
#ifdef GTK2UI_SUPPORT
	struct _GtkWidget *m_gtkWindow;
#endif
	LV2Plugin* m_plugin;
	QWidget* m_nativeui;
	bool m_guiVisible;
	int m_ui_type;
	LV2ControlFifo* m_controlFifo;
	LV2EventFilter* m_eventFilter;

	LV2_Feature  **m_features;
	LV2_Feature    m_data_access_feature;
	LV2_Feature    m_instance_access_feature;
	LV2_Extension_Data_Feature m_data_access;

	LV2_Feature    m_ui_feature;
	LV2_Feature  **m_ui_features;
public:
	LV2PluginI();
	~LV2PluginI();
	LV2Plugin* plugin() {
		return m_plugin;
	}
#ifdef SLV2_SUPPORT
	const LV2UI_Descriptor *lv2_ui_descriptor() const;
	LV2UI_Handle lv2_ui_handle() const;
	SLV2Instance instantiatelv2();
#else
	LilvInstance* instantiatelv2();
#endif
	LV2ControlFifo* getControlFifo(unsigned long p)
	{
		if(!m_controlFifo)
			return 0;
		return &m_controlFifo[p];
	}
    virtual bool initPluginInstance(Plugin*, int channels);
    virtual void connect(int ports, float** src, float** dst);
    virtual void activate();
    virtual void deactivate();
    virtual void apply(int n);
    virtual void setChannels(int);
	void showGui();
	void showGui(bool);
    virtual void showNativeGui();
    virtual void showNativeGui(bool);
	virtual bool nativeGuiVisible()
	{
		return m_guiVisible;
	}
	void makeGui();
	void makeNativeGui();
	void closeNativeGui();
    virtual bool isAudioIn(int k);
    virtual bool isAudioOut(int k);
    virtual void range(int i, float* min, float* max) const
    {
		float def;
        m_plugin->lv2range(controls[i].idx, &def, min, max);
    }
	int paramIndex(int i)
	{
		return controls[i].idx;
	}
	void heartBeat();
};
#endif