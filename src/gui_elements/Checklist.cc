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

#include "Checklist.h"
#include "Label.h"
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/inishell.h"

#include <QStringList>

/**
 * @class Checklist
 * @brief Default constructor for a Checklist panel.
 * @details A Checklist shows a checkable list where each list item can have arbitrary children
 * which are displayed below if the item is checked and hidden if it's not.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Checklist.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Checklist::Checklist(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* label, list, and container for child widgets */
	auto *key_label( new Label(QString(), QString(), options, no_spacers, key_, this) );
	list_ = new QListWidget;
	setPrimaryWidget(list_);
	connect(list_, &QListWidget::itemClicked, this, &Checklist::listClick);
	child_container_ = new Group(QString(), QString(), false, false, false, true); //tight layout
	child_container_->setVisible(false); //container for children activates only when clicked (saves space)

	/* layout of the basic elements */
	checklist_layout_ = new QVBoxLayout;
	checklist_layout_->setContentsMargins(0, 0, 0, 0);
	checklist_layout_->addWidget(list_);
	checklist_layout_->addWidget(child_container_);

	/* layout of basic elements plus checklist */
	auto *layout( new QHBoxLayout );
	setLayoutMargins(layout);
	layout->addWidget(key_label);
	layout->addLayout(checklist_layout_);
	if (!no_spacers)
		layout->addSpacerItem(buildSpacer());
	main_help_ = addHelp(layout, options, no_spacers, true); //force to add a Helptext for access by subitems
	main_help_->setProperty("main_help", options.firstChildElement("help").text());
	this->setLayout(layout);

	setOptions(options); //fill list items

	if (main_help_->property("main_help").isNull() && !has_child_helptexts_)
		main_help_->hide(); //no main help and no child panel help --> save space
}

/**
 * @brief Parse options for a Checklist from XML.
 * @param[in] options XML node holding the Checklist.
 */
void Checklist::setOptions(const QDomNode &options)
{
	QStringList item_strings;
	for (QDomElement op = options.firstChildElement(); !op.isNull(); op = op.nextSiblingElement()) {
		if (op.tagName() != "option" && op.tagName() != "o")
			continue; //short <o></o> notation possible
		if (!hasSectionSpecified(section_, op)) //option is excluded for this section
			continue;
		substituteKeys(op, "@", key_);
		//add the option value as list item:
		const QString value( op.attribute("value") );
		auto *item( new QListWidgetItem(value, list_) );

		/* set item properties */
		item_strings.push_back(op.attribute("value"));
		item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable); //not checkable because we do it on row click
		item->setCheckState(Qt::Unchecked);
		item->setToolTip(op.attribute("help"));
		if (!op.firstChildElement("help").isNull()) {
			item->setData(Qt::UserRole, op.firstChildElement("help").text());
			has_child_helptexts_ = true;
		}
		if (!op.attribute("color").isEmpty())
			item->setForeground(colors::getQColor(op.attribute("color")));
		item->setFont(setFontOptions(item->font(), op));

		//group all elements of this option together with a tight layout:
		auto *item_group( new Group(section_, "_checklist_itemgroup_" + key_, false, false, false, true) );
		//construct the children of this option:
		recursiveBuild(op, item_group, section_);
		child_container_->addWidget(item_group);
		item_group->setVisible(false); //becomes visible when checked

		if (op.attribute("default").toLower() == "true") {
			const QString def_val( this->property("default_value").toString() );
			this->setProperty("default_value", (def_val.isEmpty()? "" : def_val + " ") + value);
			list_->setCurrentRow(list_->count() - 1);
			emit listClick(list_->item(list_->count() - 1)); //to set the default value
		}
	}

	if (list_->count() == 0) {
		topLog(QString(tr(R"(Invalid XML syntax for Checklist panel "%1::%2": no checkable options set.)").arg(
		    section_, key_)), "error");
		list_->setVisible(false);
		return;
	}

	//this many rows should be visible (with some padding):
	list_->setFixedHeight(list_->sizeHintForRow(0) * qMin(Cst::nr_items_visible, list_->count()) +
	    Cst::checklist_safety_padding_vertical);
	list_->setMinimumWidth(getElementTextWidth(item_strings, Cst::tiny, Cst::width_checklist_max) +
	    Cst::checklist_safety_padding_horizontal);
	list_->setMaximumWidth(Cst::width_checklist_max);
}

/**
 * @brief Collect Checklist texts in the order they were checked in.
 * @return INI value with ordered texts.
 */
QString Checklist::getOrderedIniList() const
{
	QString list_values;
	for (auto &it : ordered_item_list_)
		list_values += it->text() + " "; //collect values as "value1 value2 value3..."
	list_values.chop(1); //trailing space (if available)
	return list_values;
}

/**
 * @brief Set the child containers' visibilities.
 * @param[in] index Index of the checkbox that is being clicked.
 * @param[in] checked State of the checkbox that is being clicked.
 */
void Checklist::setChildVisibility(QListWidgetItem *item)
{
	const QLayout *group_layout( child_container_->getLayout() );
	auto *item_group( static_cast<Group *>(group_layout->itemAt(list_->currentRow())->widget()) );

	bool one_visible = false;
	bool all_unchecked = true;
	for (int ii = 0; ii < list_->count(); ++ii) {
		if (list_->item(ii)->checkState() == Qt::Checked) {
			all_unchecked = false;
			if (!static_cast<Group *>(group_layout->itemAt(ii)->widget())->isEmpty()) {
				one_visible = true; //hide container if empty (a few blank pixels)
				break;
			}
		}
	}
	child_container_->setVisible(one_visible);
	if (all_unchecked) //switch back to main help:
		main_help_->updateText(main_help_->property("main_help").toString());

	item_group->setVisible(item->checkState() == Qt::Checked && !item_group->isEmpty());
}

/**
 * @brief Event handler for clicks in the list widget.
 * @details This handles showing and hiding children belonging to list items.
 * @param[in] item Item that was clicked.
 */
void Checklist::listClick(QListWidgetItem *item)
{
	//click in line is enough to check/uncheck:
	if (item->checkState() == Qt::Unchecked) { //now it's CHECKED
		item->setCheckState(Qt::Checked);
		const QString item_help( item->data(Qt::UserRole).toString() );
		main_help_->updateText( //switch main help to item help if available
		    item_help.isEmpty()? main_help_->property("main_help").toString() : item_help);
		ordered_item_list_.push_back(item); //remember checking order
	} else { //now it's UNCHECKED
		setUpdatesEnabled(false); //we only experience flickering when hiding
		item->setCheckState(Qt::Unchecked);
		auto erase_it( std::find(ordered_item_list_.begin(), ordered_item_list_.end(), item) );
		if (erase_it != ordered_item_list_.end())
			ordered_item_list_.erase(erase_it);
	}
	setChildVisibility(item);

	const QString list_values( getOrderedIniList() );
	setDefaultPanelStyles(list_values);
	setIniValue(list_values);
	this->setToolTip(key_ + " = " + list_values); //show sorting to the user
	primary_widget_->setToolTip(this->toolTip());
	setBufferedUpdatesEnabled();
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void Checklist::onPropertySet()
{
	const QString values( this->property("ini_value").toString() );
	if (ini_value_ == values)
		return;
	const QStringList value_list( values.split(QRegExp("\\s+"), QString::SkipEmptyParts) );

	/* clear the list, overwriting current settings */
	for (int ii = 0; ii < list_->count(); ++ii) {
		if (list_->item(ii)->checkState() == Qt::Checked) {
			list_->setCurrentRow(ii);
			emit listClick(list_->item(ii)); //unload child panels
		}
	}

	/* check all items found in the given value */
	for (auto &val : value_list) {
		//Note: since we find and click the checkbox for each value they are correctly inserted into the ordering
		bool found_option_to_set = false;
		for (int jj = 0; jj < list_->count(); ++jj) {
			if (QString::compare(list_->item(jj)->text(), val, Qt::CaseInsensitive) == 0) {
				list_->setCurrentRow(jj); //else we will get NULL when we fetch the item
				emit listClick(list_->item(jj)); //to set the default value
				found_option_to_set = true;
				break;
			}
		} //endfor jj
		if (!found_option_to_set)
			topLog(tr(R"(Checklist item "%1" could not be set from INI file for key "%2::%3": no such option specified in XML file)").arg(
			    val, section_, key_), "warning");
	} //endfor ii
}
