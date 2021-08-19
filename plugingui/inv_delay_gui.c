/* 

    This LV2 extension provides delay gui's

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
#include "widgets/knob.h"
#include "widgets/lamp.h"
#include "widgets/meter-peak.h"
#include "widgets/switch-toggle.h"
#include "../plugin/inv_delay.h"
#include "inv_delay_gui.h"


static LV2UI_Descriptor *IDelayGuiDescriptor = NULL;

typedef struct {
	GtkWidget	*windowContainer;
	GtkWidget	*heading;
	GtkWidget	*toggleBypass;
	GtkWidget	*meterIn;
	GtkWidget	*meterOut;
	GtkWidget	*toggleMode;
	GtkWidget	*toggleMungeMode;
	GtkWidget	*knobCycle;
	GtkWidget	*knobWidth;
	GtkWidget	*lampLFO;
	GtkWidget	*knobMunge;
	GtkWidget	*knobDelay1;
	GtkWidget	*knobFB1;
	GtkWidget	*knobPan1;
	GtkWidget	*knobVol1;
	GtkWidget	*knobDelay2;
	GtkWidget	*knobFB2;
	GtkWidget	*knobPan2;
	GtkWidget	*knobVol2;
	GtkWidget	*spinTempo;
	GtkWidget	*treeviewDelayCalc;

	gint		InChannels;
	gint		OutChannels;
	float 		bypass;
	float 		mode;
	float 		mungemode;
	float 		munge;
	float 		cycle;
	float 		width;
	float 		delay1;
	float		fb1;
	float		pan1;
	float		vol1;
	float 		delay2;
	float		fb2;
	float		pan2;
	float		vol2;
	float		tempo;

	LV2UI_Write_Function 	write_function;
	LV2UI_Controller 	controller;

} IDelayGui;



static LV2UI_Handle 
instantiateIDelayGui(const struct _LV2UI_Descriptor* descriptor, const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
	IDelayGui *pluginGui = (IDelayGui *)malloc(sizeof(IDelayGui));
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
	file = g_strdup_printf("%s/gtk/inv_delay_gui.xml",bundle_path);
	gtk_builder_add_from_file (builder, file, &err);
	free(file);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "delay_window"));

	/* get pointers to some useful widgets from the design */
	pluginGui->windowContainer = GTK_WIDGET (gtk_builder_get_object (builder, "delay_container"));
	pluginGui->heading = GTK_WIDGET (gtk_builder_get_object (builder, "label_heading"));
	pluginGui->spinTempo = GTK_WIDGET (gtk_builder_get_object (builder, "spinbutton_tempo"));
	pluginGui->treeviewDelayCalc = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_delaycalc"));

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

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_mode_toggle"));
	pluginGui->toggleMode = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleMode);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_mungemode_toggle"));
	pluginGui->toggleMungeMode = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleMungeMode);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_munge_knob"));
	pluginGui->knobMunge = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobMunge);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_cycle_knob"));
	pluginGui->knobCycle = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobCycle);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_width_knob"));
	pluginGui->knobWidth = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobWidth);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_cycle_lamp"));
	pluginGui->lampLFO = inv_lamp_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->lampLFO);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_delay1_knob"));
	pluginGui->knobDelay1 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobDelay1);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_fb1_knob"));
	pluginGui->knobFB1 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobFB1);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_pan1_knob"));
	pluginGui->knobPan1 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobPan1);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_vol1_knob"));
	pluginGui->knobVol1 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobVol1);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_delay2_knob"));
	pluginGui->knobDelay2 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobDelay2);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_fb2_knob"));
	pluginGui->knobFB2 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobFB2);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_pan2_knob"));
	pluginGui->knobPan2 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobPan2);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_vol2_knob"));
	pluginGui->knobVol2 = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobVol2);

	/* customise for the plugin */
	if(!strcmp(plugin_uri,IDELAY_MONO_URI)) 
	{
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Delay Munge (mono in)</b>");
	}
	if(!strcmp(plugin_uri,IDELAY_SUM_URI)) 
	{
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Delay Munge (sum L+R in)</b>");
	}

	pluginGui->InChannels	= 1;
	pluginGui->OutChannels	= 2;
	pluginGui->bypass	= 0.0;
	pluginGui->mode		= 0.0;
	pluginGui->mungemode	= 0.0;
	pluginGui->munge	= 50.0;
	pluginGui->cycle	= 20.0;
	pluginGui->width	= 0.0;
	pluginGui->delay1	= 300.0;
	pluginGui->fb1		= 50.0;
	pluginGui->pan1		= -0.7;
	pluginGui->vol1		= 100.0;
	pluginGui->delay2	= 200.0;
	pluginGui->fb2		= 50.0;
	pluginGui->pan2		= 0.7;
	pluginGui->vol2		= 100.0;
	pluginGui->tempo	= 120.0;

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(   INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, "Active");
	inv_switch_toggle_set_value(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0, 0.0, 0.0);
	inv_switch_toggle_set_text(   INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  "Bypassed");
	inv_switch_toggle_set_state(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), "<span size=\"8000\"><b>Description:</b> This switch bypasses the plugin.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleBypass),"button-release-event",G_CALLBACK(on_inv_delay_bypass_toggle_button_release),pluginGui);

	inv_meter_set_bypass(INV_METER (pluginGui->meterIn),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterIn), INV_METER_DRAW_MODE_TOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterIn), pluginGui->InChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterIn),-90);

	inv_meter_set_bypass(INV_METER (pluginGui->meterOut),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterOut), INV_METER_DRAW_MODE_TOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterOut), pluginGui->OutChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterOut),-90);
	inv_meter_set_RdB(INV_METER (pluginGui->meterOut),-90);

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value(  INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour (INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(   INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_OFF, "Discrete");
	inv_switch_toggle_set_value(  INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour( INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_ON,  0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(   INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_ON,  "Ping-Pong");
	inv_switch_toggle_set_state(  INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleMode), "<span size=\"8000\"><b>Description:</b> This switch changes the mode of the delay between discrete channels and ping-ping.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleMode),"button-release-event",G_CALLBACK(on_inv_delay_mode_toggle_button_release),pluginGui);

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value(  INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour (INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(   INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_OFF, "Type 1");
	inv_switch_toggle_set_value(  INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour( INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_ON,  0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(   INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_ON,  "Type 2");
	inv_switch_toggle_set_state(  INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), "<span size=\"8000\"><b>Description:</b> This switch changes the munge type used on the signal in the feedback loop.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleMungeMode),"button-release-event",G_CALLBACK(on_inv_delay_mungemode_toggle_button_release),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobMunge), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobMunge), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobMunge), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobMunge), INV_KNOB_MARKINGS_5); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobMunge), "%");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobMunge), 0.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobMunge), 100.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobMunge), pluginGui->munge);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobMunge), "<span size=\"8000\"><b>Description:</b> This knob sets the amount of munge.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobMunge),"motion-notify-event",G_CALLBACK(on_inv_delay_munge_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobCycle), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobCycle), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobCycle), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobCycle), INV_KNOB_MARKINGS_3); 
	inv_knob_set_human(   INV_KNOB (pluginGui->knobCycle)); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobCycle), "s");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobCycle), 2.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobCycle), 200.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobCycle), pluginGui->cycle);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobCycle), "<span size=\"8000\"><b>Description:</b> This knob sets the period of the LFO.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobCycle), "motion-notify-event",G_CALLBACK(on_inv_delay_cycle_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobWidth), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobWidth), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobWidth), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobWidth), INV_KNOB_MARKINGS_5); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobWidth), "%");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobWidth), 0.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobWidth), 100.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobWidth), pluginGui->width);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobWidth), "<span size=\"8000\"><b>Description:</b> This knob sets the width of the LFO.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobWidth),"motion-notify-event",G_CALLBACK(on_inv_delay_width_knob_motion),pluginGui);

	inv_lamp_set_value(INV_LAMP (pluginGui->lampLFO),0.0);
	inv_lamp_set_scale(INV_LAMP (pluginGui->lampLFO),1.0);
	inv_lamp_set_tooltip(INV_LAMP (pluginGui->lampLFO), "<span size=\"8000\">This shows the LFO cycle.</span>");


	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobDelay1), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobDelay1), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobDelay1), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobDelay1), INV_KNOB_MARKINGS_3); 
	inv_knob_set_human(   INV_KNOB (pluginGui->knobDelay1));
	inv_knob_set_units(   INV_KNOB (pluginGui->knobDelay1), "s");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobDelay1), 0.02);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobDelay1), 2.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobDelay1), pluginGui->delay1);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobDelay1), "<span size=\"8000\"><b>Description:</b> This knob sets the delay time for the first delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobDelay1),"motion-notify-event",G_CALLBACK(on_inv_delay_delay1_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobFB1), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobFB1), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobFB1), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobFB1), INV_KNOB_MARKINGS_5); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobFB1), "%");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobFB1), 0.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobFB1), 133.3333);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobFB1), pluginGui->fb1);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobFB1), "<span size=\"8000\"><b>Description:</b> This knob sets the amount of feedback for the first delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobFB1),"motion-notify-event",G_CALLBACK(on_inv_delay_fb1_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobPan1), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobPan1), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobPan1), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobPan1), INV_KNOB_MARKINGS_PAN); 
	inv_knob_set_min(     INV_KNOB (pluginGui->knobPan1), -1.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobPan1), 1.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobPan1), pluginGui->pan1);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobPan1), "<span size=\"8000\"><b>Description:</b> This knob sets the position within the output mix for the first delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobPan1),"motion-notify-event",G_CALLBACK(on_inv_delay_pan1_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobVol1), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobVol1), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobVol1), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobVol1), INV_KNOB_MARKINGS_4); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobVol1), "%");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobVol1), 0.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobVol1), 100.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobVol1), pluginGui->vol1);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobVol1), "<span size=\"8000\"><b>Description:</b> This knob sets the volume in the output mix for the first delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobVol1),"motion-notify-event",G_CALLBACK(on_inv_delay_vol1_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobDelay2), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobDelay2), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobDelay2), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobDelay2), INV_KNOB_MARKINGS_3); 
	inv_knob_set_human(   INV_KNOB (pluginGui->knobDelay2));
	inv_knob_set_units(   INV_KNOB (pluginGui->knobDelay2), "s");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobDelay2), 0.02);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobDelay2), 2.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobDelay2), pluginGui->delay2);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobDelay2), "<span size=\"8000\"><b>Description:</b> This knob sets the delay time for the second delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobDelay2),"motion-notify-event",G_CALLBACK(on_inv_delay_delay2_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobFB2), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobFB2), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobFB2), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobFB2), INV_KNOB_MARKINGS_5); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobFB2), "%");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobFB2), 0.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobFB2), 133.3333);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobFB2), pluginGui->fb2);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobFB2), "<span size=\"8000\"><b>Description:</b> This knob sets the amount of feedback for the second delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobFB2),"motion-notify-event",G_CALLBACK(on_inv_delay_fb2_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobPan2), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobPan2), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobPan2), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobPan2), INV_KNOB_MARKINGS_PAN); 
	inv_knob_set_min(     INV_KNOB (pluginGui->knobPan2), -1.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobPan2), 1.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobPan2), pluginGui->pan2);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobPan2), "<span size=\"8000\"><b>Description:</b> This knob sets the position within the output mix for the second delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobPan2),"motion-notify-event",G_CALLBACK(on_inv_delay_pan2_knob_motion),pluginGui);

	inv_knob_set_bypass(  INV_KNOB (pluginGui->knobVol2), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(    INV_KNOB (pluginGui->knobVol2), INV_KNOB_SIZE_SMALL);
	inv_knob_set_curve(   INV_KNOB (pluginGui->knobVol2), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobVol2), INV_KNOB_MARKINGS_4); 
	inv_knob_set_units(   INV_KNOB (pluginGui->knobVol2), "%");
	inv_knob_set_min(     INV_KNOB (pluginGui->knobVol2), 0.0);
	inv_knob_set_max(     INV_KNOB (pluginGui->knobVol2), 100.0);
	inv_knob_set_value(   INV_KNOB (pluginGui->knobVol2), pluginGui->vol2);
	inv_knob_set_tooltip( INV_KNOB (pluginGui->knobVol2), "<span size=\"8000\"><b>Description:</b> This knob sets the volume in the output mix for the second delay.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobVol2),"motion-notify-event",G_CALLBACK(on_inv_delay_vol2_knob_motion),pluginGui);


	gtk_spin_button_set_value(GTK_SPIN_BUTTON (pluginGui->spinTempo),pluginGui->tempo);
	g_signal_connect_after(G_OBJECT(pluginGui->spinTempo),"value-changed",G_CALLBACK(on_inv_delay_tempo_value_changed),pluginGui);

	inv_delay_init_delaycalc(pluginGui->treeviewDelayCalc);
	inv_delay_update_delaycalc(pluginGui->treeviewDelayCalc, pluginGui->tempo);
	g_signal_connect_after(G_OBJECT(pluginGui->treeviewDelayCalc),"button-release-event",G_CALLBACK(on_inv_delay_calc_button_release),pluginGui);

	/* strip the parent window from the design so the host can attach its own */
	gtk_widget_ref(pluginGui->windowContainer);
	gtk_container_remove(GTK_CONTAINER(window), pluginGui->windowContainer);

	*widget = (LV2UI_Widget) pluginGui->windowContainer;

	g_object_unref (G_OBJECT (builder));
             
	/* return the instance */
	return pluginGui;
}


static void 
cleanupIDelayGui(LV2UI_Handle ui)
{
	return;
}


static void 
port_eventIDelayGui(LV2UI_Handle ui, uint32_t port, uint32_t buffer_size, uint32_t format, const void*  buffer)
{
	IDelayGui *pluginGui = (IDelayGui *)ui;

	float value;

	if(format==0) 
	{
		value=* (float *) buffer;
		switch(port)
		{
			case IDELAY_BYPASS:
				pluginGui->bypass=value;
				if(value <= 0.5) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass),      INV_SWITCH_TOGGLE_OFF);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterIn),         INV_PLUGIN_ACTIVE);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterOut),        INV_PLUGIN_ACTIVE);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleMode),      INV_PLUGIN_ACTIVE);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobMunge),       INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobCycle),       INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobWidth),       INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobDelay1),      INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobFB1),         INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobPan1),        INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobVol1),        INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobDelay2),      INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobFB2),         INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobPan2),        INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobVol2),        INV_PLUGIN_ACTIVE);
					gtk_widget_set_sensitive(     GTK_WIDGET        (pluginGui->treeviewDelayCalc),TRUE);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass),      INV_SWITCH_TOGGLE_ON);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterIn),         INV_PLUGIN_BYPASS);
					inv_meter_set_bypass(         INV_METER         (pluginGui->meterOut),        INV_PLUGIN_BYPASS);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleMode),      INV_PLUGIN_BYPASS);
					inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobMunge),       INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobCycle),       INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobWidth),       INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobDelay1),      INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobFB1),         INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobPan1),        INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobVol1),        INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobDelay2),      INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobFB2),         INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobPan2),        INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(          INV_KNOB          (pluginGui->knobVol2),        INV_PLUGIN_BYPASS);
					gtk_widget_set_sensitive(     GTK_WIDGET        (pluginGui->treeviewDelayCalc),FALSE);
				}
				gtk_widget_queue_draw (pluginGui->windowContainer);
				break;
			case IDELAY_MODE:
				pluginGui->mode=value;
				if(value <= 0.5) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_OFF);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleMode), INV_SWITCH_TOGGLE_ON);
				}
				break;
			case IDELAY_MUNGEMODE:
				pluginGui->mungemode=value;
				if(value <= 0.5) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_OFF);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleMungeMode), INV_SWITCH_TOGGLE_ON);
				}
				break;
			case IDELAY_MUNGE:
				pluginGui->munge=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobMunge), pluginGui->munge);
				break;
			case IDELAY_LFO_CYCLE:
				pluginGui->cycle=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobCycle), pluginGui->cycle);
				break;
			case IDELAY_LFO_WIDTH:
				pluginGui->width=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobWidth), pluginGui->width);
				break;
			case IDELAY_LAMP_LFO:
				inv_lamp_set_value(INV_LAMP (pluginGui->lampLFO),value);
				break;
			case IDELAY_1_DELAY:
				pluginGui->delay1=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobDelay1), pluginGui->delay1);
				break;
			case IDELAY_1_FB:
				pluginGui->fb1=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobFB1), pluginGui->fb1);
				break;
			case IDELAY_1_PAN:
				pluginGui->pan1=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobPan1), pluginGui->pan1);
				break;
			case IDELAY_1_VOL:
				pluginGui->vol1=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobVol1), pluginGui->vol1);
				break;
			case IDELAY_2_DELAY:
				pluginGui->delay2=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobDelay2), pluginGui->delay2);
				break;
			case IDELAY_2_FB:
				pluginGui->fb2=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobFB2), pluginGui->fb2);
				break;
			case IDELAY_2_PAN:
				pluginGui->pan2=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobPan2), pluginGui->pan2);
				break;
			case IDELAY_2_VOL:
				pluginGui->vol2=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobVol2), pluginGui->vol2);
				break;
			case IDELAY_METER_INL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterIn),value);
				break;
			case IDELAY_METER_OUTL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterOut),value);
				break;
			case IDELAY_METER_OUTR:
				inv_meter_set_RdB(INV_METER (pluginGui->meterOut),value);
				break;
		}
	}
}


static void 
init()
{
	IDelayGuiDescriptor =
	 (LV2UI_Descriptor *)malloc(sizeof(LV2UI_Descriptor));

	IDelayGuiDescriptor->URI 		= IDELAY_GUI_URI;
	IDelayGuiDescriptor->instantiate 	= instantiateIDelayGui;
	IDelayGuiDescriptor->cleanup		= cleanupIDelayGui;
	IDelayGuiDescriptor->port_event	= port_eventIDelayGui;
	IDelayGuiDescriptor->extension_data 	= NULL;

}


const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
	if (!IDelayGuiDescriptor) init();

	switch (index) {
		case 0:
			return IDelayGuiDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/
static  gint
inv_delay_get_col_number_from_tree_view_column (GtkTreeViewColumn *col)
{
	GList *cols;
	gint   num;

	g_return_val_if_fail ( col != NULL, -1 );
	g_return_val_if_fail ( col->tree_view != NULL, -1 );
	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(col->tree_view));
	num = g_list_index(cols, (gpointer) col);
	g_list_free(cols);

	return num;
}


static void
inv_delay_cell_data_function (  GtkTreeViewColumn *col,
				GtkCellRenderer   *renderer,
				GtkTreeModel      *model,
				GtkTreeIter       *iter,
				gint              pos) {
	gfloat  value;
	gchar   buf[20];

	gtk_tree_model_get(model, iter, pos, &value, -1);
	if(value >= 1.0) {
		g_snprintf(buf, sizeof(buf), "%.2fs ", floor(value*100)/100);
	} else if(value >= 0.1) {
		g_snprintf(buf, sizeof(buf), "%.0fms", floor(value*1000));
	} else if(value >= 0.01) {
		g_snprintf(buf, sizeof(buf), "%.1fms", floor(value*10000)/10);
	} else if(value >= 0.001) {
		g_snprintf(buf, sizeof(buf), "%.2fms", floor(value*100000)/100);
	} else {
		g_snprintf(buf, sizeof(buf), "%.3fms", floor(value*1000000)/1000);
	}
	g_object_set(renderer, "text", buf, NULL);
}

static void
inv_delay_length_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_LENGTH);
}
static void
inv_delay_dotted_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_DOTTED);
}
static void
inv_delay_tuplet32_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_TUPLET32);
}
static void
inv_delay_tuplet54_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_TUPLET54);
}
static void
inv_delay_tuplet74_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_TUPLET74);
}
static void
inv_delay_tuplet94_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_TUPLET94);
}
static void
inv_delay_tuplet114_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_delay_cell_data_function (col,renderer,model,iter,COLUMN_TUPLET114);
}

static void 
inv_delay_init_delaycalc(GtkWidget *tree)
{

	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;


	// define columns 
	/* NOTE */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Note");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",0.5,NULL);
	g_object_set (renderer,"weight",800,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", COLUMN_NOTE);

	/* LENGTH */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " Length");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_length_cell_data_function, NULL, NULL);

	/* DOTTED */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " Dotted");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_dotted_cell_data_function, NULL, NULL);

	/* 3:2 Tuplet */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " 3:2 Tuplet");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_tuplet32_cell_data_function, NULL, NULL);

	/* 5:4 Tuplet */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " 5:4 Tuplet");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_tuplet54_cell_data_function, NULL, NULL);

	/* 7:4 Tuplet */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " 7:4 Tuplet");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_tuplet74_cell_data_function, NULL, NULL);

	/* 9:4 Tuplet */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " 9:4 Tuplet");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_tuplet94_cell_data_function, NULL, NULL);

	/* 11:4 Tuplet */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, " 11:4 Tuplet");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_delay_tuplet114_cell_data_function, NULL, NULL);

	/* tooltips */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "tooltips");
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	gtk_tree_view_column_set_visible (col,FALSE);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tree),COLUMN_TOOLTIP);
	return;
}

static void 
inv_delay_update_delaycalc(GtkWidget *tree, float tempo)
{

	GtkListStore 	*liststore;
	GtkTreeIter   	iter;
	gfloat 		length;
	gint		i;
	char		notelabel[8];
	
	length=240.0/tempo; //assumes beat length in time signature is a crotchet

	// create empty store
	liststore = gtk_list_store_new( NUM_COLS, 
					G_TYPE_STRING, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_STRING);
	for(i=0;i<7;i++) {
		gtk_list_store_append(liststore, &iter);
		if(i==0) {
			sprintf(notelabel,"1");
		} else {
			sprintf(notelabel,"1/%i",(int)pow(2,i));
		}
		gtk_list_store_set (liststore, &iter, 
			COLUMN_NOTE,      notelabel, 
			COLUMN_LENGTH,    length,
			COLUMN_DOTTED,    length*1.5,
			COLUMN_TUPLET32,  length*2/3,
			COLUMN_TUPLET54,  length*4/5,
			COLUMN_TUPLET74,  length*4/7,
			COLUMN_TUPLET94,  length*4/9,
			COLUMN_TUPLET114, length*4/11,
			COLUMN_TOOLTIP,   "<span size=\"8000\"><b>Description:</b> Calculated delay times for the current tempo across different length notes.\n<b>Usage:</b> Left-Click on a cell to assign the value to Delay 1, Right-Click to assign the value to Delay 2.</span>",
			-1);
		length=length/2;
	}

	// add the model to the treeview
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(liststore));

	return;
}

/*****************************************************************************/

static void 
on_inv_delay_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->bypass=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_BYPASS, 4, 0, &pluginGui->bypass);
	return;
}

static void 
on_inv_delay_mode_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->mode=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_MODE, 4, 0, &pluginGui->mode);
	return;
}

static void 
on_inv_delay_mungemode_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->mungemode=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_MUNGEMODE, 4, 0, &pluginGui->mungemode);
	return;
}

static void 
on_inv_delay_munge_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->munge=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_MUNGE, 4, 0, &pluginGui->munge);
	return;
}

static void 
on_inv_delay_cycle_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->cycle=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_LFO_CYCLE, 4, 0, &pluginGui->cycle);
	return;
}

static void 
on_inv_delay_width_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->width=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_LFO_WIDTH, 4, 0, &pluginGui->width);
	return;
}

static void 
on_inv_delay_delay1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->delay1=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_1_DELAY, 4, 0, &pluginGui->delay1);
	return;
}

static void 
on_inv_delay_fb1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->fb1=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_1_FB, 4, 0, &pluginGui->fb1);
	return;
}

static void 
on_inv_delay_pan1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->pan1=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_1_PAN, 4, 0, &pluginGui->pan1);
	return;
}

static void 
on_inv_delay_vol1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->vol1=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_1_VOL, 4, 0, &pluginGui->vol1);
	return;
}

static void 
on_inv_delay_delay2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->delay2=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_2_DELAY, 4, 0, &pluginGui->delay2);
	return;
}

static void 
on_inv_delay_fb2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->fb2=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_2_FB, 4, 0, &pluginGui->fb2);
	return;
}

static void 
on_inv_delay_pan2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->pan2=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_2_PAN, 4, 0, &pluginGui->pan2);
	return;
}

static void 
on_inv_delay_vol2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->vol2=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IDELAY_2_VOL, 4, 0, &pluginGui->vol2);
	return;
}

static void 
on_inv_delay_tempo_value_changed(GtkWidget *widget, gpointer data)
{
	IDelayGui *pluginGui = (IDelayGui *) data;

	pluginGui->tempo=gtk_spin_button_get_value(GTK_SPIN_BUTTON (pluginGui->spinTempo));
	inv_delay_update_delaycalc(pluginGui->treeviewDelayCalc, pluginGui->tempo);
	return;
}
static void 
on_inv_delay_calc_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkTreePath 		*path;
	GtkTreeViewColumn 	*column;
	GtkTreeModel 		*model;
    	GtkTreeIter   		iter;

	gfloat			value;
	gint			col;

	IDelayGui *pluginGui = (IDelayGui *) data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW (pluginGui->treeviewDelayCalc));

	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW (pluginGui->treeviewDelayCalc), 
					(gint) ((GdkEventButton*)event)->x,
					(gint) ((GdkEventButton*)event)->y, 
					&path, &column, NULL, NULL);

	col=inv_delay_get_col_number_from_tree_view_column(column);

	if (col > 0 && gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, col, &value, -1);
		if(value >= 0.02 && value <= 2.0) {
			if(((GdkEventButton*)event)->button ==1) {
				pluginGui->delay1=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobDelay1), pluginGui->delay1);
				(*pluginGui->write_function)(pluginGui->controller, IDELAY_1_DELAY, 4, 0, &pluginGui->delay1);
			}
			if(((GdkEventButton*)event)->button ==3) {
				pluginGui->delay2=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobDelay2), pluginGui->delay2);
				(*pluginGui->write_function)(pluginGui->controller, IDELAY_2_DELAY, 4, 0, &pluginGui->delay2);
			}
		}
	}

	gtk_tree_path_free(path);
	return;
}
