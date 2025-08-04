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

#include "Group.h"
#include "src/main/colors.h"

#ifdef DEBUG
	#include <QDebug>
#endif //def DEBUG

/**
 * @class Group
 * @brief Default Group constructor
 * @details Group is the central grouping element which the GUI building recursion works on.
 * Essentially it is a wrapper for QGroupBox.
 * The default mode is an invisible grouping element, but it can also show a light border as well
 * as a thick one to act as a frame. Additionally, it can be switched to a grid layout instead of
 * a vertical one.
 * The main program tab holds Groups in which the panels go. If the panels feature
 * child elements themselves they too own a group in which they are put.
 * @param[in] section Name of the INI section the group corresponds to. This could be used
 * to style the groups differently depending on the section, like it is already shown for coloring.
 * This parameter does not affect the childrens' settings.
 * @param[in] key (INI) key for the group. Does not affect the childrens' keys.
 * @param[in] has_border Optional border around the group.
 * @param[in] grid_layout Elements are not placed vertically (default) but in a grid layout.
 * @param[in] is_frame Act as a frame with border and title.
 * @param[in] caption If the group is a frame the caption is displayed as title.
 * @param[in] parent The parent widget.
 */
Group::Group(const QString &section, const QString &key, const bool &has_border,
	    const bool &grid_layout, const bool &is_frame, const bool &tight, const QString &caption,
	    const QString &in_frame_color, const QString &background_color, QWidget *parent)
	    : Atomic(section, key, parent)
{
	//Groups have an identifier via section/key, but they don't stand for an actual INI value:
	this->property("no_ini") = true; //panel is to be ignored on output
	box_ = new QGroupBox(is_frame? caption : QString()); //all children go here, only frame can have a title
	setPrimaryWidget(box_);
	if (grid_layout)
		layout_ = new QGridLayout; //mainly for the grid panel
	else
		layout_ = new QVBoxLayout; //frame is always this
	box_->setLayout(layout_); //we can add widgets at any time

	QString stylesheet; //set title within box instead of above, colors, rounded borders, frame font
	if (!is_frame) { //normal group
		if (has_border) {
			stylesheet = "QGroupBox#_primary_" + getQtKey(getId()) + " {border: 1px solid " +
			    colors::getQColor("groupborder").name() + "; border-radius: 6px";
			setLayoutMargins(layout_);
		} else {
			stylesheet = "QGroupBox#_primary_" + getQtKey(getId()) + " {border: none; margin-top: 0px";
			layout_->setContentsMargins(5, 5, 5, 5);
		}
	} else { //it's a frame
		QString frame_color( in_frame_color ); //pick default frame and frame title color if not given
		if (frame_color.isNull())
			frame_color = colors::getQColor("frameborder").name();
		else
			frame_color = colors::getQColor(frame_color).name();
		stylesheet = "QGroupBox::title#_primary_" + getQtKey(getId()) +
		    " {subcontrol-origin: margin; left: 17px; padding: 0px 5px 0px 5px}" +
		    "QGroupBox#_primary_" + getQtKey(getId()) + "  {border: 2px solid " +
		    frame_color + "; border-radius: 6px; margin-top: 8px; color: " + frame_color;
		layout_->setContentsMargins(Cst::frame_left_margin, Cst::frame_top_margin, //a little room for the frame
		    Cst::frame_right_margin, Cst::frame_bottom_margin);
	}
	//QTBUG: Note that there's a bug in the "Fusion" GTK style which does not hide empty QGroupBox titles,
	//hence the coloring may extend above a little if we're not careful.
	if (!background_color.isNull())
		stylesheet += "; background-color: " + colors::getQColor(background_color).name();
	stylesheet += "}";
	box_->setStyleSheet(stylesheet);
	if (tight)
		layout_->setContentsMargins(0, 0, 0, 0);

	auto *main_layout( new QVBoxLayout ); //layout for the derived class to assign the box_ to it
	setLayoutMargins(main_layout); //tighter placements
	main_layout->addWidget(box_, 0, Qt::AlignTop); //alignment important when we hide/show children
	this->setLayout(main_layout);
	this->setProperty("no_ini", true); //a Group is Atomic but can never contribute values by itself
}

/**
 * @brief Add a child widget to the group's vertical layout.
 * @param[in] widget The widget to add.
 */
void Group::addWidget(QWidget *widget)
{
	auto *layout( qobject_cast<QVBoxLayout *>(layout_) );
#ifdef DEBUG
	if (!layout) {
		qDebug() << "Casting a QLayout to QVBoxLayout failed in Group::addWidget() when it shouldn't have.";
		return;
	}
#endif //def DEBUG
	layout->addWidget(widget, 0, Qt::AlignTop); //alignment needed after we hide/show groups
}

/**
 * @brief Add a child widget to the group's grid layout.
 * @details This is called by the GridPanel to add widgets in a raster.
 * @param[in] widget The widget to add.
 * @param[in] row Row position of the widget (starts at 0).
 * @param[in] column Column position of the widget (starts at 0).
 * @param[in] rowSpan The widget spans this many rows (default: 1).
 * @param[in] columnSpan The widget spans this many columns (default: 1).
 * @param[in] alignment Alignment of the widget within the raster point.
 */
void Group::addWidget(QWidget *widget, int row, int column, int rowSpan, int columnSpan,
    Qt::Alignment alignment)
{
	auto *layout( qobject_cast<QGridLayout *>(layout_) );
#ifdef DEBUG
	if (!layout) {
		qDebug() << "Casting a QLayout to QGridLayout failed in Group::addWidget() when it shouldn't have.";
		return;
	}
#endif //def DEBUG
	layout->addWidget(widget, row, column, rowSpan, columnSpan, alignment);
}

/**
 * @brief Delete all child panels of the group.
 * @details The group itself is not deleted, but the container is rendered useless.
 */
void Group::erase()
{
	setUpdatesEnabled(false); //disable painting in case we have a lot of content
	delete box_;
	setBufferedUpdatesEnabled();
}

/**
 * @brief Retrieve the number of child panels.
 * @return The number of child panels.
 */
int Group::count() const
{
	return (this->getLayout()->count());
}

/**
 * @brief Check if the group has child panels.
 * @return True if there is at least one child.
 */
bool Group::isEmpty() const
{
	return (this->count() == 0);
}
