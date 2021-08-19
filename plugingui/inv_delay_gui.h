/* 

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
/* enum for delay calc tree */
enum {
	COLUMN_NOTE = 0,
	COLUMN_LENGTH,
	COLUMN_DOTTED,
	COLUMN_TUPLET32,
	COLUMN_TUPLET54,
	COLUMN_TUPLET74,
	COLUMN_TUPLET94,
	COLUMN_TUPLET114,
	COLUMN_TOOLTIP,
	NUM_COLS
};


/* helper functions */
static gint inv_delay_get_col_number_from_tree_view_column (GtkTreeViewColumn *col);
static void inv_delay_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gint pos);
static void inv_delay_length_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_dotted_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_tuplet32_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_tuplet54_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_tuplet74_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_tuplet94_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_tuplet114_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_delay_init_delaycalc(GtkWidget *tree);
static void inv_delay_update_delaycalc(GtkWidget *tree, float tempo);

/* call backs */
static void on_inv_delay_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_mode_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_mungemode_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_munge_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_cycle_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_width_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_delay1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_fb1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_pan1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_vol1_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_delay2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_fb2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_pan2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_vol2_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_delay_tempo_value_changed(GtkWidget *widget, gpointer data);
static void on_inv_delay_calc_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);





