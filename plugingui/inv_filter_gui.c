/* 

    This LV2 extension provides lpf and hpf gui's

    (c) Fraser Stuart 2009

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <lv2.h>
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "widgets/widgets.h"
#include "widgets/knob.h"
#include "widgets/lamp.h"
#include "widgets/meter-peak.h"
#include "widgets/display-FrequencyGain.h"
#include "widgets/switch-toggle.h"
#include "../plugin/inv_filter.h"
#include "inv_filter_gui.h"


static LV2UI_Descriptor *IFilterGuiDescriptor = NULL;

typedef struct {
	GtkWidget	*windowContainer;
	GtkWidget	*heading;
	GtkWidget	*toggleBypass;
	GtkWidget	*meterIn;
	GtkWidget	*meterOut;
	GtkWidget	*display;
	GtkWidget	*knobFreq;
	GtkWidget	*knobGain;
	GtkWidget	*toggleNoClip;
	GtkWidget	*lampNoClip;

	gint		InChannels;
	gint		OutChannels;
	float		bypass;
	float		freq;
	float		gain;
	float 		noClip;

	LV2UI_Write_Function 	write_function;
	LV2UI_Controller 	controller;

} IFilterGui;



static LV2UI_Handle 
instantiateIFilterGui(const struct LV2UI_Descriptor* descriptor, const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
	IFilterGui *pluginGui = (IFilterGui *)malloc(sizeof(IFilterGui));
	if(pluginGui==NULL)
		return NULL;

	pluginGui->write_function = write_function;
	pluginGui->controller = controller;

	GtkBuilder      *builder; 
	GtkWidget       *window;
	GtkWidget	*tempObject;

	char 		*file;

	GError *err = NULL;

	gtk_init (NULL,NULL);

	builder = gtk_builder_new ();
	file = g_strdup_printf("%s/gtk/inv_filter_gui.xml",bundle_path);
	gtk_builder_add_from_file (builder, file, &err);
	free(file);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "filter_window"));

	/* get pointers to some useful widgets from the design */
	pluginGui->windowContainer = GTK_WIDGET (gtk_builder_get_object (builder, "filter_container"));
	pluginGui->heading = GTK_WIDGET (gtk_builder_get_object (builder, "label_heading"));

	/* add custom widgets */
	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_bypass_toggle"));
	pluginGui->toggleBypass = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleBypass);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_meter_in"));
	pluginGui->meterIn = inv_meter_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->meterIn);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_meter_out"));
	pluginGui->meterOut = inv_meter_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->meterOut);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_filter_display"));
	pluginGui->display = inv_display_fg_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->display);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_frequency_knob"));
	pluginGui->knobFreq = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobFreq);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_gain_knob"));
	pluginGui->knobGain = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobGain);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_noclip_toggle"));
	pluginGui->toggleNoClip = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleNoClip);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_noclip_lamp"));
	pluginGui->lampNoClip = inv_lamp_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->lampNoClip);

	/* customise for the plugin */
	if(!strcmp(plugin_uri,IFILTER_MONO_LPF_URI)) 
	{
		pluginGui->InChannels=1;
		pluginGui->OutChannels=1;
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Low Pass Filter (mono)</b>");
		inv_display_fg_set_mode(INV_DISPLAY_FG (pluginGui->display), INV_DISPLAYFG_MODE_LPF);
		inv_knob_set_highlight(INV_KNOB (pluginGui->knobFreq), INV_KNOB_HIGHLIGHT_R);
	}
	if(!strcmp(plugin_uri,IFILTER_MONO_HPF_URI)) 
	{
		pluginGui->InChannels=1;
		pluginGui->OutChannels=1;
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>High Pass Filter (mono)</b>");
		inv_display_fg_set_mode(INV_DISPLAY_FG (pluginGui->display), INV_DISPLAYFG_MODE_HPF);
		inv_knob_set_highlight(INV_KNOB (pluginGui->knobFreq), INV_KNOB_HIGHLIGHT_L);
	}
	if(!strcmp(plugin_uri,IFILTER_STEREO_LPF_URI)) 
	{
		pluginGui->InChannels=2;
		pluginGui->OutChannels=2;
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Low Pass Filter (stereo)</b>");
		inv_display_fg_set_mode(INV_DISPLAY_FG (pluginGui->display), INV_DISPLAYFG_MODE_LPF);
		inv_knob_set_highlight(INV_KNOB (pluginGui->knobFreq), INV_KNOB_HIGHLIGHT_R);
	}
	if(!strcmp(plugin_uri,IFILTER_STEREO_HPF_URI)) 
	{
		pluginGui->InChannels=2;
		pluginGui->OutChannels=2;
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>High Pass Filter (stereo)</b>");
		inv_display_fg_set_mode(INV_DISPLAY_FG (pluginGui->display), INV_DISPLAYFG_MODE_HPF);
		inv_knob_set_highlight(INV_KNOB (pluginGui->knobFreq), INV_KNOB_HIGHLIGHT_L);
	}

	pluginGui->bypass=0.0;
	pluginGui->freq=1000.0;
	pluginGui->gain=0.0;
	pluginGui->noClip=0.0;

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, "Active");
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0, 0.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  "Bypassed");
	inv_switch_toggle_set_state( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), "<span size=\"8000\"><b>Description:</b> This switch bypasses the plugin.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleBypass),"button-release-event",G_CALLBACK(on_inv_filter_bypass_toggle_button_release),pluginGui);

	inv_meter_set_bypass(INV_METER (pluginGui->meterIn),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterIn), INV_METER_DRAW_MODE_TOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterIn), pluginGui->InChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterIn),-90);
	inv_meter_set_RdB(INV_METER (pluginGui->meterIn),-90);

	inv_meter_set_bypass(INV_METER (pluginGui->meterOut),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterOut), INV_METER_DRAW_MODE_TOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterOut), pluginGui->OutChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterOut),-90);
	inv_meter_set_RdB(INV_METER (pluginGui->meterOut),-90);

	inv_display_fg_set_bypass(INV_DISPLAY_FG (pluginGui->display), INV_PLUGIN_ACTIVE);
	inv_display_fg_set_freq(INV_DISPLAY_FG (pluginGui->display), pluginGui->freq);
	inv_display_fg_set_gain(INV_DISPLAY_FG (pluginGui->display), pluginGui->gain);
	g_signal_connect_after(G_OBJECT(pluginGui->display),"motion-notify-event",G_CALLBACK(on_inv_filter_display_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobFreq), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobFreq), INV_KNOB_SIZE_LARGE);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobFreq), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobFreq), INV_KNOB_MARKINGS_4); 
	inv_knob_set_human(INV_KNOB (pluginGui->knobFreq)); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobFreq), "Hz");
	inv_knob_set_min(INV_KNOB (pluginGui->knobFreq), 20.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobFreq), 20000.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobFreq), pluginGui->freq);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobFreq), "<span size=\"8000\"><b>Description:</b> This knob sets the frequency the filter is centered at.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobFreq),"motion-notify-event",G_CALLBACK(on_inv_filter_freq_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobGain), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobGain), INV_KNOB_SIZE_LARGE);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobGain), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobGain), INV_KNOB_MARKINGS_5);
	inv_knob_set_highlight(INV_KNOB (pluginGui->knobGain), INV_KNOB_HIGHLIGHT_L);
	inv_knob_set_units(INV_KNOB (pluginGui->knobGain), "dB");
	inv_knob_set_min(INV_KNOB (pluginGui->knobGain), 0.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobGain), 12.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobGain), pluginGui->gain);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobGain), "<span size=\"8000\"><b>Description:</b> This knob sets the output gain of the filter.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobGain),"motion-notify-event",G_CALLBACK(on_inv_filter_gain_knob_motion),pluginGui);

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF, "Off");
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON,  0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON,  "Active");
	inv_switch_toggle_set_state( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), "<span size=\"8000\"><b>Description:</b> This switch activates soft-clipping on the output. The soft clipping function outputs a value between -3dB and 0dB for input values between -3dB and +infinity.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleNoClip),"button-release-event",G_CALLBACK(on_inv_filter_noClip_toggle_button_release),pluginGui);

	inv_lamp_set_value(INV_LAMP (pluginGui->lampNoClip),0.0);
	inv_lamp_set_scale(INV_LAMP (pluginGui->lampNoClip),3.0);
	inv_lamp_set_tooltip(INV_LAMP (pluginGui->lampNoClip), "<span size=\"8000\">This glows when soft clipping is occurring.</span>");

	/* strip the parent window from the design so the host can attach its own */
	gtk_widget_ref(pluginGui->windowContainer);
	gtk_container_remove(GTK_CONTAINER(window), pluginGui->windowContainer);

	*widget = (LV2UI_Widget) pluginGui->windowContainer;

	g_object_unref (G_OBJECT (builder));
             
	/* return the instance */
	return pluginGui;
}


static void 
cleanupIFilterGui(LV2UI_Handle ui)
{
	return;
}


static void 
port_eventIFilterGui(LV2UI_Handle ui, uint32_t port, uint32_t buffer_size, uint32_t format, const void*  buffer)
{
	IFilterGui *pluginGui = (IFilterGui *)ui;

	float value;

	if(format==0) 
	{
		value=* (float *) buffer;
		switch(port)
		{
			case IFILTER_BYPASS:
				pluginGui->bypass=value;
				if(value <= 0.0) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
					inv_meter_set_bypass(INV_METER (pluginGui->meterIn),INV_PLUGIN_ACTIVE);
					inv_meter_set_bypass(INV_METER (pluginGui->meterOut),INV_PLUGIN_ACTIVE);
					inv_display_fg_set_bypass(INV_DISPLAY_FG (pluginGui->display), INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(INV_KNOB (pluginGui->knobFreq), INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(INV_KNOB (pluginGui->knobGain), INV_PLUGIN_ACTIVE);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_PLUGIN_ACTIVE);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON);
					inv_meter_set_bypass(INV_METER (pluginGui->meterIn),INV_PLUGIN_BYPASS);
					inv_meter_set_bypass(INV_METER (pluginGui->meterOut),INV_PLUGIN_BYPASS);
					inv_display_fg_set_bypass(INV_DISPLAY_FG (pluginGui->display), INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(INV_KNOB (pluginGui->knobFreq), INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(INV_KNOB (pluginGui->knobGain), INV_PLUGIN_BYPASS);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_PLUGIN_BYPASS);
				}
				gtk_widget_queue_draw (pluginGui->windowContainer);
				break;
			case IFILTER_FREQ:
				pluginGui->freq=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobFreq), pluginGui->freq);
				inv_display_fg_set_freq(INV_DISPLAY_FG (pluginGui->display), pluginGui->freq);
				break;
			case IFILTER_GAIN:
				pluginGui->gain=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobGain), pluginGui->gain);
				inv_display_fg_set_gain(INV_DISPLAY_FG (pluginGui->display), pluginGui->gain);
				break;
			case IFILTER_NOCLIP:
				pluginGui->noClip=value;
				if(value <= 0.0) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON);
				}
				break;
			case IFILTER_METER_INL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterIn),value);
				break;
			case IFILTER_METER_INR:
				if(pluginGui->InChannels==2) inv_meter_set_RdB(INV_METER (pluginGui->meterIn),value);
				break;
			case IFILTER_METER_OUTL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterOut),value);
				break;
			case IFILTER_METER_OUTR:
				if(pluginGui->OutChannels==2) inv_meter_set_RdB(INV_METER (pluginGui->meterOut),value);
				break;
			case IFILTER_METER_DRIVE:
				inv_lamp_set_value(INV_LAMP (pluginGui->lampNoClip),value);
				break;
		}
	}
}


static void 
init()
{
	IFilterGuiDescriptor =
	 (LV2UI_Descriptor *)malloc(sizeof(LV2UI_Descriptor));

	IFilterGuiDescriptor->URI 		= IFILTER_GUI_URI;
	IFilterGuiDescriptor->instantiate 	= instantiateIFilterGui;
	IFilterGuiDescriptor->cleanup		= cleanupIFilterGui;
	IFilterGuiDescriptor->port_event	= port_eventIFilterGui;
	IFilterGuiDescriptor->extension_data 	= NULL;

}


const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
	if (!IFilterGuiDescriptor) init();

	switch (index) {
		case 0:
			return IFilterGuiDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/

static void 
on_inv_filter_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IFilterGui *pluginGui = (IFilterGui *) data;

	pluginGui->bypass=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, IFILTER_BYPASS, 4, 0, &pluginGui->bypass);
	return;
}

static void 
on_inv_filter_freq_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IFilterGui *pluginGui = (IFilterGui *) data;

	pluginGui->freq=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IFILTER_FREQ, 4, 0, &pluginGui->freq);
	return;
}

static void 
on_inv_filter_gain_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IFilterGui *pluginGui = (IFilterGui *) data;

	pluginGui->gain=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IFILTER_GAIN, 4, 0, &pluginGui->gain);
	return;
}

static void 
on_inv_filter_noClip_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IFilterGui *pluginGui = (IFilterGui *) data;

	pluginGui->noClip=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, IFILTER_NOCLIP, 4, 0, &pluginGui->noClip);
	return;
}


static void 
on_inv_filter_display_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IFilterGui *pluginGui = (IFilterGui *) data;

	pluginGui->freq=inv_display_fg_get_freq(INV_DISPLAY_FG (widget));
	pluginGui->gain=inv_display_fg_get_gain(INV_DISPLAY_FG (widget));
	(*pluginGui->write_function)(pluginGui->controller, IFILTER_FREQ, 4, 0, &pluginGui->freq);
	(*pluginGui->write_function)(pluginGui->controller, IFILTER_GAIN, 4, 0, &pluginGui->gain);
	return;
}

