/* 

    This LV2 extension provides er reverb gui's

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
#include "widgets/display-ErReverb.h"
#include "widgets/knob.h"
#include "widgets/meter-peak.h"
#include "widgets/switch-toggle.h"
#include "../plugin/inv_erreverb.h"
#include "inv_erreverb_gui.h"


static LV2UI_Descriptor *IErReverbGuiDescriptor = NULL;

typedef struct {
	GtkWidget	*windowContainer;
	GtkWidget	*heading;
	GtkWidget	*toggleBypass;
	GtkWidget	*meterIn;
	GtkWidget	*meterOut;
	GtkWidget	*display;
	GtkWidget	*knobLength;
	GtkWidget	*knobWidth;
	GtkWidget	*knobHeight;
	GtkWidget	*knobHPF;
	GtkWidget	*knobWarmth;
	GtkWidget	*knobDiffusion;

	gint		InChannels;
	gint		OutChannels;
	float 		bypass;
	float		length;
	float		width;
	float		height;
	float		sourceLR;
	float		sourceFB;
	float		destLR;
	float		destFB;
	float		hpf;
	float		warmth;
	float		diffusion;

	LV2UI_Write_Function 	write_function;
	LV2UI_Controller 	controller;

} IErReverbGui;



static LV2UI_Handle 
instantiateIErReverbGui(const struct LV2UI_Descriptor* descriptor, const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
	IErReverbGui *pluginGui = (IErReverbGui *)malloc(sizeof(IErReverbGui));
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
	file = g_strdup_printf("%s/gtk/inv_erreverb_gui.xml",bundle_path);
	gtk_builder_add_from_file (builder, file, &err);
	free(file);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "erreverb_window"));

	/* get pointers to some useful widgets from the design */
	pluginGui->windowContainer = GTK_WIDGET (gtk_builder_get_object (builder, "erreverb_container"));
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

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_erreverb_display"));
	pluginGui->display = inv_display_err_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->display);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_length_knob"));
	pluginGui->knobLength = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobLength);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_width_knob"));
	pluginGui->knobWidth = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobWidth);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_height_knob"));
	pluginGui->knobHeight = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobHeight);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_hpf_knob"));
	pluginGui->knobHPF = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobHPF);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_warmth_knob"));
	pluginGui->knobWarmth = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobWarmth);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_diffusion_knob"));
	pluginGui->knobDiffusion = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobDiffusion);


	/* customise for the plugin */
	if(!strcmp(plugin_uri,IERR_MONO_URI)) 
	{
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Early Reflection Reverb (mono in)</b>");
	}
	if(!strcmp(plugin_uri,IERR_SUM_URI)) 
	{
		gtk_label_set_markup (GTK_LABEL (pluginGui->heading), "<b>Early Reflection Reverb (sum L+R in)</b>");
	}

	pluginGui->InChannels	= 1;
	pluginGui->OutChannels	= 2;
	pluginGui->bypass	= 0.0;
	pluginGui->length	= 25.0;
	pluginGui->width	= 30.0;
	pluginGui->height	= 10.0;
	pluginGui->sourceLR	=-0.01;
	pluginGui->sourceFB	= 0.8;
	pluginGui->destLR	= 0.01;
	pluginGui->destFB	= 0.2;
	pluginGui->hpf		= 1000.0;
	pluginGui->warmth	= 50.0;
	pluginGui->diffusion	= 50.0;

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, "Active");
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0, 0.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  "Bypassed");
	inv_switch_toggle_set_state( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), "<span size=\"8000\"><b>Description:</b> This switch bypasses the plugin.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleBypass),"button-release-event",G_CALLBACK(on_inv_erreverb_bypass_toggle_button_release),pluginGui);

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

	inv_display_err_set_bypass(INV_DISPLAY_ERR (pluginGui->display), INV_PLUGIN_ACTIVE);
	inv_display_err_set_room(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_ROOM_LENGTH, pluginGui->length);
	inv_display_err_set_room(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_ROOM_WIDTH,  pluginGui->width);
	inv_display_err_set_room(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_ROOM_HEIGHT, pluginGui->height);
	inv_display_err_set_source(INV_DISPLAY_ERR (pluginGui->display), INV_DISPLAY_ERR_LR, pluginGui->sourceLR);
	inv_display_err_set_source(INV_DISPLAY_ERR (pluginGui->display), INV_DISPLAY_ERR_FB, pluginGui->sourceFB);
	inv_display_err_set_dest(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_LR, pluginGui->destLR);
	inv_display_err_set_dest(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_FB, pluginGui->destFB);
	inv_display_err_set_diffusion(INV_DISPLAY_ERR (pluginGui->display), pluginGui->diffusion);
	g_signal_connect_after(G_OBJECT(pluginGui->display),"motion-notify-event",G_CALLBACK(on_inv_erreverb_display_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobLength),   INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobLength),     INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobLength),    INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobLength), INV_KNOB_MARKINGS_4); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobLength),    "m");
	inv_knob_set_min(INV_KNOB (pluginGui->knobLength),      3.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobLength),      100.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobLength),    pluginGui->length);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobLength), "<span size=\"8000\"><b>Description:</b> This knob sets the length of the virtual room.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobLength),"motion-notify-event",G_CALLBACK(on_inv_erreverb_length_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobWidth), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobWidth), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobWidth), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobWidth), INV_KNOB_MARKINGS_4); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobWidth), "m");
	inv_knob_set_min(INV_KNOB (pluginGui->knobWidth), 3.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobWidth), 100.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobWidth), pluginGui->length);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobWidth), "<span size=\"8000\"><b>Description:</b> This knob sets the width of the virtual room.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobWidth),"motion-notify-event",G_CALLBACK(on_inv_erreverb_width_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobHeight), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobHeight), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobHeight), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobHeight), INV_KNOB_MARKINGS_4); 
	inv_knob_set_units(INV_KNOB (pluginGui->knobHeight), "m");
	inv_knob_set_min(INV_KNOB (pluginGui->knobHeight), 3.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobHeight), 30.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobHeight), pluginGui->length);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobHeight), "<span size=\"8000\"><b>Description:</b> This knob sets the height of the virtual room.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobHeight),"motion-notify-event",G_CALLBACK(on_inv_erreverb_height_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobHPF), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobHPF), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobHPF), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobHPF), INV_KNOB_MARKINGS_3);
	inv_knob_set_human(INV_KNOB (pluginGui->knobHPF));
	inv_knob_set_units(INV_KNOB (pluginGui->knobHPF), "Hz");
	inv_knob_set_min(INV_KNOB (pluginGui->knobHPF), 20.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobHPF), 2000.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobHPF), pluginGui->hpf);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobHPF), "<span size=\"8000\"><b>Description:</b> This knob rolls off bottom end as it tends to make reverbs sound muddy.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobHPF),"motion-notify-event",G_CALLBACK(on_inv_erreverb_hpf_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobWarmth), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobWarmth), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobWarmth), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobWarmth), INV_KNOB_MARKINGS_5);
	inv_knob_set_units(INV_KNOB (pluginGui->knobWarmth), "%");
	inv_knob_set_min(INV_KNOB (pluginGui->knobWarmth), 0.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobWarmth), 100.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobWarmth), pluginGui->warmth);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobWarmth), "<span size=\"8000\"><b>Description:</b> This knob sets the high frequency loss of reflections within the room. Low values have little loss (like a tiled room). High values have a lot of loss.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobWarmth),"motion-notify-event",G_CALLBACK(on_inv_erreverb_warmth_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobDiffusion), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobDiffusion), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobDiffusion), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobDiffusion), INV_KNOB_MARKINGS_5);
	inv_knob_set_units(INV_KNOB (pluginGui->knobDiffusion), "%");
	inv_knob_set_min(INV_KNOB (pluginGui->knobDiffusion), 0.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobDiffusion), 100.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobDiffusion), pluginGui->diffusion);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobDiffusion), "<span size=\"8000\"><b>Description:</b> This knob sets the scattering of reflections which simulates rough or uneven surfaces within the virtual room.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobDiffusion),"motion-notify-event",G_CALLBACK(on_inv_erreverb_diffusion_knob_motion),pluginGui);


	/* strip the parent window from the design so the host can attach its own */
	gtk_widget_ref(pluginGui->windowContainer);
	gtk_container_remove(GTK_CONTAINER(window), pluginGui->windowContainer);

	*widget = (LV2UI_Widget) pluginGui->windowContainer;

	g_object_unref (G_OBJECT (builder));
             
	/* return the instance */
	return pluginGui;
}


static void 
cleanupIErReverbGui(LV2UI_Handle ui)
{
	return;
}


static void 
port_eventIErReverbGui(LV2UI_Handle ui, uint32_t port, uint32_t buffer_size, uint32_t format, const void*  buffer)
{
	IErReverbGui *pluginGui = (IErReverbGui *)ui;

	float value;

	if(format==0) 
	{
		value=* (float *) buffer;
		switch(port)
		{
			case IERR_BYPASS:
				pluginGui->bypass=value;
				if(value <= 0.0) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
					inv_meter_set_bypass(       INV_METER       (pluginGui->meterIn),       INV_PLUGIN_ACTIVE);
					inv_meter_set_bypass(       INV_METER       (pluginGui->meterOut),      INV_PLUGIN_ACTIVE);
					inv_display_err_set_bypass( INV_DISPLAY_ERR (pluginGui->display),       INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobLength),    INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobWidth),     INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobHeight),    INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobHPF),       INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobWarmth),    INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobDiffusion), INV_PLUGIN_ACTIVE);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON);
					inv_meter_set_bypass(       INV_METER       (pluginGui->meterIn),       INV_PLUGIN_BYPASS);
					inv_meter_set_bypass(       INV_METER       (pluginGui->meterOut),      INV_PLUGIN_BYPASS);
					inv_display_err_set_bypass( INV_DISPLAY_ERR (pluginGui->display),       INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobLength),    INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobWidth),     INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobHeight),    INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobHPF),       INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobWarmth),    INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB        (pluginGui->knobDiffusion), INV_PLUGIN_BYPASS);
				}
				gtk_widget_queue_draw (pluginGui->windowContainer);
				break;
			case IERR_ROOMLENGTH:
				pluginGui->length=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobLength), pluginGui->length);
				inv_display_err_set_room(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_ROOM_LENGTH, pluginGui->length);
				gtk_widget_queue_draw (pluginGui->display);
				break;
			case IERR_ROOMWIDTH:
				pluginGui->width=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobWidth), pluginGui->width);
				inv_display_err_set_room(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_ROOM_WIDTH,  pluginGui->width);
				gtk_widget_queue_draw (pluginGui->display);
				break;
			case IERR_ROOMHEIGHT:
				pluginGui->height=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobHeight), pluginGui->height);
				inv_display_err_set_room(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_ROOM_HEIGHT, pluginGui->height);
				gtk_widget_queue_draw (pluginGui->display);
				break;
			case IERR_SOURCELR:
				pluginGui->sourceLR=value;
				inv_display_err_set_source(INV_DISPLAY_ERR (pluginGui->display), INV_DISPLAY_ERR_LR, pluginGui->sourceLR);
				break;
			case IERR_SOURCEFB:
				pluginGui->sourceFB=value;
				inv_display_err_set_source(INV_DISPLAY_ERR (pluginGui->display), INV_DISPLAY_ERR_FB, pluginGui->sourceFB);
				break;
			case IERR_DESTLR:
				pluginGui->destLR=value;
				inv_display_err_set_dest(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_LR, pluginGui->destLR);
				break;
			case IERR_DESTFB:
				pluginGui->destFB=value;
				inv_display_err_set_dest(INV_DISPLAY_ERR (pluginGui->display),   INV_DISPLAY_ERR_FB, pluginGui->destFB);
				break;
			case IERR_HPF:
				pluginGui->hpf=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobHPF), pluginGui->hpf);
				break;
			case IERR_WARMTH:
				pluginGui->warmth=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobWarmth), pluginGui->warmth);
				break;
			case IERR_DIFFUSION:
				pluginGui->diffusion=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobDiffusion), pluginGui->diffusion);
				inv_display_err_set_diffusion(INV_DISPLAY_ERR (pluginGui->display), pluginGui->diffusion);
				gtk_widget_queue_draw (pluginGui->display);
				break;
			case IERR_METER_IN:
				inv_meter_set_LdB(INV_METER (pluginGui->meterIn),value);
				break;
			case IERR_METER_OUTL:
				inv_meter_set_LdB(INV_METER (pluginGui->meterOut),value);
				break;
			case IERR_METER_OUTR:
				inv_meter_set_RdB(INV_METER (pluginGui->meterOut),value);
				break;
		}
	}
}


static void 
init()
{
	IErReverbGuiDescriptor =
	 (LV2UI_Descriptor *)malloc(sizeof(LV2UI_Descriptor));

	IErReverbGuiDescriptor->URI 		= IERR_GUI_URI;
	IErReverbGuiDescriptor->instantiate 	= instantiateIErReverbGui;
	IErReverbGuiDescriptor->cleanup		= cleanupIErReverbGui;
	IErReverbGuiDescriptor->port_event	= port_eventIErReverbGui;
	IErReverbGuiDescriptor->extension_data 	= NULL;

}


const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
	if (!IErReverbGuiDescriptor) init();

	switch (index) {
		case 0:
			return IErReverbGuiDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/

static void 
on_inv_erreverb_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->bypass=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_BYPASS, 4, 0, &pluginGui->bypass);
	return;
}

static void 
on_inv_erreverb_length_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->length=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_ROOMLENGTH, 4, 0, &pluginGui->length);
	return;
}

static void 
on_inv_erreverb_width_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->width=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_ROOMWIDTH, 4, 0, &pluginGui->width);
	return;
}

static void 
on_inv_erreverb_height_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->height=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_ROOMHEIGHT, 4, 0, &pluginGui->height);
	return;
}

static void 
on_inv_erreverb_hpf_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->hpf=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_HPF, 4, 0, &pluginGui->hpf);
	return;
}

static void
on_inv_erreverb_warmth_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->warmth=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_WARMTH, 4, 0, &pluginGui->warmth);
	return;
}

static void 
on_inv_erreverb_diffusion_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	pluginGui->diffusion=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, IERR_DIFFUSION, 4, 0, &pluginGui->diffusion);
	return;
}

static void 
on_inv_erreverb_display_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gint active_dot;
	IErReverbGui *pluginGui = (IErReverbGui *) data;

	active_dot=inv_display_err_get_active_dot(INV_DISPLAY_ERR (widget));

	switch(active_dot) {
		case INV_DISPLAY_ERR_DOT_SOURCE:
			pluginGui->sourceLR=inv_display_err_get_source(INV_DISPLAY_ERR (widget), INV_DISPLAY_ERR_LR);
			pluginGui->sourceFB=inv_display_err_get_source(INV_DISPLAY_ERR (widget), INV_DISPLAY_ERR_FB);
			(*pluginGui->write_function)(pluginGui->controller, IERR_SOURCELR, 4, 0, &pluginGui->sourceLR);
			(*pluginGui->write_function)(pluginGui->controller, IERR_SOURCEFB, 4, 0, &pluginGui->sourceFB);
			break;
		case INV_DISPLAY_ERR_DOT_DEST:
			pluginGui->destLR=inv_display_err_get_dest(INV_DISPLAY_ERR (widget), INV_DISPLAY_ERR_LR);
			pluginGui->destFB=inv_display_err_get_dest(INV_DISPLAY_ERR (widget), INV_DISPLAY_ERR_FB);
			(*pluginGui->write_function)(pluginGui->controller, IERR_DESTLR, 4, 0, &pluginGui->destLR);
			(*pluginGui->write_function)(pluginGui->controller, IERR_DESTFB, 4, 0, &pluginGui->destFB);
			break;
	}
	return;
}
