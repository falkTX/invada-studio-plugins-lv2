/* 

    This LV2 extension provides input module gui's

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
#include "widgets/meter-phase.h"
#include "widgets/switch-toggle.h"
#include "../plugin/inv_testtone.h"
#include "inv_testtone_gui.h"


static LV2UI_Descriptor *IToneGuiDescriptor = NULL;

typedef struct {
	GtkWidget	*windowContainer;
	GtkWidget	*heading;
	GtkWidget	*toggleBypass;
	GtkWidget	*meterOut;
	GtkWidget	*knobFreq;
	GtkWidget	*knobTrim;
	GtkWidget	*treeTest;
	GtkWidget	*treeMusic;

	gint		OutChannels;
	float 		bypass;
	float		freq;
	float		trim;


	LV2UI_Write_Function 	write_function;
	LV2UI_Controller 	controller;

} IToneGui;



static LV2UI_Handle 
instantiateIToneGui(const struct LV2UI_Descriptor* descriptor, const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{

	IToneGui *pluginGui = (IToneGui *)malloc(sizeof(IToneGui));
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
	file = g_strdup_printf("%s/gtk/inv_testtone_gui.xml",bundle_path);
	gtk_builder_add_from_file (builder, file, &err);
	free(file);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "testtone_window"));

	/* get pointers to some useful widgets from the design */
	pluginGui->windowContainer = GTK_WIDGET (gtk_builder_get_object (builder, "testtone_container"));
	pluginGui->heading = GTK_WIDGET (gtk_builder_get_object (builder, "label_heading"));
	pluginGui->treeTest = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_testtones"));
	pluginGui->treeMusic = GTK_WIDGET (gtk_builder_get_object (builder, "treeview_musictones"));

	/* add custom widgets */
	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_bypass_toggle"));
	pluginGui->toggleBypass = inv_switch_toggle_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->toggleBypass);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_meter_out"));
	pluginGui->meterOut = inv_meter_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->meterOut);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_frequency_knob"));
	pluginGui->knobFreq = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobFreq);

	tempObject=GTK_WIDGET (gtk_builder_get_object (builder, "alignment_trim_knob"));
	pluginGui->knobTrim = inv_knob_new ();
	gtk_container_add (GTK_CONTAINER (tempObject), pluginGui->knobTrim);

	pluginGui->OutChannels	= 1;
	pluginGui->bypass	= 0.0;
	pluginGui->freq		= 1000.0;
	pluginGui->trim		= 0.0;

	inv_meter_set_bypass(INV_METER (pluginGui->meterOut),INV_PLUGIN_ACTIVE);
	inv_meter_set_mode(INV_METER (pluginGui->meterOut), INV_METER_DRAW_MODE_BIGTOZERO);
	inv_meter_set_channels(INV_METER (pluginGui->meterOut), pluginGui->OutChannels);
	inv_meter_set_LdB(INV_METER (pluginGui->meterOut),-90);
	inv_meter_set_RdB(INV_METER (pluginGui->meterOut),-90);

	inv_switch_toggle_set_bypass( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_PLUGIN_ACTIVE);
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, 0.0, 1.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF, "On");
	inv_switch_toggle_set_value( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0);
	inv_switch_toggle_set_colour(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  1.0, 0.0, 0.0);
	inv_switch_toggle_set_text(  INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON,  "Off");
	inv_switch_toggle_set_state( INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
	inv_switch_toggle_set_tooltip(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), "<span size=\"8000\"><b>Description:</b> This switch activates the plugin.\n<b>Usage:</b> Click to toggle between values.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->toggleBypass),"button-release-event",G_CALLBACK(on_inv_tone_bypass_toggle_button_release),pluginGui);

	inv_knob_set_bypass(   INV_KNOB (pluginGui->knobFreq), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(     INV_KNOB (pluginGui->knobFreq), INV_KNOB_SIZE_LARGE);
	inv_knob_set_curve(    INV_KNOB (pluginGui->knobFreq), INV_KNOB_CURVE_LOG);
	inv_knob_set_markings( INV_KNOB (pluginGui->knobFreq), INV_KNOB_MARKINGS_4); 
	inv_knob_set_highlight(INV_KNOB (pluginGui->knobFreq), INV_KNOB_HIGHLIGHT_C);
	inv_knob_set_human(    INV_KNOB (pluginGui->knobFreq));
	inv_knob_set_units(    INV_KNOB (pluginGui->knobFreq), "Hz");
	inv_knob_set_min(      INV_KNOB (pluginGui->knobFreq), 20.0);
	inv_knob_set_max(      INV_KNOB (pluginGui->knobFreq), 20000.0);
	inv_knob_set_value(    INV_KNOB (pluginGui->knobFreq), pluginGui->freq);
	inv_knob_set_tooltip(  INV_KNOB (pluginGui->knobFreq), "<span size=\"8000\"><b>Description:</b> This knob sets the oscillator frequency.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobFreq),"motion-notify-event",G_CALLBACK(on_inv_tone_freq_knob_motion),pluginGui);

	inv_knob_set_bypass(INV_KNOB (pluginGui->knobTrim), INV_PLUGIN_ACTIVE);
	inv_knob_set_size(INV_KNOB (pluginGui->knobTrim), INV_KNOB_SIZE_MEDIUM);
	inv_knob_set_curve(INV_KNOB (pluginGui->knobTrim), INV_KNOB_CURVE_LINEAR);
	inv_knob_set_markings(INV_KNOB (pluginGui->knobTrim), INV_KNOB_MARKINGS_5);
	inv_knob_set_highlight(INV_KNOB (pluginGui->knobTrim), INV_KNOB_HIGHLIGHT_L);
	inv_knob_set_units(INV_KNOB (pluginGui->knobTrim), "dB");
	inv_knob_set_min(INV_KNOB (pluginGui->knobTrim), -24.0);
	inv_knob_set_max(INV_KNOB (pluginGui->knobTrim), 0.0);
	inv_knob_set_value(INV_KNOB (pluginGui->knobTrim), pluginGui->trim);
	inv_knob_set_tooltip(INV_KNOB (pluginGui->knobTrim), "<span size=\"8000\"><b>Description:</b> This knob sets the output trim.\n<b>Usage:</b> Click and drag vertically to change value, hortizontally to change the sensitvity.</span>");
	g_signal_connect_after(G_OBJECT(pluginGui->knobTrim),"motion-notify-event",G_CALLBACK(on_inv_tone_trim_knob_motion),pluginGui);


	inv_tone_create_testtone(pluginGui->treeTest);
	g_signal_connect_after(G_OBJECT(pluginGui->treeTest),"button-release-event",G_CALLBACK(on_inv_tone_test_button_release),pluginGui);

	inv_tone_create_musictone(pluginGui->treeMusic);
	g_signal_connect_after(G_OBJECT(pluginGui->treeMusic),"button-release-event",G_CALLBACK(on_inv_tone_music_button_release),pluginGui);

	/* strip the parent window from the design so the host can attach its own */
	gtk_widget_ref(pluginGui->windowContainer);
	gtk_container_remove(GTK_CONTAINER(window), pluginGui->windowContainer);

	*widget = (LV2UI_Widget) pluginGui->windowContainer;

	g_object_unref (G_OBJECT (builder));
 
	/* return the instance */
	return pluginGui;
}


static void 
cleanupIToneGui(LV2UI_Handle ui)
{
	return;
}


static void 
port_eventIToneGui(LV2UI_Handle ui, uint32_t port, uint32_t buffer_size, uint32_t format, const void*  buffer)
{
	IToneGui *pluginGui = (IToneGui *)ui;

	float value;

	if(format==0) 
	{
		value=* (float *) buffer;
		switch(port)
		{
			case ITONE_ACTIVE:
				pluginGui->bypass=value;
				if(value <= 0.0) {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_OFF);
					inv_meter_set_bypass(       INV_METER         (pluginGui->meterOut),     INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB          (pluginGui->knobFreq),     INV_PLUGIN_ACTIVE);
					inv_knob_set_bypass(        INV_KNOB          (pluginGui->knobTrim),     INV_PLUGIN_ACTIVE);
					gtk_widget_set_sensitive(   GTK_WIDGET        (pluginGui->treeTest),     TRUE);
					gtk_widget_set_sensitive(   GTK_WIDGET        (pluginGui->treeMusic),    TRUE);
				} else {
					inv_switch_toggle_set_state(INV_SWITCH_TOGGLE (pluginGui->toggleBypass), INV_SWITCH_TOGGLE_ON);
					inv_meter_set_bypass(       INV_METER         (pluginGui->meterOut),     INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB          (pluginGui->knobFreq),     INV_PLUGIN_BYPASS);
					inv_knob_set_bypass(        INV_KNOB          (pluginGui->knobTrim),     INV_PLUGIN_BYPASS);
					gtk_widget_set_sensitive(   GTK_WIDGET        (pluginGui->treeTest),     FALSE);
					gtk_widget_set_sensitive(   GTK_WIDGET        (pluginGui->treeMusic),    FALSE);
				}
				gtk_widget_queue_draw (pluginGui->windowContainer);
				break;
			case ITONE_FREQ:
				pluginGui->freq=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobFreq), pluginGui->freq);
				break;
			case ITONE_TRIM:
				pluginGui->trim=value;
				inv_knob_set_value(INV_KNOB (pluginGui->knobTrim), pluginGui->trim);
				break;
			case ITONE_METER_OUT:
				inv_meter_set_LdB(INV_METER (pluginGui->meterOut),value);
				break;
		}
	}
}


static void 
init()
{
	IToneGuiDescriptor =
	 (LV2UI_Descriptor *)malloc(sizeof(LV2UI_Descriptor));

	IToneGuiDescriptor->URI 		= ITONE_GUI_URI;
	IToneGuiDescriptor->instantiate 	= instantiateIToneGui;
	IToneGuiDescriptor->cleanup		= cleanupIToneGui;
	IToneGuiDescriptor->port_event		= port_eventIToneGui;
	IToneGuiDescriptor->extension_data 	= NULL;

}


const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
	if (!IToneGuiDescriptor) init();

	switch (index) {
		case 0:
			return IToneGuiDescriptor;
	default:
		return NULL;
	}
}

/*****************************************************************************/

static  gint
inv_tone_get_col_number_from_tree_view_column (GtkTreeViewColumn *col)
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
inv_tone_cell_data_function (  GtkTreeViewColumn *col,
				GtkCellRenderer   *renderer,
				GtkTreeModel      *model,
				GtkTreeIter       *iter,
				gint              pos) {
	gfloat  value;
	gchar   buf[20];

	gtk_tree_model_get(model, iter, pos, &value, -1);
	if(value < 20) {
		g_snprintf(buf, sizeof(buf), " ");
	} else if(value < 100) {
		g_snprintf(buf, sizeof(buf), "%.1fHz ", floor(value*10)/10);
	} else if(value < 1000) {
		g_snprintf(buf, sizeof(buf), "%.0fHz", floor(value));
	} else if(value < 10000) {
		g_snprintf(buf, sizeof(buf), "%.2fkHz", floor(value/10)/100);
	} else if(value <= 20000) {
		g_snprintf(buf, sizeof(buf), "%.1fkHz", floor(value/100)/10);
	} else {
		g_snprintf(buf, sizeof(buf), " ");
	}
	g_object_set(renderer, "text", buf, NULL);
}

static void
inv_tone_cola_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,1);
}
static void
inv_tone_colb_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,2);
}
static void
inv_tone_colc_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,3);
}
static void
inv_tone_cold_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,4);
}
static void
inv_tone_cole_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,5);
}
static void
inv_tone_colf_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,6);
}
static void
inv_tone_colg_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,7);
}
static void
inv_tone_colh_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,8);
}
static void
inv_tone_coli_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,9);
}
static void
inv_tone_colj_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,10);
}
static void
inv_tone_colk_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,11);
}
static void
inv_tone_coll_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

	inv_tone_cell_data_function (col,renderer,model,iter,12);
}

static void 
inv_tone_create_testtone(GtkWidget *tree)
{

	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;

	// define columns 
	/* mult */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Multiplier");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",0.5,NULL);
	g_object_set (renderer,"weight",800,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", TV1_COLUMN_SCALE);

	/* 20 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "20");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_cola_cell_data_function, NULL, NULL);

	/* 25 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "25");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colb_cell_data_function, NULL, NULL);

	/* 31.5 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "31.5");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colc_cell_data_function, NULL, NULL);

	/* 40 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "40");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_cold_cell_data_function, NULL, NULL);

	/* 50 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "50");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_cole_cell_data_function, NULL, NULL);

	/* 63 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "63");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colf_cell_data_function, NULL, NULL);

	/* 80 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "80");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colg_cell_data_function, NULL, NULL);

	/* 100 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "100");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colh_cell_data_function, NULL, NULL);

	/* 125 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "125");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_coli_cell_data_function, NULL, NULL);

	/* 160 */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "160");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colj_cell_data_function, NULL, NULL);

	/* tooltips */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "tooltips");
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	gtk_tree_view_column_set_visible (col,FALSE);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tree),TV1_COLUMN_TOOLTIP);


	GtkListStore 	*liststore;
	GtkTreeIter   	iter;
	gint		i,mul;
	char		notelabel[16];

	// create empty store
	liststore = gtk_list_store_new( TV1_NUM_COLS, 
					G_TYPE_STRING, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_STRING);
	for(i=0;i<4;i++) {
		gtk_list_store_append(liststore, &iter);
		mul=(int)pow(10,i);
		sprintf(notelabel,"%ix ",mul);
		gtk_list_store_set (liststore, &iter, 
			TV1_COLUMN_SCALE, notelabel, 
			TV1_COLUMN_A,     (float)mul*20.0,
			TV1_COLUMN_B,     (float)mul*25,
			TV1_COLUMN_C,     (float)mul*31.5,
			TV1_COLUMN_D,     (float)mul*40,
			TV1_COLUMN_E,     (float)mul*50,
			TV1_COLUMN_F,     (float)mul*63,
			TV1_COLUMN_G,     (float)mul*80,
			TV1_COLUMN_H,     (float)mul*100,
			TV1_COLUMN_I,     (float)mul*125,
			TV1_COLUMN_J,     (float)mul*160,
			TV1_COLUMN_TOOLTIP,   "<span size=\"8000\"><b>Description:</b> Test tones at standard 1/3 octave frequencies.\n<b>Usage:</b> Left-Click on a cell to assign the value to the oscillator.</span>",
			-1);
	}

	// add the model to the treeview
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(liststore));

	return;
}

static void 
inv_tone_create_musictone(GtkWidget *tree)
{

	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;

	// define columns 
	/* octave */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Ocatve");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",0.5,NULL);
	g_object_set (renderer,"weight",800,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", TV2_COLUMN_OCTAVE);



	/* C */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "C");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_cola_cell_data_function, NULL, NULL);

	/* C# */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "C#");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colb_cell_data_function, NULL, NULL);

	/* D */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "D");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colc_cell_data_function, NULL, NULL);

	/* D# */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "D#");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_cold_cell_data_function, NULL, NULL);

	/* E */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "E");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_cole_cell_data_function, NULL, NULL);

	/* F */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "F");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colf_cell_data_function, NULL, NULL);

	/* F# */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "F#");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colg_cell_data_function, NULL, NULL);

	/* G */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "G");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colh_cell_data_function, NULL, NULL);

	/* G# */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "G#");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_coli_cell_data_function, NULL, NULL);

	/* A */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "A");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colj_cell_data_function, NULL, NULL);

	/* A# */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "A#");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_colk_cell_data_function, NULL, NULL);

	/* B */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "B");
	gtk_tree_view_column_set_spacing(col,2);
	gtk_tree_view_column_set_alignment(col,0.5);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer,"xalign",1.0,NULL);
	g_object_set (renderer,"size",8000,NULL);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, inv_tone_coll_cell_data_function, NULL, NULL);

	/* tooltips */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "tooltips");
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
	gtk_tree_view_column_set_visible (col,FALSE);
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tree),TV2_COLUMN_TOOLTIP);


	GtkListStore 	*liststore;
	GtkTreeIter   	iter;
	gint		i;
	float 		scale;
	char		notelabel[8];

	// create empty store
	liststore = gtk_list_store_new( TV2_NUM_COLS, 
					G_TYPE_STRING, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_FLOAT, 
					G_TYPE_STRING);
	for(i=0;i<11;i++) {
		gtk_list_store_append(liststore, &iter);
		scale=pow(2,i-5);
		sprintf(notelabel,"%i",i);
		gtk_list_store_set (liststore, &iter, 
			TV2_COLUMN_OCTAVE, notelabel, 
			TV2_COLUMN_A,     scale*440.0*pow(2,3.0/12.0),
			TV2_COLUMN_B,     scale*440.0*pow(2,4.0/12.0),
			TV2_COLUMN_C,     scale*440.0*pow(2,5.0/12.0),
			TV2_COLUMN_D,     scale*440.0*pow(2,6.0/12.0),
			TV2_COLUMN_E,     scale*440.0*pow(2,7.0/12.0),
			TV2_COLUMN_F,     scale*440.0*pow(2,8.0/12.0),
			TV2_COLUMN_G,     scale*440.0*pow(2,9.0/12.0),
			TV2_COLUMN_H,     scale*440.0*pow(2,10.0/12.0),
			TV2_COLUMN_I,     scale*440.0*pow(2,11.0/12.0),
			TV2_COLUMN_J,     2*scale*440.0,
			TV2_COLUMN_K,     2*scale*440.0*pow(2,1.0/12.0),
			TV2_COLUMN_L,     2*scale*440.0*pow(2,2.0/12.0),
			TV2_COLUMN_TOOLTIP,   "<span size=\"8000\"><b>Description:</b> Test tones at musical frequencies.\n<b>Usage:</b> Left-Click on a cell to assign the value to the oscillator.</span>",
			-1);
	}

	// add the model to the treeview
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(liststore));

	return;
}

/*****************************************************************************/

static void 
on_inv_tone_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IToneGui *pluginGui = (IToneGui *) data;

	pluginGui->bypass=inv_switch_toggle_get_value(INV_SWITCH_TOGGLE (widget));
	(*pluginGui->write_function)(pluginGui->controller, ITONE_ACTIVE, 4, 0, &pluginGui->bypass);
	return;
}

static void 
on_inv_tone_freq_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	IToneGui *pluginGui = (IToneGui *) data;

	pluginGui->freq=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ITONE_FREQ, 4, 0, &pluginGui->freq);
	return;
}

static void 
on_inv_tone_trim_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	IToneGui *pluginGui = (IToneGui *) data;

	pluginGui->trim=inv_knob_get_value(INV_KNOB (widget));
	(*pluginGui->write_function)(pluginGui->controller, ITONE_TRIM, 4, 0, &pluginGui->trim);
	return;
}

static void 
on_inv_tone_test_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkTreePath 		*path;
	GtkTreeViewColumn 	*column;
	GtkTreeModel 		*model;
    	GtkTreeIter   		iter;

	gfloat			value;
	gint			col;

	IToneGui *pluginGui = (IToneGui *) data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW (pluginGui->treeTest));

	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW (pluginGui->treeTest), 
					(gint) ((GdkEventButton*)event)->x,
					(gint) ((GdkEventButton*)event)->y, 
					&path, &column, NULL, NULL);

	col=inv_tone_get_col_number_from_tree_view_column(column);

	if (col > 0 && gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, col, &value, -1);
		if(value >= 20.0 && value <= 20000.0) {
			pluginGui->freq=value;
			inv_knob_set_value(INV_KNOB (pluginGui->knobFreq), pluginGui->freq);
			(*pluginGui->write_function)(pluginGui->controller, ITONE_FREQ, 4, 0, &pluginGui->freq);
		}
	}

	gtk_tree_path_free(path);
	return;
}

static void 
on_inv_tone_music_button_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkTreePath 		*path;
	GtkTreeViewColumn 	*column;
	GtkTreeModel 		*model;
    	GtkTreeIter   		iter;

	gfloat			value;
	gint			col;

	IToneGui *pluginGui = (IToneGui *) data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW (pluginGui->treeMusic));

	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW (pluginGui->treeMusic), 
					(gint) ((GdkEventButton*)event)->x,
					(gint) ((GdkEventButton*)event)->y, 
					&path, &column, NULL, NULL);

	col=inv_tone_get_col_number_from_tree_view_column(column);

	if (col > 0 && gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, col, &value, -1);
		if(value >= 20.0 && value <= 20000.0) {
			pluginGui->freq=value;
			inv_knob_set_value(INV_KNOB (pluginGui->knobFreq), pluginGui->freq);
			(*pluginGui->write_function)(pluginGui->controller, ITONE_FREQ, 4, 0, &pluginGui->freq);
		}
	}

	gtk_tree_path_free(path);
	return;
}
