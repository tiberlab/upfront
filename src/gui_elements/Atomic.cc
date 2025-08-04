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

#include "Atomic.h"
#include "src/main/inishell.h"

#include <QAction>
#include <QCryptographicHash>
#include <QCursor>
#include <QFontMetrics>

#include <utility>

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class Atomic
 * @brief Base class of most panels.
 * @details The panels inherit from this class which provides some common functionality
 * like handling the object IDs.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this panel.
 * @param[in] parent The parent widget.
 */
Atomic::Atomic(QString section, QString key, QWidget *parent)
    : QWidget(parent), section_(std::move(section)), key_(std::move(key))
{
	ini_ = getMainWindow()->getIni();
	createContextMenu();
}

/**
 * @brief Check if a panel value is mandatory or currently at the default value.
 * @param[in] in_value The current value of the panel.
 */
void Atomic::setDefaultPanelStyles(const QString &in_value)
{
	setPanelStyle(FAULTY, false); //first we disable temporary styles
	setPanelStyle(VALID, false);
	const bool is_default = (QString::compare(in_value, this->property("default_value").toString(),
	    Qt::CaseInsensitive) == 0);
	setPanelStyle(DEFAULT, is_default && !this->property("default_value").isNull() && !in_value.isNull());
	if (this->property("is_mandatory").toBool())
		setPanelStyle(MANDATORY, in_value.isEmpty());
}

/**
 * @brief Get an alphanumeric key from an aribtrary one.
 * @details Unfortunately, our arg1::arg2 syntax runs into Qt's sub-controls syntax, but the targeting
 * with stylesheets only allows _ and - anyway so we hash the ID. This also ensures that all object
 * names we set manually that contain an underscore or dash do not coincide with an INI setting name.
 *
 * @param[in] ini_key Key as given in the XML
 * @return A key without special chars.
 */
QString Atomic::getQtKey(const QString &ini_key)
{ //sounds heavy but is fast
	return QString(QCryptographicHash::hash((ini_key.toLower().toLocal8Bit()),
	    QCryptographicHash::Md5).toHex());
}

/**
 * @brief Return this panel's set INI value.
 * @details This function is called by the main output routine for all panels.
 * @param[out] section Gets set to this panel's section.
 * @param[out] key Gets stored to this panel's key.
 * @return This panel's value.
 */
QString Atomic::getIniValue(QString &section, QString &key) const noexcept
{
	section = section_;
	key = key_;
	return ini_value_;
}

/**
 * @brief Reset panel to the default value.
 * @param[in] set_default If true, reset the value to default. If false, delete the key.
 */
void Atomic::clear(const bool &set_default)
{
	/*
	 * The property needs to actually change to have a signal emitted. So first we set
	 * the property (since we set it to ini_value_, a check in onPropertySet() will
	 * make sure that nothing is calculated), and then set it to the default value
	 * (which will only take effect if it's different).
	 * TODO: This means unneccessary calls that may not be trivial (depending on the panel),
	 * so a redesign could be desired.
	 */
	this->setProperty("ini_value", ini_value_);
	this->setProperty("ini_value", set_default? this->property("default_value") : QString());
}

/**
 * @brief Set a panel's primary widget.
 * @details The widget pointed to this way is responsible for actually changing a value and
 * is used for highlighting purposes.
 * @param primary_widget The panel's main widget.
 */
void Atomic::setPrimaryWidget(QWidget *primary_widget, const bool &set_object_name, const bool &no_styles)
{
	primary_widget_ = primary_widget;
	primary_widget->setObjectName("_primary_" + getQtKey(getId()));
	if (set_object_name) //template panels may want to do this themselves (handling substitutions)
		this->setObjectName(getQtKey(getId()));
	QObject *const property_watcher( new PropertyWatcher(primary_widget) );
	this->connect(property_watcher, SIGNAL(changedValue()), SLOT(onPropertySet())); //old style for easy delegation
	this->installEventFilter(property_watcher);
	if (!no_styles) //panels that style different widgets than the primary one (e. g. Choice)
		setDefaultPanelStyles(this->property("ini_value").toString());
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, &Atomic::onConextMenuRequest);
}

/**
 * @brief Set a property indicating that the value this panel controls is defaulted or mandatory,
 * or to be highlighted in a different way.
 * @param[in] set Set style on/off.
 * @param[in] widget If given, set the style for this widget instead of the primary one
 * (used in Choice panel for example).
 */
void Atomic::setPanelStyle(const PanelStyle &style, const bool &set, QWidget *widget)
{
	QWidget *widget_to_set = (widget == nullptr)? primary_widget_ : widget;
	if (widget_to_set == nullptr) //e. g. horizontal panel
		return;

	QString style_string;
	switch (style) {
	case MANDATORY:
		style_string = "mandatory";
		break;
	case DEFAULT:
		style_string = "shows_default";
		break;
	case FAULTY:
		style_string = "faulty";
		break;
	case VALID:
		style_string = "valid";
	}
	widget_to_set->setProperty(style_string.toLocal8Bit(), set? "true" : "false");
	this->style()->unpolish(widget_to_set); //if a property is set dynamically, we might have to refresh
	this->style()->polish(widget_to_set);
} //https://wiki.qt.io/Technical_FAQ#How_can_my_stylesheet_account_for_custom_properties.3F

/**
 * @brief Switch between "faulty" and "valid" panel styles.
 * @param[in] on True to set "faulty", false to set "valid".
 */
void Atomic::setValidPanelStyle(const bool &on)
{
	setPanelStyle(VALID, on);
	setPanelStyle(FAULTY, !on);
}

/**
 * @brief Substitute values in keys and texts.
 * @details This is for child elements that inherit a property from its parent which should be
 * displayed (e. g. "TA" in "TA::FILTER1 = ..."). The function traverses through the whole child
 * tree recursively.
 * @param[in] parent_element Parenting XML node holding the desired values.
 * @param[in] replace String to replace.
 * @param[in] replace_with Text to replace the string with.
 */
void Atomic::substituteKeys(QDomElement &parent_element, const QString &replace, const QString &replace_with)
{ //TODO: clean up the substitutions with proper logic from bottom up
	for (QDomElement element = parent_element.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
		QString key(element.attribute("key"));
		//only subsitutute first occurrence, leave the rest to subsequent panels (e. g. TA::FILTER#::ARG#):
		element.setAttribute("key", key.replace(key.indexOf(replace), 1, replace_with));
		QString text(element.attribute("caption"));
		element.setAttribute("caption", text.replace(key.indexOf(replace), 1, replace_with)); //for labels
		QString label(element.attribute("label"));
		element.setAttribute("label", label.replace(label.indexOf(replace), 1, replace_with));
		substituteKeys(element, replace, replace_with);
	}
}

/**
 * @brief Build a standard spacer item for widget positioning.
 * @details This is the only little workaround we use for our layouts. The scroll areas are allowed
 * to resize everything, and sometimes they do so aggressively; for example, they ignore left-aligned
 * elements and move them to the right if the screen gets very big. If on the other hand we try to
 * adjust the sizes of all child widgets manually via .adjustSize() update(), etc., then we run into
 * troubles that require far uglier hacks if they can be solved at all. It has proven best to try not
 * to meddle with the layout manager.
 * The solution here is to add a huge spacer to a list of left-aligned widgets if we want to keep all of
 * them a fixed (small) size.
 * @return Spacer element that can be added to a layout.
 */
QSpacerItem * Atomic::buildSpacer()
{
	//If the spacer is smaller than the total width (including child panels of Alternative etc.,
	//then AlignLeft does not help anymore and everything starts wandering to the right.
	return new QSpacerItem(getMainWindow()->width() * 5, 0, QSizePolicy::Maximum); //huge spacer

	//TODO: This does leave some problems, namely when certain panels, e. g. a Selector, are nested.
	//The plus/minus buttons can start to wander, since there we disable the spacers.
	//For the usual XMLs it should not matter, but in the future it should be tested off-site how
	//to keep widgets to the left in the best manner.
}

/**
 * @brief Set margins of a layout.
 * @details This controls how much space there is between widgets, i. e. the spacing in the main GUI.
 * @param [in] layout Any layout.
 */
void Atomic::setLayoutMargins(QLayout *layout)
{
	//                  left, top, right, bottom
	layout->setContentsMargins(2, 1, 2, 1); //with our nested groups we want to keep them tight
}

/**
 * @brief Convenience call to add a Helptext object to the end of a vertical layout.
 * @details <help> elements are displayed in a Helptext, "help" attributes in the tooltip.
 * @param[in] layout Layout to add the Helptext to.
 * @param[in] options Parent XML node controlling the appearance of the help text.
 * @param[in] force Add a Helptext even if it's empty (used by panels that can change the text).
 */
Helptext * Atomic::addHelp(QHBoxLayout *layout, const QDomNode &options, const bool& tight, const bool &force)
{ //changes here might have to be mirrored in other places, e. g. Choice::setOptions()
	QDomElement help_element( options.firstChildElement("help") ); //dedicated <help> tag if there is one
	if (help_element.isNull())
		help_element = options.firstChildElement("h"); //shortcut notation
	const bool single_line = (help_element.attribute("wrap") == "false"); //single line extending the scroll bar
	const QString helptext( help_element.text() );
	if (primary_widget_ != nullptr) {
		const QString inline_help( options.toElement().attribute("help") ); //help in attribute as opposed to element
		primary_widget_->setToolTip(inline_help.isEmpty()? key_ : inline_help);
	}
	if (force || !helptext.isEmpty()) {
		auto *help( new Helptext(helptext, tight, single_line) );
		static constexpr int gap_width = 10; //little bit of space after the panels
		layout->addSpacerItem(new QSpacerItem(gap_width, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
		layout->addWidget(help, 0, Qt::AlignRight);
		return help;
	}
	return nullptr;
}

/**
 * @brief Call setUpdatesEnabled() with a tiny delay
 * @details There is GUI flickering when hiding widgets, even for simple ones. It is occasional, but
 * sometimes quite annoying. Disabling and re-enabling the painting is not enough, but firing a timer
 * to do so helps quite a bit. This is used by panels that can show/hide child panels.
 */
void Atomic::setBufferedUpdatesEnabled(const int &time)
{
	//TODO: there is still some flickering, but the mechanisms against it don't work,
	//and it's also a little strange in which direction the widgets jump.
	//I suspect that the QScrollAreas resizing the widgets might be the culprit.
	QTimer::singleShot(time, this, &Atomic::onTimerBufferedUpdatesEnabled);
}

/**
 * @brief Find the widest text in a list.
 * @details This function calculates the on-screen width of a list of texts and returns the
 * greatest one, optionally capping at a fixed value.
 * @param[in] text_list List of texts to check.
 * @param[in] element_min_width Minimum size the panel should have.
 * @param[in] element_max_width Maximum allowed width to return.
 * @return The maximum text width, or the hard set limit.
 */
int Atomic::getElementTextWidth(const QStringList &text_list, const int &element_min_width, const int &element_max_width)
{
	const QFontMetrics font_metrics( this->font() );
	int width = 0;
	for (auto &text : text_list) {
		const int text_width = font_metrics.boundingRect(text).width();
		if (text_width > width)
			width = text_width;
	}
	if (width > element_max_width)
		width = element_max_width;
	if (width < element_min_width)
		width = element_min_width;
	return width;
}

/**
 * @brief Set font stylesheet for an object.
 * @param[in] widget The widget to style the font of.
 * @param[in] options XML user options to parse.
 */
void Atomic::setFontOptions(QWidget *widget, const QDomNode &options)
{
	const QDomElement op( options.toElement() );
	QString stylesheet( widget->metaObject()->className() );
	stylesheet += " {";
	if (op.attribute("caption_bold").toLower() == "true")
		stylesheet += "font-weight: bold; ";
	if (op.attribute("caption_italic").toLower() == "true")
		stylesheet += "font-style: italic; ";
	if (!op.attribute("caption_font").isNull())
		stylesheet += "font-family: \"" + op.attribute("caption_font") + "\"; ";
	if (op.attribute("caption_underline").toLower() == "true")
		stylesheet += "text-decoration: underline; ";
	if (!op.attribute("caption_size").isNull())
		stylesheet += "font-size: " + op.attribute("caption_size") + "pt; ";
	if (!op.attribute("caption_color").isNull())
		stylesheet += "color: " + colors::getQColor(op.attribute("caption_color")).name() + "; ";
	stylesheet += "}";
	widget->setStyleSheet(stylesheet);
}

/**
 * @brief Add attributes to a given font.
 * @details This function can be used for setting the font of objects that are not Q_OBJECTs, such
 * as QListWidgetItems, as well as for objects where the stylesheet should not be disturbed.
 * @param[in] item_font The original font.
 * @param[in] options XML user options to parse.
 * @return The modified font; object.setFont() needs to be set to this from outside.
 */
QFont Atomic::setFontOptions(const QFont &item_font, const QDomElement &options)
{
	QFont retFont(item_font);
	retFont.setBold(options.attribute("bold").toLower() == "true");
	retFont.setItalic(options.attribute("italic").toLower() == "true");
	retFont.setUnderline(options.attribute("underline").toLower() == "true");
	if (!options.attribute("font").isEmpty())
		retFont.setFamily(options.attribute("font"));
	if (!options.attribute("font_size").isEmpty())
		retFont.setPointSize(options.attribute("font_size").toInt());
	return retFont;
}

/**
 * @brief Event handler for property changes.
 * @details Suitable panels override this to react to changes of the INI value from outside.
 */
void Atomic::onPropertySet()
{
	//Ignore this signal for unsuitable panels. Suitable ones have their own implementation.
}

/**
 * @brief Convert a number to string and pass to INI value setter.
 * @param[in] value The integer to convert.
 */
void Atomic::setIniValue(const int &value)
{
	setIniValue(QString::number(value));
}

/**
 * @brief Convert a number to string and pass to INI value setter.
 * @param[in] value The double value to convert.
 */
void Atomic::setIniValue(const double &value)
{
	setIniValue(QString::number(value));
}

/**
 * @brief Store this panel's current value in a uniform way.
 * @details Gets called after range checks have been performed, i. e. when we
 * want to propagate the change to the INI. It will be read by the main program when the
 * INI is being written out. It is called by changes to:
 *  - Values through users entering in the GUI
 *  - Default values in the XML
 *  - Values from an INI file
 * @param[in] value Set the current value of the panel.
 */
void Atomic::setIniValue(const QString &value)
{
	ini_value_ = value;

	//HACK this is needed as a workaround for KDE bug https://bugs.kde.org/show_bug.cgi?id=337491
#if defined Q_OS_LINUX || defined Q_OS_FREEBSD //we assume the kde is only used on Linux and FreeBSD
	ini_value_.replace("&", "");
#endif
}

/**
 * @brief Prepare the panel's custom context menu.
 */
void Atomic::createContextMenu()
{
	QAction *info_entry( panel_context_menu_.addAction(key_) );
	info_entry->setEnabled(false);
	panel_context_menu_.addSeparator();
	panel_context_menu_.addAction(tr("Reset to default"));
	panel_context_menu_.addAction(tr("Delete key"));
}

/**
 * @brief Re-enable GUI painting.
 * @details Cf. notes in setBufferedUpdatesEnabled().
 */
void Atomic::onTimerBufferedUpdatesEnabled()
{
	setUpdatesEnabled(true);
}

/**
 * @brief Show context menu to clear this panel.
 */
void Atomic::onConextMenuRequest(const QPoint &/*pos*/)
{
	if (key_.isEmpty())
		return;
	if (qobject_cast<Group *>(this)) //e. g. Selector/Replicator containers
		return;
	QAction *selected( panel_context_menu_.exec(QCursor::pos()) );
	if (selected) {
		if (selected->text() == tr("Reset to default"))
			clear();
		else if (selected->text() == tr("Delete key"))
			clear(false);
	}
}
