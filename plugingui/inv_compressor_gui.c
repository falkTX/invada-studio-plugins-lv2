/* 

    This LV2 extension provides compressor gui's

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
#include "lv2_ui.h"
#include "widgets/widgets.h"
#include "widgets/display-Compressor.h"
#include "widgets/knob.h"
#include "widgets/lamp.h"
#include "widgets/meter-peak.h"
#include "widgets/switch-toggle.h"
#include "../plugin/inv_compressor.h"
#include "inv_compressor_gui.h"


static LV2UI_Descriptor *ICompGuiDescriptor = NULL;

typedef struct {
	GtkWidget	*windowContainer;
	GtkWidget	*heading;
	GtkWidget	*toggleBypass;
	GtkWidget	*meterIn;
	GtkWidget	*meterGR;
	GtkWidget	*meterOut;
	GtkWidget	*display;
	GtkWidget	*knobRms;
	GtkWidget	*knobAttack;
	GtkWidget	*knobRelease;
	GtkWidget	*knobThreshold;
	GtkWidget	*knobRatio;
	GtkWidget	*knobGain;
	GtkWidget	*toggleNoClip;
	GtkWidget	*lampNoClip;

	gint		InChannels;
	gint		GRChannels;
	gint		OutChannels;
	float 		bypass;
	float		rms;
	float		attack;
	float		release;
	float		threshold;
	float		ratio;
	float		gain;
	float 		noClip;

	LV2UI_Write_Function 	write_function;
	LV2UI_Controller 	controller;

} ICompGui;



static LV2UI_Handle instantiateICompGui(const struct _LV2UI_Descriptor* descriptor, const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
	ICompGui *pluginGui = (ICompGui *)malloc(sizeof(ICompGui));
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
	file = g_strdup_printf("%s/gtk/inv_compressor_gui.xml",bundle_path);
	gtk_builder_add_from_file (builder, file, &err);
	free(file);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "comp_window"));

	/* get pointers to some useful widgets from the design */
	pluginGui->windowContainer = GTK_WIDGET (gtk_builder_get_object (builder, "comp_container"));
	pluginGui->heading = GTK_WIDGET (gtk_builder_get_object (builder, "label_heading"));

	/* add custom widgets */
	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_bypass_toggle"));
	pluginGui->toggleBypass = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleBypass);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_meter_in"));
	pluginGui->meterIn = inv_meter_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->meterIn);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_meter_gr"));
	pluginGui->meterGR = inv_meter_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->meterGR);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_meter_out"));
	pluginGui->meterOut = inv_meter_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->meterOut);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_comp_display"));
	pluginGui->display = inv_display_comp_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->display);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_rms_knob"));
	pluginGui->knobRms = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobRms);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_attack_knob"));
	pluginGui->knobAttack = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobAttack);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_release_knob"));
	pluginGui->knobRelease = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobRelease);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_threshold_knob"));
	pluginGui->knobThreshold = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobThreshold);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_ratio_knob"));
	pluginGui->knobRatio = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobRatio);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_makeupgain_knob"));
	pluginGui->knobGain = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobGain);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_noclip_toggle"));
	pluginGui->toggleNoClip = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleNoClip);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_noclip_lamp"));
	pluginGui->lampNoClip = inv_lamp_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->lampNoClip);

	/* customise for the plugin */
	if(!strcmp(plugin_uri,ICOMP_MONO_URI)) 
	{
		pluginGui->InChannels=1;
		pluginGui->OutChannels=1;
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Invada Compressor (mono)</b>");
	}
	if(!strcmp(plugin_uri,ICOMP_STEREO_URI)) 
	{
		pluginGui->InChannels=2;
		pluginGui->OutChannels=2;
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Invada Compressor (stereo)</b>");
	}

	pluginGui->GRChannels	= 1;
	pluginGui->bypass	= 0.0;
	pluginGui->rms		= 0.5;
	pluginGui->attack	= 0.015;
	pluginGui->release	= 0.050;
	pluginGui->threshold	= 0.0;
	pluginGui->ratio	= 1.0;
	pluginGui->gain		= 0.0;
	pluginGui->noClip	= 0.0;

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, "Active");
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0, 0.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  "Bypassed");
	inv_switch_toggle_set_state( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), "<span size=\"8000\"><b>Description:</b> This switch bypasses the plugin.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleBypass),"button-release-event",G_CALLBACK(on_inv_comp_bypass_toggle_button_release),pluginGui);

	inv_meter_set_bypass(INV_METER (pluginGui->meterIn),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterIn), INV_METER_DRAW_MODE_TOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterIn), pluginGui->InChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterIn),-90);
	inv_meter_set_RdB(INV_METER (pluginGui->meterIn),-90);

	inv_meter_set_bypass(INV_METER (pluginGui->meterGR),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterGR), INV_METER_DRAW_MODE_FROMZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterGR), pluginGui->GRChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterGR),0);

	inv_meter_set_bypass(INV_METER (pluginGui->meterOut),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterOut), INV_METER_DRAW_MODE_TOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterOut), pluginGui->OutChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterOut),-90);
	inv_meter_set_RdB(INV_METER (pluginGui->meterOut),-90);

	inv_display_comp_set_bypass(INV_DISPLAY_COMP (pluginGui->display), INV_PLUGIN_ACTIVE);
	inv_display_comp_set_rms(INV_DISPLAY_COMP (pluginGui->display), pluginGui->rms);
	inv_display_comp_set_attack(INV_DISPLAY_COMP (pluginGui->display), pluginGui->attack);
	inv_display_comp_set_release(INV_DISPLAY_COMP (pluginGui->display), pluginGui->release);
	inv_display_comp_set_threshold(INV_DISPLAY_COMP (pluginGui->display), pluginGui->threshold);
	inv_display_comp_set_ratio(INV_DISPLAY_COMP (pluginGui->display), pluginGui->ratio);
	inv_display_comp_set_gain(INV_DISPLAY_COMP (pluginGui->display), pluginGui->gain);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobRms),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobRms), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobRms), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobRms), INV_KNOB_MARKINGS_CUST10); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobRms), "");
	inv_knob_set_custom(INV_KNOB (pluginGui->knobRms), 0, "Peak");
	inv_knob_set_custom(INV_KNOB (pluginGui->knobRms), 1, "Fast RMS");
	inv_knob_set_custom(INV_KNOB (pluginGui->knobRms), 2, "RMS");
	inv_knob_set_min(INV_KNOB (pluginGui->knobRms), 0.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobRms), 1.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobRms), pluginGui->rms);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobRms), "<span size=\"8000\"><b>Description:</b> This knob controls the smoothing on the input signal the compressor performs.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobRms),"motion-notify-event",G_CALLBACK(on_inv_comp_rms_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobAttack),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobAttack), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobAttack), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobAttack), INV_KNOB_MARKINGS_5);
	inv_knob_set_human(INV_KNOB (pluginGui->knobAttack)); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobAttack), "s");
	inv_knob_set_min(INV_KNOB (pluginGui->knobAttack), 0.00001);
	inv_knob_set_max(INV_KNOB (pluginGui->knobAttack), 0.750);
	inv_knob_set_value(INV_KNOB (pluginGui->knobAttack), pluginGui->attack);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobAttack), "<span size=\"8000\"><b>Description:</b> This knob sets the attack of the envelope which controls how well the compressor follows rising signals.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobAttack),"motion-notify-event",G_CALLBACK(on_inv_comp_attack_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobRelease),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobRelease), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobRelease), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobRelease), INV_KNOB_MARKINGS_5);
	inv_knob_set_human(INV_KNOB (pluginGui->knobRelease)); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobRelease), "s");
	inv_knob_set_min(INV_KNOB (pluginGui->knobRelease), 0.001);
	inv_knob_set_max(INV_KNOB (pluginGui->knobRelease), 5.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobRelease), pluginGui->release);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobRelease), "<span size=\"8000\"><b>Description:</b> This knob sets the release of the envelope which controls how well the compressor follows falling signals.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobRelease),"motion-notify-event",G_CALLBACK(on_inv_comp_release_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobThreshold),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobThreshold), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobThreshold), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobThreshold), INV_KNOB_MARKINGS_5);
	inv_knob_set_highlight(INV_KNOB (pluginGui->knobThreshold), INV_KNOB_HIGHLIGHT_L);
	inv_knob_set_units(INV_KNOB (pluginGui->knobThreshold), "dB");
	inv_knob_set_min(INV_KNOB (pluginGui->knobThreshold), -36.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobThreshold), 0.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobThreshold), pluginGui->threshold);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobThreshold), "<span size=\"8000\"><b>Description:</b> This knob sets the threshold at which the compressor starts affecting the signal.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobThreshold),"motion-notify-event",G_CALLBACK(on_inv_comp_threshold_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobRatio),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobRatio), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobRatio), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobRatio), INV_KNOB_MARKINGS_5);
	inv_knob_set_highlight(INV_KNOB (pluginGui->knobRatio), INV_KNOB_HIGHLIGHT_L);
	inv_knob_set_units(INV_KNOB (pluginGui->knobRatio), ":1");
	inv_knob_set_min(INV_KNOB (pluginGui->knobRatio), 1.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobRatio), 20.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobRatio), pluginGui->ratio);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobRatio), "<span size=\"8000\"><b>Description:</b> This knob sets the compression ratio for signals that have exceeded the threshold.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobRatio),"motion-notify-event",G_CALLBACK(on_inv_comp_ratio_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobGain),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobGain), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobGain), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobGain), INV_KNOB_MARKINGS_5);
	inv_knob_set_highlight(INV_KNOB (pluginGui->knobGain), INV_KNOB_HIGHLIGHT_L);
	inv_knob_set_units(INV_KNOB (pluginGui->knobGain), "dB");
	inv_knob_set_min(INV_KNOB (pluginGui->knobGain), -6.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobGain), 36.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobGain), pluginGui->gain);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobGain), "<span size=\"8000\"><b>Description:</b> This knob sets the output or markup gain of the compressor.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobGain),"motion-notify-event",G_CALLBACK(on_inv_comp_gain_knob_motion),pluginGui);

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF, "Off");
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON,  0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON,  "Active");
	inv_switch_toggle_set_state( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), "<span size=\"8000\"><b>Description:</b> This switch activates soft-clipping on the output. The soft clipping function outputs a value between -3dB and 0dB for input values between -3dB and +infinity.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleNoClip),"button-release-event",G_CALLBACK(on_inv_comp_noClip_toggle_button_release),pluginGui);

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


static void cleanupICompGui(LV2UI_Handle ui)
{
	return;
}


static void port_eventICompGui(LV2UI_Handle ui, uint32_t port, uint32_t buffer_size, uint32_t format, const void*  buffer)
{
	ICompGui *pluginGui = (ICompGui *)ui;

	float value;

	if(format==0) 
	{
		value=* (float *) buffer;
		switch(port)
		{
			case ICOMP_BYPASS:
				pluginGui->bypass=value;
				if(value <= 0.0) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterIn),      INV_PLUGIN_ACTIVE);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterGR),      INV_PLUGIN_ACTIVE);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterOut),     INV_PLUGIN_ACTIVE);
					inv_display_comp_set_bypass(  INV_DISPLAY_COMP  (pluginGui->display),      INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobRms),      INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobAttack),   INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobRelease),  INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobThreshold),INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobRatio),    INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobGain),     INV_PLUGIN_ACTIVE);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_PLUGIN_ACTIVE);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterIn),      INV_PLUGIN_BYPASS);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterGR),      INV_PLUGIN_BYPASS);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterOut),     INV_PLUGIN_BYPASS);
					inv_display_comp_set_bypass(  INV_DISPLAY_COMP  (pluginGui->display),      INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobRms),      INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobAttack),   INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobRelease),  INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobThreshold),INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobRatio),    INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobGain),     INV_PLUGIN_BYPASS);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_PLUGIN_BYPASS);
				}
				gtk_widget_queue_draw (pluginGui->windowContainer);
				break;
			case ICOMP_RMS:
				pluginGui->rms=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobRms), pluginGui->rms);
				inv_display_comp_set_rms(INV_DISPLAY_COMP (pluginGui->display), pluginGui->rms);
				break;
			case ICOMP_ATTACK:
				pluginGui->attack=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobAttack), pluginGui->attack);
				inv_display_comp_set_attack(INV_DISPLAY_COMP (pluginGui->display), pluginGui->attack);
				break;
			case ICOMP_RELEASE:
				pluginGui->release=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobRelease), pluginGui->release);
				inv_display_comp_set_release(INV_DISPLAY_COMP (pluginGui->display), pluginGui->release);
				break;
			case ICOMP_THRESH:
				pluginGui->threshold=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobThreshold), pluginGui->threshold);
				inv_display_comp_set_threshold(INV_DISPLAY_COMP (pluginGui->display), pluginGui->threshold);
				break;
			case ICOMP_RATIO:
				pluginGui->ratio=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobRatio), pluginGui->ratio);
				inv_display_comp_set_ratio(INV_DISPLAY_COMP (pluginGui->display), pluginGui->ratio);
				break;
			case ICOMP_GAIN:
				pluginGui->gain=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobGain), pluginGui->gain);
				inv_display_comp_set_gain(INV_DISPLAY_COMP (pluginGui->display), pluginGui->gain);
				break;
			case ICOMP_NOCLIP:
				pluginGui->noClip=value;
				if(value <= 0.0) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_OFF);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleNoClip), INV_SWITCH_TOGGLE_ON);
				}
				break;
			case ICOMP_METER_GR:
				inv_meter_set_LdB(INV_METER (pluginGui->meterGR),value);
				break;
			case ICOMP_METER_INL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterIn),value);
				break;
			case ICOMP_METER_INR:
				if(pluginGui->InChannels==2) inv_meter_set_RdB(INV_METER (pluginGui->meterIn),value);
				break;
			case ICOMP_METER_OUTL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterOut),value);
				break;
			case ICOMP_METER_OUTR:
				if(pluginGui->OutChannels==2) inv_meter_set_RdB(INV_METER (pluginGui->meterOut),value);
				break;
			case ICOMP_METER_DRIVE:
				inv_lamp_set_value(INV_LAMP (pluginGui->lampNoClip),value);
				break;
		}
	}
}


static void init()
{
	ICompGuiDescriptor =
	 (LV2UI_Descriptor *)malloc(sizeof(LV2UI_Descriptor));

	ICompGuiDescriptor->URI 		= ICOMP_GUI_URI;
	ICompGuiDescriptor->instantiate 	= instantiateICompGui;
	ICompGuiDescriptor->cleanup		= cleanupICompGui;
	ICompGuiDescriptor->port_event	= port_eventICompGui;
	ICompGuiDescriptor->extension_data 	= NULL;

}


const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
	if (!ICompGuiDescriptor) init();

	switch (index) {
		case 0:
			return ICompGuiDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/

static void 
on_inv_comp_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->bypass=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_BYPASS, 4, 0, &pluginGui->bypass);
	return;
}


static void on_inv_comp_rms_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->rms=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_RMS, 4, 0, &pluginGui->rms);
	return;
}

static void on_inv_comp_attack_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->attack=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_ATTACK, 4, 0, &pluginGui->attack);
	return;
}

static void on_inv_comp_release_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->release=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_RELEASE, 4, 0, &pluginGui->release);
	return;
}

static void on_inv_comp_threshold_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->threshold=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_THRESH, 4, 0, &pluginGui->threshold);
	return;
}

static void on_inv_comp_ratio_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->ratio=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_RATIO, 4, 0, &pluginGui->ratio);
	return;
}

static void on_inv_comp_gain_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->gain=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_GAIN, 4, 0, &pluginGui->gain);
	return;
}

static void on_inv_comp_noClip_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	ICompGui *pluginGui = (ICompGui *) data;

	pluginGui->noClip=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, ICOMP_NOCLIP, 4, 0, &pluginGui->noClip);
	return;
}

