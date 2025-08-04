/*****************************************************************************/
/*  Copyright 2020 ALPsolut.eu                                               */
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

#include "Datepicker.h"
#include "Label.h"
#include "src/main/colors.h"

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class DateKeyPressFilter
 * @brief Key press event listener for the Datepicker panel.
 * @details We can not override 'event' in the panel itself because we want to
 * listen to the events of a child widget.
 * @param[in] object Object the event stems from (the QDateTimeEdit).
 * @param[in] event The type of event.
 * @return True if the event was accepted.
 */
bool DateKeyPressFilter::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		const QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
		if (key_event->key() != Qt::Key_Tab) {
			if (object->property("empty").toBool()) {
				/*
				 * A Datepicker panel's value may be hidden to mark the panel as not set.
				 * If a user starts entering something we reveal the value as the current date.
				 */
				object->setProperty("empty", "false");
				auto *panel = qobject_cast<QDateTimeEdit*>(object);
				panel->parent()->setProperty("ini_value", QDateTime::currentDateTime().toString());
				panel->style()->unpolish(panel);
				panel->style()->polish(panel);
			} //endif property
		} //endif key_event
	}
	return QObject::eventFilter(object, event); //pass to actual event of the object
}

/**
 * @class Datepicker
 * @brief Default constructor for a Datepicker.
 * @details A Datepicker is used to select/enter a date and time.
 * @param[in] key INI key corresponding to the value that is being controlled by this Datepicker.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Datepicker::Datepicker(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* label and text box */
	auto *key_label( new Label(QString(), QString(), options, no_spacers, key_, this) );
	datepicker_ = new QDateTimeEdit; 
	setPrimaryWidget(datepicker_);
	datepicker_->setCalendarPopup(true);
	datepicker_->setToolTip(tr("Pick a date/time"));
	QTimer::singleShot(1, this, [=]{ setEmpty(true); });
	connect(datepicker_, &QDateTimeEdit::dateTimeChanged, this, &Datepicker::checkValue);

	key_filter_ = new DateKeyPressFilter;
	datepicker_->installEventFilter(key_filter_);

	/* main layout */
	auto *datepicker_layout( new QHBoxLayout );
	setLayoutMargins(datepicker_layout);
	datepicker_layout->addWidget(key_label, 0, Qt::AlignLeft);
	datepicker_layout->addWidget(datepicker_, 0, Qt::AlignLeft);
	if (!no_spacers)
		datepicker_layout->addSpacerItem(buildSpacer());
	addHelp(datepicker_layout, options, no_spacers);
	this->setLayout(datepicker_layout);
	setOptions(options);
}

/**
 * @brief The destructor with minimal cleanup.
 */
Datepicker::~Datepicker()
{
	delete key_filter_;
}

/**
 * @brief Parse options for a Datepicker from XML.
 * @param[in] options XML node holding the Datepicker.
 */
void Datepicker::setOptions(const QDomNode &options)
{
	const QString user_date_format( options.toElement().attribute("format") );
	if (!user_date_format.isEmpty())
		date_format_ = user_date_format;
	datepicker_->setDisplayFormat(date_format_);

	/* allow to set "empty" via the property system */
	QString bg_color( colors::getQColor("app_bg").name() );
	//find font color to use for hidden datepicker dependent on background color:
	if (options.toElement().attribute("optional").toLower() == "false")
		bg_color = colors::getQColor("mandatory").name();
	datepicker_->setStyleSheet("* [empty=\"true\"] {color: " + bg_color + "}");
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void Datepicker::onPropertySet()
{
	const QString text_to_set( this->property("ini_value").toString() );
	if (ini_value_ == text_to_set)
		return;

	/*
	 * In checkValue() we always receive a date because the widget can't be empty.
	 * So we catch this case early here (where an empty value can come from XML
	 * for example) and style "empty" manually.
	 */
	if (text_to_set.isEmpty()) {
		QTimer::singleShot(1, this, [=]{ setEmpty(true); });
		setIniValue(QString());
		setDefaultPanelStyles(QString());
		return;
	}
	
	QDateTime dt( QDateTime::fromString(text_to_set, date_format_) );
	if (!dt.isValid()) {
		dt.setDate(QDate::currentDate());
		dt.setTime(QTime()); //round to start of current day
	}

	datepicker_->setDateTime(dt);
	checkValue(dt);
}

/**
 * @brief Sets the entered date to the ini.
 * @param[in] text The current text to check.
 */
void Datepicker::checkValue(const QDateTime &datetime)
{
	const QString date_text( datetime.toString(date_format_) ); //retrieve in user format
	setDefaultPanelStyles(date_text);
	setIniValue(date_text);
	QTimer::singleShot(1, this, [=]{ setEmpty(false); });
}

/**
 * @brief Set an empty value for the QDateTimeEdit.
 * @details See the same function in Number.cc.
 * @param[in] is_empty Hide text if true, show if false.
 */
void Datepicker::setEmpty(const bool &is_empty)
{
	datepicker_->setProperty("empty", is_empty);
	this->style()->unpolish(datepicker_);
	this->style()->polish(datepicker_);
}
