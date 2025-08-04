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

#include "inishell.h"
#include "src/gui_elements/gui_elements.h"
#include "src/main/colors.h"

#ifdef DEBUG
	#include <iostream>
#endif

/**
 * @class PropertyWatcher
 * @brief This class listens to changes of the INI value from anywhere in the program.
 * @details INIshell uses Qt's property system to tell a panel that the INI value has
 * been modified. When the PropertyWatcher detects a change in the "ini_value" property,
 * the changedValue() signal is emitted to Atomic, which in turn delegates the signal
 * to the specific panel's onPropertySet() function.
 * This is used for changes in the INI value from "outside" to tell the panel that it
 * should modify the displayed value. Hence, it is not used when a value is changed
 * through user interaction (because it is already being displayed).
 * @param parent The object's parent.
 */
PropertyWatcher::PropertyWatcher(QObject *parent) : QObject(parent)
{
	//do nothing
}

/**
 * @brief Event filter of the PropertyWatcher.
 * @details This PropertyWatcher only listens to changes in the "ini_value" property.
 * @param[in] object The object for which an event has occurred.
 * @param[in] event The event that has occurred.
 * @return True if the event is being accepted.
 */
bool PropertyWatcher::eventFilter(QObject *object, QEvent *event) {
	//only act on changes to the "ini_value" property:
	if(event->type() == QEvent::DynamicPropertyChange) {
		auto *const propEvent = static_cast<QDynamicPropertyChangeEvent *>(event);
		const QString property_name(propEvent->propertyName().data());
		if (property_name == "ini_value")
			emit changedValue(); //changedValue will be connected to a function in Atomic
	}
	return QObject::eventFilter(object, event);
}

/**
 * @brief Retrieve a pointer to the main window for member access etc.
 * @return Pointer to the main window.
 */
MainWindow * getMainWindow()
{
	static MainWindow *inishell_main;
	if (inishell_main) //already found before
		return inishell_main;

	const QWidgetList topLevel( QApplication::topLevelWidgets() );
	for (QWidget *widget : topLevel) {
		inishell_main = qobject_cast<MainWindow*>(widget);
		if (inishell_main) //cast successful - right window
			return inishell_main;
	}
	return nullptr;
}

/**
 * @brief Recursively build the interface.
 * @details This function traverses the XML tree and builds input/output panels into a grouping element.
 * At the top level, this grouping element is held by the scroll panels (one per tab). Panels are
 * added there, and these panels can themselves own groups, which again can be built in.
 * @param[in] parent_node The parent XML node. This corresponds to the parent node/group and
 * therefore holds all options for the parent Group, and all info about children that should be built.
 * @param[in] parent_group The Group to build in. If empty, it will be created in the main tab.
 * @param[in] parent_section The current section. If omitted, the parent section is chosen.
 * @param[in] no_spacers The parent group requests to save space and build a tight layout.
 */
void recursiveBuild(const QDomNode &parent_node, Group *parent_group, const QString &parent_section, const bool &no_spacers)
{
	/* run through all child nodes of the current level */
	for (QDomNode current_node = parent_node.firstChildElement(); !current_node.isNull(); current_node = current_node.nextSibling()) {

		/* read some attributes */
		QDomElement current_element( current_node.toElement() );
		const QString key( current_element.attribute("key") ); //INI key
		const QString element_type( current_element.tagName() ); //identifier for the node's purpose
		//from here we build frames and parameter panels; everything else (e. g. options) is done elsewhere:
		if (element_type != "frame" && element_type != "parameter" && element_type != "section")
			continue;

		/* read requested section from a number of different places in the XML */
		if (parent_group == nullptr && element_type == "section") { //dedicated <section> node
			recursiveBuild(current_node, parent_group, current_node.toElement().attribute("name"));
			continue;
		}
		QStringList section_list;
		const bool has_section = parseAvailableSections(current_element, parent_section, section_list);
		if (!has_section)
			continue;

		/* following is the actual recursion building frames and panels */
		for (auto &current_section : section_list) {
			/* read some attributes of the current node */
			Group *group_to_add_to(parent_group);
			if (group_to_add_to == nullptr) { //top level -> find or if necessary create tab
				QString tab_background_color(current_element.firstChildElement("section").attribute("background_color"));
				QString tab_font_color(current_element.firstChildElement("section").attribute("color"));
				if (current_element.parentNode().toElement().tagName() == "section") {
					//colors for dedicated section tabs have not been parsed before, do it now:
					if (tab_background_color.isEmpty())
						tab_background_color = parent_node.toElement().attribute("background_color");
					if (tab_font_color.isEmpty())
					tab_font_color = parent_node.toElement().attribute("color");
				}
				group_to_add_to = getMainWindow()->getControlPanel()->
				    getSectionScrollarea(current_section, tab_background_color, tab_font_color)->getGroup();
			}
			if (element_type == "frame") { //visual grouping by a frame with title
				const QString frame_title(current_element.attribute("caption"));
				const QString frame_color(current_element.attribute("color"));
				const QString frame_background_color(current_element.attribute("background_color"));
				/* construct new group with border and title for the frame */
				Group *frame = new Group(current_section, key, true, false, true, false,
				    frame_title, frame_color, frame_background_color);
				group_to_add_to->addWidget(frame);
				recursiveBuild(current_node, frame, current_section); //all children go into the frame
			} else if (element_type == "parameter") { //a panel
				if (current_element.attribute("template").toLower() == "true") //Selector panel will handle this
					continue;
				/* build the desired object, add it to the parent group, and recursively build its children */
				QWidget *new_element = elementFactory(current_element.attribute("type"), current_section, key,
				    current_node, no_spacers);
				if (new_element != nullptr) {
					group_to_add_to->addWidget(new_element);
					recursiveBuild(current_node, group_to_add_to, current_section, no_spacers);
				}
			} //endif element type
		} //endfor section_list
		//TODO: respect multiple colors if multiple sections are given
	} //endfor current_node
}

/**
 * @brief Helper function to retrieve the section(s) that were set via XML.
 * @details They can be set via a parent <section> node (handled outside), <section>
 * tags within the panel, and <section> tags in child elements like Alternative panel items.
 * @param[in] current_element
 * @param[in] parent_section
 * @param[out] section_list List of found sections.
 * @return True if a section was found that matches the parent section, hinting that the element
 * should be built for this section. False if the element should be ignored for the current section.
 */
bool parseAvailableSections(const QDomElement &current_element, const QString &parent_section, QStringList &section_list)
{
	/*
	 * The following reads a list of all sections given for the parameter. At the top
	 * level, a panel is built for each section. Descending down children can not
	 * switch parents, and the given sections are checked against the parent. If one
	 * of the sections given matches the parent the panel is built.
	 * (This way there can be collections of parameters contributing to multiple
	 * sections, where the individual panels can be excluded from some sections.)
	 */

	for (QDomNode sec_node = current_element.firstChildElement("section"); !sec_node.isNull();
	    sec_node = sec_node.nextSiblingElement("section")) //read all <section> tags
		section_list.push_back(sec_node.toElement().attribute("name"));
	if (section_list.isEmpty()) { //check for section given in attributes, else pick default:
		if (!current_element.attribute("section").isNull())
			section_list.push_back(current_element.attribute("section"));
		else
			section_list.push_back(parent_section.isNull()? Cst::default_section : parent_section);
	}
	if (!parent_section.isNull()) { //not at top level - the parent is fixed
		if (!section_list.isEmpty() && !section_list.contains(parent_section, Qt::CaseInsensitive))
			return false; //sections are given, but they don't match the parent
		section_list.clear(); //don't build multiple times
		section_list.push_back(parent_section);
	}
	return true;
}

/**
 * @brief Access to logging function from parent-less objects.
 * @details If the main logger is not stored in a class, this function can be used to access
 * the logging window anyway.
 * @param[in] message The message to log.
 * @param[in] color Color of the log message.
 */
void topLog(const QString &message, const QString &color)
{
	MainWindow *inishell_main = getMainWindow();
	if(inishell_main != nullptr)
		inishell_main->log(message, color);
}

/**
 * @brief Access to the status bar from parent-less objects.
 * @details If a module does not have access to the main status bar, this function can be used to
 * display status messages anyway.
 * @param[in] message The status message to display.
 * @param[in] color Color of the status message.
 * @param[in] status_light True to enable the "busy" status light, false to disable it.
 * @param[in] time Time span to display the status message for.
 */
void topStatus(const QString &message, const QString &color, const bool &status_light,
    const int &time)
{
	MainWindow *inishell_main = getMainWindow();
	if(inishell_main != nullptr)
		inishell_main->setStatus(message, color, status_light, time);
}
