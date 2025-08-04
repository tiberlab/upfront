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

#include "Selector.h"
#include "Label.h"
#include "src/main/constants.h"
#include "src/main/inishell.h"
#include "src/main/XMLReader.h"

#include <QPushButton>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class SelectorKeyPressFilter
 * @brief Key press event listener for the Selector panel.
 * @details We can not override 'event' in the panel itself because we want to
 * listen to the events of a child widget.
 * @param[in] object Object the event stems from (the combobox or text field).
 * @param[in] event The type of event.
 * @return True if the event was accepted.
 */
bool SelectorKeyPressFilter::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		const QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
		if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
			auto *sel( qobject_cast<Selector *>(object->parent()) );
			sel->plus_button_->animateClick(); //click + button on 'Enter'
		} //endif key_event
	}
	return QObject::eventFilter(object, event); //pass to actual event of the object
}

/**
 * @class Selector
 * @brief Default constructor for a Selector.
 * @details A selector panel allows to select from a list of text pieces (e. g. meteo parameters), either from
 * a fixed dropdown list or with possible free text input. Its single child panel must be declared as "template"
 * in its attributes. With the click of a button the child element is duplicated, inheriting the text piece.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Selector.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Selector::Selector(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* dropdown/text, label and buttons */
	key_events_input_ = new SelectorKeyPressFilter;
	//selectors that don't have predefined options -> text field instead of combo box:
	if (options.toElement().attribute("textmode").toLower() == "true") {
		textfield_ = new QLineEdit;
		textfield_->setMinimumWidth(Cst::tiny);
		textfield_->installEventFilter(key_events_input_);
	} else {
		dropdown_ = new QComboBox;
		dropdown_->setMinimumWidth(Cst::tiny); //no tiny elements
		if (options.toElement().attribute("editable").toLower() != "false")
			dropdown_->setEditable(true); //free text with autocomplete
		dropdown_->installEventFilter(key_events_input_);
	}

	auto *key_label( new Label(section_, "_selector_label_" + key_, options, no_spacers, key_, this) );
	plus_button_ = new QPushButton("+");
	setPrimaryWidget(plus_button_, false); //set ID later
	auto *minus_button( new QPushButton("-") );
	plus_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	minus_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(plus_button_, &QPushButton::clicked, this, &Selector::guiAddPanel);
	connect(minus_button, &QPushButton::clicked, this, &Selector::removeCurrentPanel);

	/* layout of the basic elements */
	auto *selector_layout( new QHBoxLayout );
	setLayoutMargins(selector_layout);
	selector_layout->addWidget(key_label);
	if (textfield_)
		selector_layout->addWidget(textfield_, 0, Qt::AlignLeft);
	else
		selector_layout->addWidget(dropdown_, 0, Qt::AlignLeft);
	selector_layout->addWidget(plus_button_, 0, Qt::AlignLeft);
	selector_layout->addWidget(minus_button, 0, Qt::AlignLeft);
	if (!no_spacers)
		selector_layout->addSpacerItem(buildSpacer()); //keep buttons from wandering to the right
	addHelp(selector_layout, options, no_spacers);

	/* layout for the basic elements plus children */
	container_ = new Group(section_, "_selector_" + key_, true);
	container_->setVisible(false); //only visible with items
	auto *layout( new QVBoxLayout );
	setLayoutMargins(layout);
	layout->addLayout(selector_layout);
	layout->addWidget(container_);
	this->setLayout(layout);

	setOptions(options); //construct children
}

/**
 * @brief Destructor for the Selector panel with minimal cleanup.
 */
Selector::~Selector()
{
	delete key_events_input_;
}

/**
 * @brief Remove all child panels.
 * @param[in] set_default Unused in this panel.
 */
void Selector::clear(const bool &/*set_default*/)
{
	for (auto &gr : container_map_)
		delete gr.second;
	container_map_.clear();
	container_->setVisible(false);
	this->setProperty("ini_value", QString()); //so that new requests will trigger
}

/**
 * @brief Parse options for a Selector from XML.
 * @param[in] options XML node holding the Selector.
 */
void Selector::setOptions(const QDomNode &options)
{
	bool found_template = false;
	for (QDomElement par = options.firstChildElement("parameter"); !par.isNull(); par = par.nextSiblingElement("parameter")) {
		if (par.attribute("template").toLower() == "true") {
			if (found_template) {
				topLog(tr(R"(XML error: Multiple template panels found for key "%1::%2" - ignoring)").arg(
				    section_, key_), "error");
				break;
			}
			templ_ = par; //save the node describing the child (shallow copy)
			this->setObjectName(getQtKey(section_ + Cst::sep + par.attribute("key")));
			found_template = true;
		}
	}
	if (textfield_) { //text mode - set optional placeholder text
		QString placeholder_text( options.toElement().attribute("placeholder"));
		if (!placeholder_text.isEmpty())
			textfield_->setPlaceholderText(placeholder_text);
	} else { //dropdown mode - fill with children
		for (QDomElement op = options.firstChildElement("option"); !op.isNull(); op = op.nextSiblingElement("option"))
			dropdown_->addItem(op.attribute("value")); //fill list of texts the selector shows as default
	}

	if (!found_template)
		topLog(tr(R"(XML error: No template panel given for key "%1::%2")").arg(section_, key_), "error");
}

/**
 * @brief Event listener for the plus button: add a child panel to the selector.
 * @details This function replicates the child panel from XML and passes the selected text.
 */
void Selector::guiAddPanel()
{
	const QString param_text( getCurrentText() );
	if (param_text.isEmpty()) {
		topStatus(tr("Empty text field"), "error", false, Cst::msg_short_length);
		return;
	}
	if (container_map_.count(param_text) != 0) { //only one panel per piece of text
		topStatus(tr("Item already exists"), "error", false, Cst::msg_short_length);
		return;
	}
	topStatus(""); //above messages could be confusing if still displayed from recent click
	addPanel(param_text);
}

/**
 * @brief Construct a new child panel from the template.
 * @details This function gives the Dropdown text to the template and constructs a new clone
 * of the template panel.
 * @param[in] param_text The text to transport to the child panel.
 */
void Selector::addPanel(const QString &param_text)
{
	//we clone the child node (deep copy for string replacement) and put it in a dummy parent:
	QDomNode node( prependParent(templ_) ); //nest for recursion
	QDomElement element( node.toElement() );
	substituteKeys(element, "%", param_text); //recursive substitution for all children
	//draw it on next call (don't use as template again):
	node.firstChildElement("parameter").setAttribute("template", "false");

	/* construct all children and grandchildren */
	Group *new_group( new Group(section_, "_selector_panel_" + key_) );
	recursiveBuild(node, new_group, section_);
	container_->addWidget(new_group);
	container_->setVisible(true);
	this->setProperty("is_mandatory", "false"); //if it's mandatory then the template now shows this
	setPanelStyle(MANDATORY, false);

	const auto panel_pair( std::make_pair(param_text, new_group) );
	container_map_.insert(panel_pair); //keep an index of text to panel number
}

/**
 * @brief Remove a child panel from the Selector.
 * @param[in] param_text The child panel's parameter.
 */
void Selector::removePanel(const QString &param_text)
{
	auto it( container_map_.find(param_text) ); //look up if item exists in map
	if (it != container_map_.end()) {
		topStatus(""); //no "does not exist" error message from earlier
		it->second->erase(); //delete the group's children
		delete it->second; //delete the group itself
		container_map_.erase(it);
		if (container_map_.empty()) { //no more children - save a couple of blank pixels
			container_->setVisible(false);
			//the template reacts on mandatory, but if there is none left, transfer to this:
			if (templ_.toElement().attribute("optional") == "false") {
				this->setProperty("is_mandatory", "true");
				setPanelStyle(MANDATORY);
			}
		}
	} else {
		topStatus(tr(R"(Item "%1" does not exist)").arg(param_text), "error",
		    false, Cst::msg_short_length);
	}
}

/**
 * @brief Event listener for the minus button: remove a panel for the selected text piece.
 * @details If some text is present in the dropdown menu, the corresponding child is deleted.
 */
void Selector::removeCurrentPanel()
{
	removePanel(getCurrentText());
}

/**
 * @brief Event listener for requests to add new panels.
 * @details This function gets called from the GUI building routine when it detects that this
 * Selector is suitable for a given parametrized INI key. It's a signal to add the child panel
 * corresponding to this parameter so that it's available for the specific INI key.
 * For example, a Selector's ID could be "%::COPY", and when the request to add "TA" was
 * processed, a child panel will exist for INI key "TA::COPY".
 */
void Selector::onPropertySet()
{
	const QString panel_to_add( this->property("ini_value").toString() );
	if (panel_to_add.isNull()) //when cleared
		return;
	addPanel(panel_to_add);
}
