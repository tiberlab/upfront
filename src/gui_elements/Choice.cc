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

#include "Choice.h"
#include "Label.h"
#include "src/main/colors.h"
#include "src/main/inishell.h"

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class Choice
 * @brief Default constructor for a Choice panel.
 * @details A choice panel shows a list of checkboxes, each of which controls showing/hiding of
 * additional options.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Choice panel.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Choice::Choice(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	const QDomElement element(options.toElement());

	//grouping element of the list of checkboxes:
	checkbox_container_ = new Group(QString(), QString(), false, true, false, true); //tight grid layout
	setPrimaryWidget(checkbox_container_, true, true); //no styling since we style the checkboxes
	//grouping element for all children:
	child_container_ = new Group(QString(), QString()); //vertical layout
	child_container_->setVisible(false);
	auto *key_label = new Label(QString(), QString(), options, no_spacers, key_, this);

	/* layout for checkboxes and children together */
	auto *box_layout = new QVBoxLayout;
	box_layout->setContentsMargins(0, 0, 0, 0);
	box_layout->addWidget(checkbox_container_);
	box_layout->addWidget(child_container_);

	auto *layout = new QHBoxLayout;
	setLayoutMargins(layout);
	layout->addWidget(key_label);
	layout->addLayout(box_layout);
	addHelp(layout, options);
	this->setLayout(layout);

	setOptions(options); //build children
}

/**
 * @brief Check if the Choice value is mandatory or currently at the default value.
 * @details Usually this is handled in the Atomic base class, but here we want to color
 * the individual checkboxes instead of the whole group that is the primary widget.
 * The primary widget is still needed to dispatch INI value changes.
 * @param[in] in_value The current value of the panel.
 */
void Choice::setDefaultPanelStyles(const QString &in_value)
{
	const QList<QCheckBox *> cb_list( checkbox_container_->findChildren<QCheckBox *>() );
	const bool is_default = (QString::compare(in_value, this->property("default_value").toString(), Qt::CaseInsensitive) == 0);
	for (auto &cb : cb_list) {
		setPanelStyle(FAULTY, false, cb); //first we disable temporary styles
		setPanelStyle(VALID, false, cb);
		setPanelStyle(DEFAULT, is_default && !in_value.isNull() && !this->property("default_value").isNull(), cb);
		if (this->property("is_mandatory").toBool())
			setPanelStyle(MANDATORY, in_value.isEmpty(), cb);
	}
}

/**
 * @brief Parse options for a Choice panel from XML.
 * @param[in] options XML node holding the Choice panel.
 */
void Choice::setOptions(const QDomNode &options)
{
	int counter = 0;
	for (QDomElement op = options.firstChildElement(); !op.isNull(); op = op.nextSiblingElement()) {
		if (op.tagName() != "option" && op.tagName() != "o")
			continue; //short <o></o> notation possible
		if (!hasSectionSpecified(section_, op)) //option is excluded for this section
			continue;

		substituteKeys(op, "@", key_);
		auto *checkbox( new QCheckBox(op.attribute("value")) );
		//connect to lambda function to emit current index (modern style signal mapping):
		connect(checkbox, &QCheckBox::stateChanged, this, [=] { changedState(counter); });
		checkbox_container_->addWidget(checkbox, counter, 0);

		/* set item properties */
		if (!op.attribute("color").isEmpty())
			checkbox->setStyleSheet("QCheckBox {color: " + colors::getQColor(op.attribute("color")).name() + "}");
		checkbox->setFont(setFontOptions(checkbox->font(), op));

		/* help text */
		QString helptext( op.firstChildElement("help").text() );
		if (helptext.isEmpty()) //same as addHelp but for a certain grid position
			helptext = op.attribute("help");
		if (helptext.isEmpty())
			helptext = op.attribute("h");
		auto *help( new Helptext(helptext, false, false) ); //if this is made optional account for it in onPropertySet's ... -1!
		if (helptext.isEmpty())
			help->hide();
		checkbox->setToolTip(op.attribute("help"));
		checkbox_container_->addWidget(help, counter, 1, 1, 1, Qt::AlignRight);

		/* child elements of this checkbox */
		auto *item_group( new Group(section_, "_item_choice_" + key_, false, false, false, true) ); //tight layout
		recursiveBuild(op, item_group, section_);
		item_group->setVisible(false);
		child_container_->addWidget(item_group);
		counter++;

		if (op.attribute("default").toLower() == "true") { //collect default values declared via attributes
			const QString def_val( this->property("default_value").toString() );
			this->setProperty("default_value", (def_val.isEmpty()? "" : def_val + " ") + op.attribute("value"));
			checkbox->setCheckState(Qt::Checked); //to set the default value
		}
	}
}

/**
 * @brief Collect checkbox texts in the order they were checked in.
 * @return INI value with ordered texts.
 */
QString Choice::getOrderedIniList() const
{
	QString list_values;
	for (auto &it : ordered_item_list_) {
		auto *box_nr = qobject_cast<QCheckBox *>(
		   checkbox_container_->getGridLayout()->itemAtPosition(it, 0)->widget());
		list_values += box_nr->text() + " "; //collect values as "value1 value2 value3..."
	}
	list_values.chop(1); //trailing space (if available)
	return list_values;
}

/**
 * @brief Set the child containers' visibilities.
 * @param[in] index Index of the checkbox that is being clicked.
 * @param[in] checked State of the checkbox that is being clicked.
 */
void Choice::setChildVisibility(const int &index, const Qt::CheckState &checked)
{
	const QLayout *group_layout( child_container_->getLayout() ); //get item number 'index' from the child group's layout
	auto *item_group( qobject_cast<Group *>(group_layout->itemAt(index)->widget()) );

	//Run through all checkboxes and find out if at least one shows children.
	//This is done to hide the main container if not one child panel is visible,
	//saving a few pixels that would seem out of place.
	bool one_visible = false;
	for (int ii = 0; ii < checkbox_container_->getGridLayout()->rowCount(); ++ii) {
		auto *checkbox = qobject_cast<QCheckBox *>(checkbox_container_->getGridLayout()->
		    itemAtPosition(ii, 0)->widget());
		if (checkbox->checkState() == Qt::Checked) {
			if (!static_cast<Group *>(group_layout->itemAt(ii)->widget())->isEmpty()) {
				one_visible = true;
				break;
			}
		}
	}

	//show the clicked child's group on item check if it's not empty:
	item_group->setVisible(checked == Qt::Checked && !item_group->isEmpty());
	child_container_->setVisible(one_visible);
}

/**
 * @brief Event listener for when a single checkbox is checked/unchecked.
 * @details This function shows/hides child elements when a checkbox changes.
 * @param[in] index The index/row of the clicked item.
 */
void Choice::changedState(int index)
{
	auto *clicked_box = qobject_cast<QCheckBox *>(
	    checkbox_container_->getGridLayout()->itemAtPosition(index, 0)->widget());

	if (clicked_box->checkState() == Qt::Unchecked) {
		auto erase_it( std::find(ordered_item_list_.begin(), ordered_item_list_.end(), index) );
		if (erase_it != ordered_item_list_.end())
			ordered_item_list_.erase(erase_it);
	} else if (clicked_box->checkState() == Qt::Checked) {
		setUpdatesEnabled(false); //we only experience flickering when hiding
		ordered_item_list_.push_back(index);
	}
	setChildVisibility(index, clicked_box->checkState()); //hide child and maybe main containers

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
void Choice::onPropertySet()
{
	//in this case the INI value is a list of options to set, i. e. "key = value1 value2 value3..."
	const QString values( this->property("ini_value").toString() );
	if (ini_value_ == values)
		return;
	const QStringList value_list( values.split(QRegExp("\\s+"), QString::SkipEmptyParts) );

	if (checkbox_container_->count() == 1) {
		topLog(QString(tr(R"(XML error: No checkable options set for Choice panel "%1::%2".)").arg(
		    section_, key_)), "error");
		return;
	}

	//clear the list first (so that INI settings can overwrite XML settings):
	for (int ii = 0; ii < checkbox_container_->getGridLayout()->rowCount(); ++ii) {
		auto *checkbox = qobject_cast<QCheckBox *>(checkbox_container_->getGridLayout()->
		    itemAtPosition(ii, 0)->widget());
		checkbox->setCheckState(Qt::Unchecked);
	}

	for (auto &val : value_list) { //run through INI value list and find corresponding checkboxes
		//Note: since we find and click the checkbox for each value they are correctly inserted into the ordering
		bool found_option_to_set = false;
		for (int jj = 0; jj < checkbox_container_->getGridLayout()->rowCount(); ++jj) {
			auto *checkbox = qobject_cast<QCheckBox *>(checkbox_container_->getGridLayout()->
			    itemAtPosition(jj, 0)->widget());
			if (QString::compare(checkbox->text(), val, Qt::CaseInsensitive) == 0) {
				checkbox->setCheckState(Qt::Checked);
				found_option_to_set = true; //an XML value exists for this INI value
				break;
			}
		}
		if (!found_option_to_set)
			topLog(tr(R"(Choice item \"%1\" could not be set from INI file for key "%2::%3": no such option specified in XML file)").arg(
			    val, section_, key_), "warning");
	} //endfor ii
}
