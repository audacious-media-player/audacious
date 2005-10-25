/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "lv_log.h"
#include "lv_ui.h"

static int box_dtor (VisObject *object);
static int table_dtor (VisObject *object);
static int table_entry_dtor (VisObject *object);
static int frame_dtor (VisObject *object);
static int choice_dtor (VisObject *object);
static int widget_dtor (VisObject *object);

static int box_dtor (VisObject *object)
{
	VisUIBox *box = VISUAL_UI_BOX (object);

	visual_list_destroy_elements (&box->childs);

	widget_dtor (object);

	return VISUAL_OK;
}

static int table_dtor (VisObject *object)
{
	VisUITable *table = VISUAL_UI_TABLE (object);

	visual_list_destroy_elements (&table->childs);
	
	widget_dtor (object);

	return VISUAL_OK;
}

static int table_entry_dtor (VisObject *object)
{
	VisUITableEntry *tentry = VISUAL_UI_TABLE_ENTRY (object);

	if (tentry->widget != NULL)
		visual_object_unref (VISUAL_OBJECT (tentry->widget));

	tentry->widget = NULL;

	return VISUAL_OK;
}

static int frame_dtor (VisObject *object)
{
	VisUIContainer *container = VISUAL_UI_CONTAINER (object);

	if (container->child != NULL)
		visual_object_unref (VISUAL_OBJECT (container->child));

	container->child = NULL;

	widget_dtor (object);

	return VISUAL_OK;
}


static int choice_dtor (VisObject *object)
{
	visual_ui_choice_free_choices (VISUAL_UI_CHOICE (object));

	widget_dtor (object);
	
	return VISUAL_OK;
}

static int widget_dtor (VisObject *object)
{
	VisUIWidget *widget = VISUAL_UI_WIDGET (object);

	if (widget->tooltip != NULL)
		visual_mem_free ((char *) widget->tooltip);

	widget->tooltip = NULL;

	return VISUAL_OK;
}

/**
 * @defgroup VisUI VisUI
 * @{
 */

/**
 * Creates a new VisUIWidget structure.
 *
 * @return A newly allocated VisUIWidget, or NULL on failure.
 */
VisUIWidget *visual_ui_widget_new ()
{
	VisUIWidget *widget;

	widget = visual_mem_new0 (VisUIWidget, 1);
	widget->type = VISUAL_WIDGET_TYPE_NULL;

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (widget), TRUE, widget_dtor);

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (widget), -1, -1);

	return widget;
}

/**
 * Sets a request for size to a VisUIWidget, to be used to request a certain dimension.
 *
 * @param widget Pointer to the VisUIWidget to which the size request is set.
 * @param width The width in pixels of the size requested.
 * @param height The height in pixels of the size requested.
 *
 * @return VISUAL_OK on succes, or -VISUAL_ERROR_UI_WIDGET_NULL on failure.
 */
int visual_ui_widget_set_size_request (VisUIWidget *widget, int width, int height)
{
	visual_log_return_val_if_fail (widget != NULL, -VISUAL_ERROR_UI_WIDGET_NULL);

	widget->width = width;
	widget->height = height;

	return VISUAL_OK;
}

/**
 * Sets a tooltip to a VisUIWidget.
 *
 * @param widget Pointer to the VisUIWidget to which the tooltip is set.
 * @param tooltip A string containing the tooltip text.
 *
 * @return VISUAL_OK on succes, or -VISUAL_ERROR_UI_WIDGET_NULL on failure.
 */
int visual_ui_widget_set_tooltip (VisUIWidget *widget, const char *tooltip)
{
	visual_log_return_val_if_fail (widget != NULL, -VISUAL_ERROR_UI_WIDGET_NULL);

	if (widget->tooltip != NULL)
		visual_mem_free ((char *) widget->tooltip);
	
	widget->tooltip = strdup (tooltip);

	return VISUAL_OK;
}

/**
 * Retrieves the tooltip that is set to a VisUIWidget.
 *
 * @param widget Pointer to the VisUIWidget from which the tooltip is requested.
 *
 * @return The tooltip, possibly NULL, NULL on failure.
 */
const char *visual_ui_widget_get_tooltip (VisUIWidget *widget)
{
	visual_log_return_val_if_fail (widget != NULL, NULL);

	return widget->tooltip;
}

/**
 * Gets the top VisUIWidget from a VisUIWidget, this means that it will retrieve
 * the widget that is the parent of all underlaying widgets.
 *
 * @param widget Pointer to the VisUIWidget of which we want to have the top VisUIWidget.
 *
 * @return Pointer to the top VisUIWidget, or NULL on failure.
 */
VisUIWidget *visual_ui_widget_get_top (VisUIWidget *widget)
{
	VisUIWidget *above;
	VisUIWidget *prev = widget;

	visual_log_return_val_if_fail (widget != NULL, NULL);

	while ((above = visual_ui_widget_get_parent (widget)) != NULL) {
		prev = widget;
	}

	return prev;
}

/**
 * Gets the parent VisUIWidget from a VisUIWidget, this retrieves the parent of the given
 * widget.
 *
 * @param widget Pointer to the VisUIWidget of which the parent is requested.
 *
 * @return Pointer to the parent VisUIWidget, or NULL on failure.
 */
VisUIWidget *visual_ui_widget_get_parent (VisUIWidget *widget)
{
	visual_log_return_val_if_fail (widget != NULL, NULL);

	return widget->parent;
}

/**
 * Gets the VisUIWidgetType type from a VisUIWidget, this contains what kind of widget the given
 * VisUIWidget is.
 *
 * @param widget Pointer to the VisUIWidget of which the type is requested.
 *
 * @return The VisUIWidgetType of the given VisUIWidget.
 */
VisUIWidgetType visual_ui_widget_get_type (VisUIWidget *widget)
{
	visual_log_return_val_if_fail (widget != NULL, VISUAL_WIDGET_TYPE_NULL);

	return widget->type;
}

/**
 * Adds a VisUIWidget to a VisUIContainer.
 *
 * @see visual_ui_box_pack
 * @see visual_ui_table_attach
 * 
 * @param container Pointer to the VisUIContainer in which a VisUIWidget is put.
 * @param widget Pointer to the VisUIWidget that is been put in the VisUIContainer.
 *
 * @return VISUAL_OK on succes, or -VISUAL_ERROR_UI_CONTAINER_NULL, -VISUAL_ERROR_UI_WIDGET_NULL on failure.
 */
int visual_ui_container_add (VisUIContainer *container, VisUIWidget *widget)
{
	visual_log_return_val_if_fail (container != NULL, -VISUAL_ERROR_UI_CONTAINER_NULL);
	visual_log_return_val_if_fail (widget != NULL, -VISUAL_ERROR_UI_WIDGET_NULL);

	container->child = widget;

	return VISUAL_OK;
}

/**
 * Gets the child VisUIWidget from a VisUIContainer.
 *
 * @param container Pointer to the VisUIContainer of which we want the child VisUIWidget.
 *
 * @return The child VisUIWidget, or NULL on failure.
 */
VisUIWidget *visual_ui_container_get_child (VisUIContainer *container)
{
	visual_log_return_val_if_fail (container != NULL, NULL);

	return container->child;
}

/**
 * Creates a new VisUIBox, that can be used to pack VisUIWidgets in.
 *
 * @param orient Indicates the orientation style of the box, being either
 * 	VISUAL_ORIENT_TYPE_HORIZONTAL or VISUAL_ORIENT_TYPE_VERTICAL.
 *
 * @return The newly created VisUIBox in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_box_new (VisUIOrientType orient)
{
	VisUIBox *box;

	box = visual_mem_new0 (VisUIBox, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (box), TRUE, box_dtor);

	VISUAL_UI_WIDGET (box)->type = VISUAL_WIDGET_TYPE_BOX;

	box->orient = orient;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (box), -1, -1);

	visual_list_set_destroyer (&box->childs, visual_object_list_destroyer);

	return VISUAL_UI_WIDGET (box);
}

/**
 * Packs VisUIWidgets into a VisUIBox, this can be used to pack widgets either vertically or horizontally,
 * depending on the type of box.
 *
 * @param box Pointer to the VisUIBox in which the widget is packed.
 * @param widget Pointer to the VisUIWidget which is packed in the box.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_BOX_NULL, -VISUAL_ERROR_UI_WIDGET_NULL or
 * 	error values returned by visual_list_add () on failure.
 */
int visual_ui_box_pack (VisUIBox *box, VisUIWidget *widget)
{
	visual_log_return_val_if_fail (box != NULL, -VISUAL_ERROR_UI_BOX_NULL);
	visual_log_return_val_if_fail (widget != NULL, -VISUAL_ERROR_UI_WIDGET_NULL);

	return visual_list_add (&box->childs, widget);
}

/**
 * Retrieve a VisList of VisUIWidget elements, being the childs of the VisUIBox.
 * 
 * @param box Pointer to the VisUIBox from which the childs are requested.
 * 
 * @return VisList containing the childs of the VisUIBox or NULL on failure.
 */
VisList *visual_ui_box_get_childs (VisUIBox *box)
{
	VisUIWidget *next;
	VisListEntry *le = NULL;

	visual_log_return_val_if_fail (box != NULL, NULL);

	return &box->childs;
}

/**
 * Get the VisUIOrientType value from a VisUIBox.
 *
 * @param box VisUIBox from which the VisUIOrientType is requested.
 *
 * @return VisUIOrientType containing the orientation style for this VisUIBox.
 */
VisUIOrientType visual_ui_box_get_orient (VisUIBox *box)
{
	visual_log_return_val_if_fail (box != NULL, VISUAL_ORIENT_TYPE_NONE);

	return box->orient;
}

/**
 * Creates a new VisUITable, that can be used to attach VisUIWidgets to cells in the table.
 *
 * @param rows The number of rows in the table.
 * @param cols The number of columns in the table.
 *
 * @return The newly created VisUITable in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_table_new (int rows, int cols)
{
	VisUITable *table;

	table = visual_mem_new0 (VisUITable, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (table), TRUE, table_dtor);

	VISUAL_UI_WIDGET (table)->type = VISUAL_WIDGET_TYPE_TABLE;

	table->rows = rows;
	table->cols = cols;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (table), -1, -1);

	visual_list_set_destroyer (&table->childs, visual_object_list_destroyer);

	return VISUAL_UI_WIDGET (table);
}

/**
 * Creates a new VisUITableEntry. You probably never need this function, but it's used internally.
 *
 * @param widget Pointer to the VisUIWidget of which a VisUITableEntry is created.
 * @param row The row this entry will be placed.
 * @param col The column this entry will be placed.
 *
 * @return A newly allocated VisUITableEntry, NULL on failure.
 */
VisUITableEntry *visual_ui_table_entry_new (VisUIWidget *widget, int row, int col)
{
	VisUITableEntry *tentry;
	
	visual_log_return_val_if_fail (widget != NULL, NULL);

	tentry = visual_mem_new0 (VisUITableEntry, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (tentry), TRUE, table_entry_dtor);

	tentry->row = row;
	tentry->col = col;

	tentry->widget = widget;

	return tentry;
}

/**
 * Attaches a VisUIWidget to a cell within a VisUITable.
 * 
 * @param table Pointer to the VisUITable to which a VisUiWidget is attached.
 * @param widget Pointer to the VisUIWidget that is being attached to the VisUITable.
 * @param row The row number starting at 0.
 * @param col The column number starting at 0.
 * 
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_TABLE_NULL, -VISUAL_ERROR_UI_WIDGET_NULL or
 * 	error values returned by visual_list_add () on failure.
 */
int visual_ui_table_attach (VisUITable *table, VisUIWidget *widget, int row, int col)
{
	VisUITableEntry *tentry;

	visual_log_return_val_if_fail (table != NULL, -VISUAL_ERROR_UI_TABLE_NULL);
	visual_log_return_val_if_fail (widget != NULL, -VISUAL_ERROR_UI_WIDGET_NULL);

	tentry = visual_ui_table_entry_new (widget, row, col);

	return visual_list_add (&table->childs, tentry);
}

/**
 * Retrieve a VisList containing VisUITableEntry elements, in which the child VisUIWidget and it's place
 * 	in the VisUITable is stored.
 *
 * @param table Pointer to the VisUITable from which the childs are requested.
 *
 * @return VisList containing the childs of the VisUITable, or NULL on failure.
 */
VisList *visual_ui_table_get_childs (VisUITable *table)
{
	visual_log_return_val_if_fail (table != NULL, NULL);

	return &table->childs;
}

/**
 * Creates a new VisUIFrame, which can be used to put a frame around a VisUIWidget.
 * 
 * @param name The name of this frame.
 *
 * @return The newly created VisUIFrame in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_frame_new (const char *name)
{
	VisUIFrame *frame;

	frame = visual_mem_new0 (VisUIFrame, 1);
	
	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (frame), TRUE, frame_dtor);

	VISUAL_UI_WIDGET (frame)->type = VISUAL_WIDGET_TYPE_FRAME;

	frame->name = name;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (frame), -1, -1);
	
	return VISUAL_UI_WIDGET (frame);
}

/**
 * Creates a new VisUILabel, which can be used as a one line piece of text in an user interface.
 *
 * @param text Text of which the label consists.
 * @param bold Flag that indicates if a label should be drawn bold or not.
 *
 * @return The newly created VisUILabel in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_label_new (const char *text, int bold)
{
	VisUILabel *label;

	label = visual_mem_new0 (VisUILabel, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (label), TRUE, widget_dtor);

	VISUAL_UI_WIDGET (label)->type = VISUAL_WIDGET_TYPE_LABEL;

	label->text = text;
	label->bold = bold;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (label), -1, -1);
	
	return VISUAL_UI_WIDGET (label);
}

/**
 * Sets the bold flag for a VisUILabel.
 * 
 * @param label Pointer to the VisUILabel of which the bold flag is set, or unset.
 * @param bold Flag that indicates if a label should be drawn bold or not.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_LABEL_NULL on failure.
 */
int visual_ui_label_set_bold (VisUILabel *label, int bold)
{
	visual_log_return_val_if_fail (label != NULL, -VISUAL_ERROR_UI_LABEL_NULL);

	label->bold = bold;

	return VISUAL_OK;
}

/**
 * Sets the text for a VisUILabel.
 *
 * @param label Pointer to the VisUILabel to which text is being set.
 * @param text The text that is being set to the VisUILabel.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_LABEL_NULL on failure.
 */
int visual_ui_label_set_text (VisUILabel *label, const char *text)
{
	visual_log_return_val_if_fail (label != NULL, -VISUAL_ERROR_UI_LABEL_NULL);

	label->text = text;

	return VISUAL_OK;
}

/**
 * Retrieve the text from a VisUILabel.
 *
 * @param label Pointer to the VisUILabel from which the text is being requested.
 * 
 * return The text contained in the label, NULL on failure.
 */
const char *visual_ui_label_get_text (VisUILabel *label)
{
	visual_log_return_val_if_fail (label != NULL, NULL);

	return label->text;
}

/**
 * Creates a new VisUIImage, which can contain an image, loaded from a VisVideo.
 *
 * @param video The VisVideo containing the image to be displayed.
 *
 * @return The newly created VisUIImage in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_image_new (VisVideo *video)
{
	VisUIImage *image;

	image = visual_mem_new0 (VisUIImage, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (image), TRUE, widget_dtor);

	VISUAL_UI_WIDGET (image)->type = VISUAL_WIDGET_TYPE_IMAGE;

	image->image = video;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (image), -1, -1);

	return VISUAL_UI_WIDGET (image);
}

/**
 * Sets a VisVideo to a VisUIImage. The VisVideo contains the content of the image.
 *
 * @param image Pointer to the VisUIImage to which the VisVideo is set.
 * @param video Pointer to the VisVideo that is set to the VisUIImage.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_IMAGE_NULL on failure.
 */
int visual_ui_image_set_video (VisUIImage *image, VisVideo *video)
{
	visual_log_return_val_if_fail (image != NULL, -VISUAL_ERROR_UI_IMAGE_NULL);

	image->image = video;

	return VISUAL_OK;
}

/**
 * Retrieves the VisVideo from a VisUIImage.
 *
 * @param image Pointer to the VisUIImage from which the VisVideo is requested.
 * 
 * return The VisVideo that is connected to the VisUIImage.
 */
VisVideo *visual_ui_image_get_video (VisUIImage *image)
{
	visual_log_return_val_if_fail (image != NULL, NULL);

	return image->image;
}

/**
 * Creates a new VisUISeparator, which can function as a separation item between other VisUIWidgets.
 *
 * @param orient Indicates the orientation style of the separator, being either
 *	VISUAL_ORIENT_TYPE_HORIZONTAL or VISUAL_ORIENT_TYPE_VERTICAL.
 *
 * @return The newly created VisUISeparator in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_separator_new (VisUIOrientType orient)
{
	VisUISeparator *separator;

	separator = visual_mem_new0 (VisUISeparator, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (separator), TRUE, widget_dtor);

	VISUAL_UI_WIDGET (separator)->type = VISUAL_WIDGET_TYPE_SEPARATOR;

	separator->orient = orient;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (separator), -1, -1);

	return VISUAL_UI_WIDGET (separator);
}

/**
 * Get the VisUIOrientType value from a VisUISeparator.
 *
 * @param separator VisUISeparator from which the VisUIOrientType is requested.
 *
 * @return VisUIOrientType containing the orientation style for this VisUISeparator.
 */
VisUIOrientType visual_ui_separator_get_orient (VisUISeparator *separator)
{
	visual_log_return_val_if_fail (separator != NULL, VISUAL_ORIENT_TYPE_NONE);

	return separator->orient;
}

/**
 * Links a VisParamEntry to a VisUIMutator type.
 *
 * @param mutator Pointer to the VisUIMutator to which the VisParamEntry is linked.
 * @param param Pointer to the VisParamEntry that is linked to the VisUIMutator.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_MUTATOR_NULL or -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_ui_mutator_set_param (VisUIMutator *mutator, VisParamEntry *param)
{
	visual_log_return_val_if_fail (mutator != NULL, -VISUAL_ERROR_UI_MUTATOR_NULL);
	visual_log_return_val_if_fail (param != NULL, -VISUAL_ERROR_PARAM_NULL);

	/* FIXME Check if param is valid with mutator type, if not, give a critical */

	mutator->param = param;

	return VISUAL_OK;
}

/**
 * Request the VisParamEntry that is linked to a VisUIMutator.
 *
 * @param mutator Pointer to the VisUIMutator from which the VisParamEntry is requested.
 *
 * return The VisParamEntry that links to the VisUIMutator, or NULL on failure.
 */
VisParamEntry *visual_ui_mutator_get_param (VisUIMutator *mutator)
{
	visual_log_return_val_if_fail (mutator != NULL, NULL);

	return mutator->param;
}

/**
 * Set the properties for a VisUIRange.
 *
 * @param range Pointer to the VisUIRange to which the properties are set.
 * @param min The minimal value.
 * @param max The maximal value.
 * @param step The increase/decrease step value.
 * @param precision The precision in numbers behind the comma.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_RANGE_NULL on failure.
 */
int visual_ui_range_set_properties (VisUIRange *range, double min, double max, double step, int precision)
{
	visual_log_return_val_if_fail (range != NULL, -VISUAL_ERROR_UI_RANGE_NULL);

	range->min = min;
	range->max = max;
	range->step = step;
	range->precision = precision;

	return VISUAL_OK;
}

/**
 * Sets the maximal value for a VisUIRange.
 *
 * @param range Pointer to the VisUIRange to which the maximum value is set.
 * @param max The maximal value.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_RANGE_NULL on failure.
 */
int visual_ui_range_set_max (VisUIRange *range, double max)
{
	visual_log_return_val_if_fail (range != NULL, -VISUAL_ERROR_UI_RANGE_NULL);

	range->max = max;

	return VISUAL_OK;
}

/**
 * Sets the minimal value for a VisUIRange.
 *
 * @param range Pointer to the VisUIRange to which the minimal value is set.
 * @param min The minimal value.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_RANGE_NULL on failure.
 */
int visual_ui_range_set_min (VisUIRange *range, double min)
{
	visual_log_return_val_if_fail (range != NULL, -VISUAL_ERROR_UI_RANGE_NULL);

	range->min = min;

	return VISUAL_OK;
}

/**
 * Sets the increase/decrease step size for a VisUIRange.
 *
 * @param range Pointer to the VisUIRange to which the step size value is set.
 * @param step The increase/decrase step value.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_RANGE_NULL on failure.
 */
int visual_ui_range_set_step (VisUIRange *range, double step)
{
	visual_log_return_val_if_fail (range != NULL, -VISUAL_ERROR_UI_RANGE_NULL);

	range->step = step;

	return VISUAL_OK;
}

/**
 * Sets the precision for a VisUIRange.
 *
 * @param range Pointer to the VisUIRange to which the step size value is set.
 * @param precision The precision in numbers behind the comma.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_RANGE_NULL on failure.
 */
int visual_ui_range_set_precision (VisUIRange *range, int precision)
{
	visual_log_return_val_if_fail (range != NULL, -VISUAL_ERROR_UI_RANGE_NULL);

	range->precision = precision;

	return VISUAL_OK;
}

/**
 * Creates a new VisUIEntry, which can be used to enter text.
 *
 * @return The newly created VisUIEntry in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_entry_new ()
{
	VisUIEntry *entry;

	entry = visual_mem_new0 (VisUIEntry, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (entry), TRUE, widget_dtor);

	VISUAL_UI_WIDGET (entry)->type = VISUAL_WIDGET_TYPE_ENTRY;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (entry), -1, -1);

	return VISUAL_UI_WIDGET (entry);
}

/**
 * Sets the maximum length for the text in a VisUIEntry.
 *
 * @param entry Pointer to the VisUIEntry to which the maximum text length is set.
 * @param length The maximum text length for the VisUIEntry.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_ENTRY_NULL on failure.
 */
int visual_ui_entry_set_length (VisUIEntry *entry, int length)
{
	visual_log_return_val_if_fail (entry != NULL, -VISUAL_ERROR_UI_ENTRY_NULL);

	entry->length = length;

	return VISUAL_OK;
}

/**
 * Creates a new VisUISlider, which can be used as a VisUIRange type, in the form of a slider.
 *
 * @param showvalue Show the value of the slider place.
 * 
 * @return The newly created VisUISlider in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_slider_new (int showvalue)
{
	VisUISlider *slider;

	slider = visual_mem_new0 (VisUISlider, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (slider), TRUE, widget_dtor);

	VISUAL_UI_WIDGET (slider)->type = VISUAL_WIDGET_TYPE_SLIDER;

	slider->showvalue = showvalue;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (slider), -1, -1);

	return VISUAL_UI_WIDGET (slider);
}

/**
 * Creates a new VisUINumerici, which can be used as a VisUIRange type, in the form of a numeric spinbutton.
 *
 * @return The newly created VisUINumeric in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_numeric_new ()
{
	VisUINumeric *numeric;

	numeric = visual_mem_new0 (VisUINumeric, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (numeric), TRUE, widget_dtor);

	VISUAL_UI_WIDGET (numeric)->type = VISUAL_WIDGET_TYPE_NUMERIC;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (numeric), -1, -1);

	return VISUAL_UI_WIDGET (numeric);
}

/**
 * Creates a new VisUIColor, which can be used to select a color.
 *
 * @return The newly created VisUIColor in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_color_new ()
{
	VisUIColor *color;

	color = visual_mem_new0 (VisUIColor, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (color), TRUE, widget_dtor);
	
	VISUAL_UI_WIDGET (color)->type = VISUAL_WIDGET_TYPE_COLOR;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (color), -1, -1);

	return VISUAL_UI_WIDGET (color);
}

/**
 * Creates a new VisUIChoiceEntry, this is an entry in an entry in VisUIChoiceList, that is used
 * by the different VisUIChoice inherentence.
 *
 * @param name The name ofe the choice.
 * @param value The VisParamEntry associated with this choice.
 *
 * @return The newly created VisUIChoiceEntry.
 */
VisUIChoiceEntry *visual_ui_choice_entry_new (const char *name, VisParamEntry *value)
{
	VisUIChoiceEntry *centry;

	visual_log_return_val_if_fail (name != NULL, NULL);
	visual_log_return_val_if_fail (value != NULL, NULL);

	centry = visual_mem_new0 (VisUIChoiceEntry, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (centry), TRUE, NULL);
	
	centry->name = name;
	centry->value = value;

	return centry;
}

/**
 * Add a VisParamEntry as a choice for a VisUIWidget that inherents from VisUIChoice.
 *
 * @param choice Pointer to the VisUIChoice widget to which the choice is added.
 * @param name The name of this choice.
 * @param value Pointer to the VisParamEntry that is associated with this choice.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_CHOICE_NULL, -VISUAL_ERROR_NULL
 *	or -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_ui_choice_add (VisUIChoice *choice, const char *name, VisParamEntry *value)
{
	VisUIChoiceEntry *centry;

	visual_log_return_val_if_fail (choice != NULL, -VISUAL_ERROR_UI_CHOICE_NULL);
	visual_log_return_val_if_fail (name != NULL, -VISUAL_ERROR_NULL);
	visual_log_return_val_if_fail (value != NULL, -VISUAL_ERROR_PARAM_NULL);

	centry = visual_ui_choice_entry_new (name, value);

	choice->choices.count++;

	visual_list_add (&choice->choices.choices, centry);

	return VISUAL_OK;
}

/**
 * Add many choices using a VisParamEntry array.
 *
 * @see visual_ui_choice_add
 *
 * @param choice Pointer to the VisUIChoice widget to which the choices are added.
 * @param paramchoices Pointer to the array of VisParamEntries that are added to the internal
 *	VisUIChoiceList of the VisUIChoice widget.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_CHOICE_NULL or -VISUAL_ERROR_PARAM_NULL on failure.
 */
int visual_ui_choice_add_many (VisUIChoice *choice, VisParamEntry *paramchoices)
{
	VisUIChoiceEntry *centry;
	int i = 0;

	visual_log_return_val_if_fail (choice != NULL, -VISUAL_ERROR_UI_CHOICE_NULL);
	visual_log_return_val_if_fail (paramchoices != NULL, -VISUAL_ERROR_PARAM_NULL);

	while (paramchoices[i].type != VISUAL_PARAM_ENTRY_TYPE_END) {
		visual_ui_choice_add (choice, paramchoices[i].name, &paramchoices[i]);

		i++;
	}

	return VISUAL_OK;
}

/**
 * Frees all the memory used by the VisUIChoiceList and it's entries for the given VisUIChoice.
 *
 * @param choice Pointer to the VisUIChoice for which the VisUIChoiceList and it's entries
 *	are being freed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_CHOICE_NULL on failure.
 */
int visual_ui_choice_free_choices (VisUIChoice *choice)
{
	visual_log_return_val_if_fail (choice != NULL, -VISUAL_ERROR_UI_CHOICE_NULL);

	visual_list_set_destroyer (&choice->choices.choices, visual_object_list_destroyer);
	visual_list_destroy_elements (&choice->choices.choices); 

	return VISUAL_OK;
}

/**
 * Sets the active choice for a VisUIChoice widget.
 *
 * @param choice Pointer to the VisUIChoice widget for which an entry is being activated.
 * @param index Index containing which choice needs to be activated, counting starts at 0.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_UI_CHOICE_NULL, -VISUAL_ERROR_UI_CHOICE_ENTRY_NULL or
 *	error values returned by visual_ui_mutator_get_param () on failure.
 */
int visual_ui_choice_set_active (VisUIChoice *choice, int index)
{
	VisUIChoiceEntry *centry;
	VisParamEntry *param;
	VisParamEntry *newparam;

	visual_log_return_val_if_fail (choice != NULL, -VISUAL_ERROR_UI_CHOICE_NULL);

	centry = visual_ui_choice_get_choice (choice, index);
	visual_log_return_val_if_fail (centry != NULL, -VISUAL_ERROR_UI_CHOICE_ENTRY_NULL);

	param = visual_ui_mutator_get_param (VISUAL_UI_MUTATOR (choice));

	newparam = (VisParamEntry *) centry->value;

	return visual_param_entry_set_from_param (param, newparam);
}

/**
 * Retrieves the index of the current active choice.
 *
 * @param choice Pointer to the VisUIChoice widget from which the active choice is requested.
 *
 * @return Index of the active choice entry.
 */
int visual_ui_choice_get_active (VisUIChoice *choice)
{
	VisListEntry *le = NULL;
	VisUIChoiceEntry *centry;
	VisParamEntry *param;
	int i = 0;

	visual_log_return_val_if_fail (choice != NULL, -VISUAL_ERROR_UI_CHOICE_NULL);

	param = visual_ui_mutator_get_param (VISUAL_UI_MUTATOR (choice));

	while ((centry = visual_list_next (&choice->choices.choices, &le)) != NULL) {
		VisParamEntry *cparam;
		
		cparam = centry->value;

		if (visual_param_entry_compare (param, cparam) == TRUE)
			return i;

		i++;
	}

	return -VISUAL_ERROR_UI_CHOICE_NONE_ACTIVE;
}

/**
 * Retrieves a VisUIChoiceEntry from a VisUIChoice based on the index.
 *
 * @param choice Pointer to the VisUIChoice widget from which a VisUIChoiceEntry is requested.
 * @param index Index of the requested VisUIChoiceEntry.
 *
 * @return The VisUIChoiceEntry that is located on the given index, or NULL on failure.
 */
VisUIChoiceEntry *visual_ui_choice_get_choice (VisUIChoice *choice, int index)
{
	visual_log_return_val_if_fail (choice != NULL, NULL);

	return visual_list_get (&choice->choices.choices, index);
}

/**
 * Retrvies the VisUIChoiceList from a VisUIChoice widget.
 *
 * @param choice Pointer to the VisUIChoice widget from which the VisUIChoiceList is requested.
 * 
 * @return The VisUIChoiceList that is associated to the VisUIChoice.
 */
VisUIChoiceList *visual_ui_choice_get_choices (VisUIChoice *choice)
{
	visual_log_return_val_if_fail (choice != NULL, NULL);

	return &choice->choices;
}

/**
 * Creates a new VisUIPopup. This can be used to show a popup list containing items from which
 * can be choosen.
 *
 * @return The newly created VisUIPopup in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_popup_new ()
{
	VisUIPopup *popup;

	popup = visual_mem_new0 (VisUIPopup, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (popup), TRUE, choice_dtor);
	
	VISUAL_UI_WIDGET (popup)->type = VISUAL_WIDGET_TYPE_POPUP;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (popup), -1, -1);

	return VISUAL_UI_WIDGET (popup);
}

/**
 * Creates a new VisUIList. This can be used to show a list containing items from which
 * can be choosen.
 *
 * @return The newly created VisUIList in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_list_new ()
{
	VisUIList *list;

	list = visual_mem_new0 (VisUIList, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (list), TRUE, choice_dtor);
	
	VISUAL_UI_WIDGET (list)->type = VISUAL_WIDGET_TYPE_LIST;

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (list), -1, -1);

	return VISUAL_UI_WIDGET (list);
}

/**
 * Creates a new VisUIRadio. This can be used to show a list of radio buttons containing items from which
 * can be choosen.
 *
 * @param orient The orientation of the radio button layout.
 * 
 * @return The newly created VisUIRadio in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_radio_new (VisUIOrientType orient)
{
	VisUIRadio *radio;

	radio = visual_mem_new0 (VisUIRadio, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (radio), TRUE, choice_dtor);

	VISUAL_UI_WIDGET (radio)->type = VISUAL_WIDGET_TYPE_RADIO;

	radio->orient = orient;
	
	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (radio), -1, -1);

	return VISUAL_UI_WIDGET (radio);
}

/**
 * Creates a new VisUICheckbox. This can be used to show a checkbox that can be toggled and untoggled.
 *
 * @param name The text behind the checkbox.
 * @param boolcheck Automaticly set a boolean, TRUE and FALSE VisUIChoiceList on the VisUICheckbox.
 *
 * @return The newly created VisUICheckbox in the form of a VisUIWidget.
 */
VisUIWidget *visual_ui_checkbox_new (const char *name, int boolcheck)
{
	VisUICheckbox *checkbox;
	static VisParamEntry truefalse[] = {
		VISUAL_PARAM_LIST_ENTRY_INTEGER ("false",	FALSE),
		VISUAL_PARAM_LIST_ENTRY_INTEGER ("true",	TRUE),
		VISUAL_PARAM_LIST_END
	};

	checkbox = visual_mem_new0 (VisUICheckbox, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (checkbox), TRUE, choice_dtor);

	VISUAL_UI_WIDGET (checkbox)->type = VISUAL_WIDGET_TYPE_CHECKBOX;

	checkbox->name = name;

	/* Boolean checkbox, generate a FALSE, TRUE choicelist ourself */
	if (boolcheck == TRUE)
		visual_ui_choice_add_many (VISUAL_UI_CHOICE (checkbox), truefalse);

	visual_ui_widget_set_size_request (VISUAL_UI_WIDGET (checkbox), -1, -1);

	return VISUAL_UI_WIDGET (checkbox);
}

/**
 * @}
 */

