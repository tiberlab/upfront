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

#include "Replicator.h"
#include "Label.h"
#include "src/main/inishell.h"
#include "src/main/XMLReader.h"

#ifdef DEBUG
	#include <QDebug>
	#include <iostream>
#endif //def DEBUG

/**
 * @class Replicator
 * @brief Default constructor for a Replicator.
 * @details A Replicator holds a widget which it can replicate with the click of a button.
 * It does not have a separate identifier but rather it is activated in any given panel with the "replicate"
 * attribute. The number of the created panel is propagated to all children via "#".
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Replicator.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Replicator::Replicator(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	//for the child container we choose a grid layout to be able to control the columns:
	container_ = new Group(QString(), QString(), true, true); //has border and is grid

	/* label, dropdown menu and buttons */
	auto *key_label( new Label(section_, "_replicator_label_" + key_, options, no_spacers, key_, this) );

	plus_button_ = new QPushButton("+");
	setPrimaryWidget(plus_button_, false);
	auto *minus_button( new QPushButton("-") );
	plus_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	minus_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	//find last index and replicate to new child (button starts with 1, INI can start with 0):
	connect(plus_button_, &QPushButton::clicked, this, [=]{ replicate(findLastItemRow() + 1); });
	connect(minus_button, &QPushButton::clicked, this, &Replicator::deleteLast); //delete last child

	/* layout of the basic elements */
	auto *replicator_layout( new QHBoxLayout );
	setLayoutMargins(replicator_layout);
	replicator_layout->addWidget(key_label);
	replicator_layout->addWidget(plus_button_, 0, Qt::AlignLeft);
	replicator_layout->addWidget(minus_button, 0, Qt::AlignLeft);
	if (!no_spacers)
		replicator_layout->addSpacerItem(buildSpacer()); //keep widgets to the left
	addHelp(replicator_layout, options, no_spacers);

	/* layout of the basic elements plus children */
	auto *layout( new QVBoxLayout );
	setLayoutMargins(layout);
	layout->addLayout(replicator_layout);
	layout->addWidget(container_);
	this->setLayout(layout);

	setOptions(options); //set the child template
	container_->setVisible(false); //only visible when an item is selected
}

/**
 * @brief Parse options for a Replicator from XML.
 * @param[in] options XML node holding the Replicator.
 */
void Replicator::setOptions(const QDomNode &options)
{
	templ_ = options; //save a reference to the child XML node (shallow copy)
	templ_.removeChild(templ_.firstChildElement("help"));
	this->setObjectName(getQtKey(section_ + Cst::sep + options.toElement().attribute("key")));
}

/**
 * @brief Find the last item in the layout.
 * @details The index of the item we add is stored in order of insertion and independent of the
 * panel's grid position. This function checks the layout position for each index and returns the
 * biggest one. For example, if key1 and key3 are present then the indices are (1, 2), but this
 * function returns 3.
 * @return The grid position of the last child panel.
 */
int Replicator::findLastItemRow() const
{
	int max_row = 0; //empty grid layout
	for (int ii = container_->count() - 1; ii >= 0; --ii) {
		int row, col, rowspan, colspan;
		container_->getGridLayout()->getItemPosition(ii, &row, &col, &rowspan, &colspan);
		if (row > max_row)
			max_row = row;
	}
	return max_row;
}

/**
 * @brief Event listener for the plus button: replicate the child widget.
 * @details The child was saved as XML node, here this is parsed and built.
 */
void Replicator::replicate(const int &panel_number)
{
	setUpdatesEnabled(false);
	QDomNode node( prependParent(templ_) ); //prepend artificial parent node for recursion (runs through children)
	node.firstChildElement().setAttribute("replicate", "false"); //set node to normal element to be constructed
	node.firstChildElement().setAttribute("label", QString("No %1:").arg(panel_number));
	QDomElement element( node.toElement() );

	//recursively inject the element's number into the childrens' keys:
	substituteKeys(element, "#", QString::number(panel_number));

	Group *new_group = new Group(section_, "_replicator_item_" + key_);
	recursiveBuild(node, new_group, section_); //construct the children
	if (this->property("no_ini").toBool()) {
		const QList<Atomic *>new_panels( new_group->findChildren<Atomic *>() );
		for (auto &panel : new_panels)
			panel->setProperty("no_ini", "true");
	}

	//add new child group to column number "panel_number", effectively sorting the children:
	container_->getGridLayout()->addWidget(new_group, panel_number, 0);
	container_->setVisible(true);
	this->setProperty("is_mandatory", "false"); //if it's mandatory then the template now shows this
	setPanelStyle(MANDATORY, false);
	setBufferedUpdatesEnabled(1);
}

/**
 * @brief Event listener for the minus button: Remove the instance of the child widget created last.
 */
bool Replicator::deleteLast()
{
	if (container_->count() == 0)
		return false; //no more widgets left
	setUpdatesEnabled(false);

	int last_row = findLastItemRow(); //the maximum row we used to insert
	auto *to_delete( qobject_cast<Group *>(container_->getGridLayout()->
	    itemAtPosition(last_row, 0)->widget()) );
	if (to_delete) {
		to_delete->erase(); //delete the group's children
		delete to_delete; //delete the group itself
#ifdef DEBUG
	} else {
		qDebug() << "Could not find a grid layout item to erase when it should have existed in Replicator::deleteLast()";
#endif //def DEBUG
	}

	if (container_->count() == 0) {
		//the template reacts on mandatory, but if there is none left, transfer to this:
		if (templ_.toElement().attribute("optional") == "false") {
			this->setProperty("is_mandatory", "true");
			setPanelStyle(MANDATORY);
		}
		container_->setVisible(false);
	}
	setBufferedUpdatesEnabled();
	return true;
}

/**
 * @brief Event listener for requests to add new panels.
 * @details This function gets called from the GUI building routine when it detects that this
 * Replicator is suitable for a given numbered INI key. It's a signal to add the child panel
 * corresponding to this parameter so that it's available for the specific INI key.
 * For example, a Replicator's ID could be "STATION#", and when the request to add "3" was
 * processed, a child panel will exist for INI key "STATION3".
 */
void Replicator::onPropertySet()
{ //gets called alphabetically (but it's 1, 10, 2, ...)
	const QString panel_to_add( this->property("ini_value").toString() );
	if (panel_to_add.isNull()) //when cleared
		return;
	replicate(panel_to_add.toInt());
}

/**
 * @brief This function removes all child panels.
 * @param[in] set_default Unused in this panel.
 */
void Replicator::clear(const bool &/*set_default*/)
{
	while (deleteLast());
	this->setProperty("ini_value", QString()); //so that new requests will trigger
}
