/*****************************************************************************/
/*  Copyright 2019 WSL Institute for Snow and Avalanche Research  SLF-DAVOS  */
/*****************************************************************************/
/* This file is part of INIshell.
   INIshell is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   INIshell is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with INIshell.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * This file provides constants across INIshell. Most hardcoded values can be changed
 * from this central point.
 * 2019-10
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Cst {

	/* main window */
	static constexpr int width_inishell_default = 1400;
	static constexpr int height_inishell_default = 800;
	static constexpr int width_inishell_min = 800; //window size limits
	static constexpr int height_inishell_min = 400;
	static constexpr int proportion_workflow_horizontal_percent = 30; //size of the left list
	static constexpr int width_workflow_max = 500;

	/* settings dialog window */
	static constexpr int width_settings = 600;
	static constexpr int height_settings = 400;
	static constexpr int settings_navigation_spacing = 20;
	static constexpr int settings_navigation_maxheight = 100;

	/* logger window */
	static constexpr int width_logger_default_ = 600;
	static constexpr int height_logger_default = 300;
	static constexpr int width_logger_min_ = 400;
	static constexpr int height_logger_min = 200;

	/* preview window */
	static constexpr int width_preview_default = 500;
	static constexpr int height_preview_default = 700;

	/* help windows */
	static constexpr int width_help_about = 500;
	static constexpr int height_help_about = 300;

	/* widgets */
	static constexpr int tiny = 100; //minimum width of (input) widgets, just to look less cramped
	static constexpr int width_button_std = 25; //standard width of push button
	static constexpr int height_button_std = 25;

	/* panels */
	static constexpr int nr_items_visible = 5; //CHECKLIST panel
	static constexpr int width_checklist_max = 300;
	static constexpr int checklist_safety_padding_vertical = 3;
	static constexpr int checklist_safety_padding_horizontal = 50; //space for checkboxes
	static constexpr int width_dropdown_max = 300; //DROPDOWN panel
	static constexpr int dropdown_safety_padding = 30;
	static constexpr int width_filepath_min = tiny * 2; //FILEPATH panel
	static constexpr int frame_left_margin = 20; //FRAME panel
	static constexpr int frame_right_margin = frame_left_margin;
	static constexpr int frame_top_margin = 25; //top is higher to account for caption
	static constexpr int frame_bottom_margin = 20;
	static constexpr int width_help = 600; //HELPTEXT panel
	static constexpr int width_label = 220; //LABEL panel
	static constexpr int width_long_label = 320; //to prettify lists of longer options
	static constexpr int label_padding = 20; //safety margins to really display all text
	static constexpr int width_number_min = 125; //NUMBER panel
	static constexpr int width_textbox_medium = 200; //TEXTFIELD panel
	static constexpr int default_spacer_size = 40; //SPACER panel

	/* workflow panel */
	static constexpr int treeview_indentation_ = 15;

	/* static strings */
	static const QString settings_file_name = "inishell_config.xml";
	static const QString sep = "::"; //do a grep if this is changed -- it's hardcoded in some messages
	static const QString section_open = "["; //check all RegularExpressions if this is changed
	static const QString section_close = "]";
	static const QString default_section("General");

	/* time */
	static constexpr int msg_length = 5000; //default ms for toolbar messages
	static constexpr int msg_short_length = 3000;

} //end namespace

#endif //CONSTANTS_H
