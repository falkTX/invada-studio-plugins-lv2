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

/* enum for test tones tree */
enum {
	TV1_COLUMN_SCALE = 0,
	TV1_COLUMN_A,   //20
	TV1_COLUMN_B,   //25
	TV1_COLUMN_C,   //31.5
	TV1_COLUMN_D,   //40
	TV1_COLUMN_E,   //50
	TV1_COLUMN_F,   //63
	TV1_COLUMN_G,   //80
	TV1_COLUMN_H,   //100
	TV1_COLUMN_I,   //125
	TV1_COLUMN_J,   //160
	TV1_COLUMN_TOOLTIP,
	TV1_NUM_COLS
};

/* enum for musical tones tree */
enum {
	TV2_COLUMN_OCTAVE = 0,
	TV2_COLUMN_A,   //C
	TV2_COLUMN_B,   //C#
	TV2_COLUMN_C,   //D
	TV2_COLUMN_D,   //D#
	TV2_COLUMN_E,   //E
	TV2_COLUMN_F,   //F
	TV2_COLUMN_G,   //F#
	TV2_COLUMN_H,   //G
	TV2_COLUMN_I,   //G#
	TV2_COLUMN_J,   //A
	TV2_COLUMN_K,   //A#
	TV2_COLUMN_L,   //B
	TV2_COLUMN_TOOLTIP,
	TV2_NUM_COLS
};

/*helper functions */
static gint inv_tone_get_col_number_from_tree_view_column (GtkTreeViewColumn *col);
static void inv_tone_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gint pos);
static void inv_tone_cola_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colb_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colc_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_cold_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_cole_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colf_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colg_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colh_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_coli_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colj_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_colk_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_coll_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void inv_tone_create_testtone(GtkWidget *tree);
static void inv_tone_create_musictone(GtkWidget *tree);

/*callbacks*/
static void on_inv_tone_bypass_toggle_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_tone_freq_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_tone_trim_knob_motion(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_tone_test_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);
static void on_inv_tone_music_button_release(GtkWidget *widget, GdkEvent *event, gpointer data);
