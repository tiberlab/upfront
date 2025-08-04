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

#include "Dropdown.h"
#include "src/main/constants.h"
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/inishell.h"

#include <QFont>
#include <QStringList>
#include <QTimer>

/**
 * @class Dropdown
 * @brief Default constructor for a Dropdown panel.
 * @details A Dropdown panel shows a dropdown menu where additional options pop up depending on the
 * selected menu item. It can allow to enter free text.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Choice panel.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Dropdown::Dropdown(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* label and combo box */
	dropdown_ = new QComboBox;
	setPrimaryWidget(dropdown_);
	dropdown_->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	connect(dropdown_, QOverload<int>::of(&QComboBox::activated), this, &Dropdown::itemChanged);
	connect(dropdown_, &QComboBox::editTextChanged, this, &Dropdown::editTextChanged); //free text mode

	/* layout of the basic elements */
	auto *dropdown_layout( new QHBoxLayout );
	if (!key.isEmpty()) {
		auto *key_label( new Label(QString(), QString(), options, no_spacers, key_) );
		dropdown_layout->addWidget(key_label, 0, Qt::AlignLeft);
	}
	dropdown_layout->addWidget(dropdown_, 0, Qt::AlignLeft);
	//add a big enough spacer to the right to keep the buttons to the left (unless it's a horizontal layout):
	if (!no_spacers)
		dropdown_layout->addSpacerItem(buildSpacer());
	main_help_ = addHelp(dropdown_layout, options, no_spacers, true); //force to add a Helptext for access by subitems
	main_help_->setProperty("main_help", options.firstChildElement("help").text());

	/* layout of the basic elements plus children */
	container_ = new Group(QString(), QString(), true, false, false, true); //tight layout
	container_->setVisible(false);
	auto *layout( new QVBoxLayout );
	setLayoutMargins(layout);
	layout->addLayout(dropdown_layout);
	layout->addWidget(container_);
	this->setLayout(layout);

	setOptions(options); //construct child panels and set mode

	if (main_help_->property("main_help").isNull() && !has_child_helptexts_)
		main_help_->hide(); //no main help and no child panel help --> save space
}

/**
 * @brief Make the Dropdown's text public.
 * @details Other panels that use the Dropdown are able to access the text this way.
 * @return The current QComboBox text.
 */
QString Dropdown::currentText() const
{
	return getCurrentText();
}

/**
 * @brief Return height of the QComboBox.
 * @details This is used for example in the Selector to size the buttons according to the
 * real QComboBox, and not the Dropdown.
 * @return Height of the QComboBox.
 */
int Dropdown::getComboBoxHeight() const
{
	return dropdown_->height();
}

/**
 * @brief Suppress a possible warning while resetting.
 * @param[in] set_default If true, reset the value to default. If false, delete the key.
 */
void Dropdown::clear(const bool &set_default)
{
	this->setProperty("clearing", true);
	this->setProperty("ini_value", ini_value_);
	this->setProperty("ini_value", set_default? this->property("default_value") : QString());
}

/**
 * @brief Parse options for a Dropdown panel from XML.
 * @param[in] options XML node holding the Dropdown panel.
 */
void Dropdown::setOptions(const QDomNode &options)
{
	QStringList item_strings; //to size it with biggest item (if not too big)

	if (options.toElement().attribute("editable").toLower() == "true") { //free text mode
		dropdown_->setEditable(true);
		dropdown_->lineEdit()->setPlaceholderText(tr("<edit>")); //display instead empty
		booleans_only_ = false; //free text disables boolean mode
	} else { //fixed list
		static const QString dummy_text("<select>");
		/* add italic dummy item that stands for "blank" */
		QFont dummy_font( dropdown_->font() );
		dummy_font.setItalic(true);
		dropdown_->addItem(dummy_text);
		child_nodes_.emplace_back(QDomElement()); //NULL child for dummy item
		dropdown_->setItemData(0, "", Qt::UserRole);
		dropdown_->setItemData(0, dummy_font, Qt::FontRole);
		Group *dummy_group( new Group(section_, "_dummy_group_" + key_,
		    false, false, false, true) ); //tight layout
		container_->addWidget(dummy_group); //empty group is selected on dummy click
		item_strings.push_back(dummy_text);
		item_help_texts_.push_back(QString());
	}

	/*
	 * Check whether it is requested via XML to build chlid nodes only on request (first list click)
	 * for performance reasons. This mode should be enabled for big panel nodes with lots of children
	 * that do _not_ have to be available if the respective parent option is not set.
	 * For example in MeteoIO we make the reasonable request that TA::FILTERS1 = MIN must be found
	 * before TA::ARG1::MIN so that the child panels are built.
	 */
	const bool generate_on_the_fly = (options.toElement().attribute("pre-generate").toLower() == "false");

	/* parse child panels */
	bool found_default = false;
	int default_index = 0;
	bool found_option = false;

	for (QDomElement op = options.firstChildElement(); !op.isNull(); op = op.nextSiblingElement()) {
		if (op.tagName() != "option" && op.tagName() != "o")
			continue; //short <o></o> notation possible
		if (!hasSectionSpecified(section_, op)) //option is excluded for this section
			continue;
		found_option = true;
		/* add childrens' names to dropdown and build them */
		const QString value( op.attribute("value") );
		const QString caption( op.attribute("caption") );
		dropdown_->addItem(caption.isNull()? value : caption);
		dropdown_->setItemData(dropdown_->count() - 1, value, Qt::UserRole);

		item_strings.push_back(dropdown_->itemText(dropdown_->count() - 1));
		const QFont item_font( setFontOptions(dropdown_->font(), op) );
		dropdown_->setItemData(dropdown_->count() - 1, item_font, Qt::FontRole);
		dropdown_->setItemData(dropdown_->count() - 1, colors::getQColor(op.attribute("color")),
		    Qt::ForegroundRole); //the popup list can show colors, the main text unfortunately can't

		/* tooltips and help texts */
		QString tooltip( op.attribute("help").isNull()? op.attribute("h") : op.attribute("help") );
		if (!value.isNull()) //set true item value as tip, optionally followed by item help
			tooltip.prepend(value + (tooltip.isEmpty()? "" : ": "));
		dropdown_->setItemData(dropdown_->count() - 1, tooltip, Qt::ToolTipRole);
		QString item_help( op.firstChildElement("help").text() );
		if (item_help.isNull())
			item_help = op.firstChildElement("h").text();
		item_help_texts_.push_back(item_help);
		if (!item_help_texts_.back().isNull())
			has_child_helptexts_ = true;


		auto *item_group( new Group(QString(), QString()) ); //group all elements of this option together
		container_->addWidget(item_group);

		if (generate_on_the_fly) { //build child panels on user interaction only
			child_nodes_.emplace_back(op); //cache for building on request later
		} else {
			recursiveBuild(op, item_group, section_); //build all children immediately (safe)
			child_nodes_.emplace_back(QDomElement( )); //remember it's built
		}

		if (op.attribute("default").toLower() == "true") { //overpowers default setting in tag
			this->setProperty("default_value", value);
			default_index = dropdown_->count() - 1;
			if (found_default)
				topLog(tr(R"(XML error: Multiple default values given in option-attributes of Alternative panel "%1::%2")").arg(
			section_, key_), "error");
			found_default = true;
		}
		//check in XML if there are only booleans (if yes, integer INI keys are translated):
		if (value.toLower() != "true" && value.toLower() != "false")
			booleans_only_ = false;
	}
	if (dropdown_->isEditable())
		dropdown_->lineEdit()->setText(""); //don't automatically select 1st entry
	if (!found_option)
		container_->setVisible(false);
	if (found_default) {
		dropdown_->setCurrentIndex(default_index); //init showing the startup group
		emit itemChanged(default_index);
	}

	dropdown_->setMinimumWidth(getElementTextWidth(item_strings, Cst::tiny, Cst::width_dropdown_max) +
	    Cst::dropdown_safety_padding);

	//style dummy item italic (I don't know why this is necessary, see notes below):
	QTimer::singleShot(1, this, &Dropdown::styleTimer);
}

/**
 * @brief Event listener for when a dropdown menu item is selected.
 * @details This function shows/hides the children corresponding to the selected dropdown item.
 * @param[in] index Index/number of the selected item.
 */
void Dropdown::itemChanged(int index)
{
	QLayout *group_layout( container_->getLayout() );
	for (int ii = 0; ii < group_layout->count(); ++ii) {
		//find the group to display and hide all others:
		auto *group_item( qobject_cast<Group *>(group_layout->itemAt(ii)->widget()) );
		group_item->setVisible(index == ii);
		if (ii == index) {
			//build requested child's node on first demand:
			if (!child_nodes_.at(static_cast<size_t>(ii)).isNull()) {
				recursiveBuild(child_nodes_.at(static_cast<size_t>(ii)), group_item, section_);
				child_nodes_.at(static_cast<size_t>(ii)) = QDomElement(); //set to NULL
			}
			//hide the container if the displayed group is empty:
			container_->setVisible(!group_item->isEmpty());
		}
	}

	QString dropdown_data_text( getCurrentText() ); //underlying INI value when available, text otherwise
	setDefaultPanelStyles(dropdown_data_text);

	/* display per-item help if available */
	if (has_child_helptexts_)
		main_help_->updateText( //switch main help to item help if available
		    item_help_texts_.at(index).isEmpty()? main_help_->property("main_help").toString() : item_help_texts_.at(index));

	QFont select_font( dropdown_->font() );
	select_font.setItalic(dropdown_data_text == ""); //dummy item of non-editable mode is italic
	dropdown_->setFont(select_font);
	//respect user's wish to use 1/0 for true/false in the INI:
	if (numeric_ini_value_ && dropdown_data_text.toLower() == "false")
		dropdown_data_text = "0";
	else if (numeric_ini_value_ && dropdown_data_text.toLower() =="true")
		dropdown_data_text = "1";
	setIniValue(dropdown_data_text);
}

/**
 * @brief Event listener for when the free editable text is changed.
 * @details In free text mode, popup menu item clicks are also registered by itemChanged(), but
 * editing the text box is caught here.
 * @param[in] text The entered text.
 */
void Dropdown::editTextChanged(const QString &text)
{
	//Note that the following will switch to the caption if the underlying INI value matches
	//the entered text, showing the relation. If this behaviour is changed then onPropertySet()
	//needs to be modified to display a value found in the INI file.

	int idx = dropdown_->findData(text, Qt::UserRole);
	if (idx == -1) //not found in item data - search in captions
		idx = dropdown_->findText(text);

	if (idx != -1) { //still not found - set free text
		dropdown_->setCurrentIndex(idx);
		emit(itemChanged(idx));
		return;
	} else {
		setIniValue(dropdown_->currentText());
	}
	container_->setVisible(false); //not reached if item is found in list
	main_help_->updateText(main_help_->property("main_help").toString());
	setDefaultPanelStyles(text);
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void Dropdown::onPropertySet()
{
	const QString text_to_set( this->property("ini_value").toString() );
	if (ini_value_ == text_to_set)
		return;

	if (booleans_only_ && (text_to_set == "0" || text_to_set == "1"))
		numeric_ini_value_ = true; //remember that the INI uses 1/0 for true/false
		//TODO: handle "t/f" like the checkbox
	if  (dropdown_->isEditable()) {
		dropdown_->setCurrentText(text_to_set); //will click a list item if found
		return;
	}
	//look for item in the dropdown list and select it:
	for (int ii = 0; ii < dropdown_->count(); ++ii) {
		const QString dd_text( dropdown_->itemData(ii, Qt::UserRole).toString().toLower() );
		if ( (dd_text.toLower() == text_to_set.toLower()) ||
		    (booleans_only_ && dd_text == "false" && text_to_set == "0") ||
		    (booleans_only_ && dd_text == "true" && text_to_set == "1") ) {
			dropdown_->setCurrentIndex(ii);
			emit itemChanged(ii);
			return;
		}
	} //endfor ii
	if (!this->property("clearing").toBool())
		topLog(tr(R"(Value "%1" could not be set in Alternative panel from INI file for key "%2::%3": no such option specified in XML file)").arg(
		    text_to_set, section_, key_), "warning");
	else
		this->property("clearing") = false;
	setDefaultPanelStyles(text_to_set);
}

/**
 * @brief Get the current Dropdown item's text.
 * @details The Dropdown panel can display captions different to the INI value. Return the true INI
 * value or the text in free text mode.
 * @return The INI value belonging to the current Dropdown text.
 */
QString Dropdown::getCurrentText() const
{
	const int item_idx = dropdown_->findText(dropdown_->currentText()); //case insensitive
	if (item_idx == -1)
		return dropdown_->currentText();
	return dropdown_->itemData(item_idx, Qt::UserRole).toString();
}

/**
 * @brief Workaround to set the first dummy item italic if appropriate.
 * @details Styling a QComboBox remains a bit of a mystery (Qt 5.13.5). Some observations:
 * -) Usually, setting the font of the QComboBox also sets and overwrites the font of the popup list.
 *    The exact order it is done here avoids that, and setting the QComboBox font to the child item font
 *    on click works, just not from the constructor; hence this timer on initialization.
 * -) The popup menu can be targetetd via Stylesheets, but there does not seem to be a way to style only
 *    the collapsed main text, no hints in QComboBoxes 10 child widgets. Styling the main text will
 *    overwrite the styling of the popup list. The QLineEdit in free text mode could be colored.
 * -) Even if it were possible, I don't think individual items can be styled with sheets. This also
 *    prevents using a delegator.
 * -) Currently everything works except for showing a list item's color in the main text field.
 * -) A side effect of all this is that when the box is styled as mandatory or defaulted then the
 *    popup list is also styled that way.
 */
void Dropdown::styleTimer()
{ //only called once in the constructor
	if (getCurrentText().isEmpty()) {
		QFont font_italics( dropdown_->font() );
		font_italics.setItalic(true);
		dropdown_->setFont(font_italics);
	}
}
