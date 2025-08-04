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

#include "Number.h"
#include "src/main/colors.h"
#include "src/main/expressions.h"
#include "src/main/inishell.h"

#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTimer>

#include <algorithm> //for max()
#include <climits> //for the number panel limits

#ifdef DEBUG
	#include <iostream>
#endif //def DEBUG

/**
 * @class KeyPressFilter
 * @brief Key press event listener for the Number panel.
 * @details We can not override 'event' in the panel itself because we want to
 * listen to the events of a child widget.
 * @param[in] object Object the event stems from (the SpinBox).
 * @param[in] event The type of event.
 * @return True if the event was accepted.
 */
bool KeyPressFilter::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		const QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
		if ((key_event->key() >= Qt::Key_0 && key_event->key() <= Qt::Key_9) || (key_event->key() == Qt::Key_Minus)) {
			if (object->property("empty").toBool()) {
				/*
				 * A Number panel's value may be hidden to mark the panel as not set.
				 * If a user starts entering a number via the keyboard however, then
				 * this hidden value contributes. Here, we prevent this.
				 */
				object->setProperty("empty", "false"); //necessary if entered number happens to be the hidden value
				if (auto *spinbox = qobject_cast<QSpinBox *>(object)) { //try both types
					spinbox->parent()->setProperty("ini_value", key_event->key() - Qt::Key_0);
					spinbox->style()->unpolish(spinbox);
					spinbox->style()->polish(spinbox);
				} else if (auto *spinbox = qobject_cast<QDoubleSpinBox *>(object)) {
					spinbox->parent()->setProperty("ini_value", key_event->key() - Qt::Key_0);
					spinbox->style()->unpolish(spinbox);
					spinbox->style()->polish(spinbox);
				}
				return true; //we have already input the value - prevent 2nd time
			} //endif property
		} //endif key_event
	}
	return QObject::eventFilter(object, event); //pass to actual event of the object
}

/**
 * @class Number
 * @brief Default constructor for a Number panel.
 * @details A number panel displays and manipulates a float or integer value.
 * It can also switch to free text mode, allowing to enter an expression (according to SLF software
 * syntax) which it will check.
 * @param[in] section INI section the controlled value belongs to.
 * @param[in] key INI key corresponding to the value that is being controlled by this Number panel.
 * @param[in] options XML node responsible for this panel with all options and children.
 * @param[in] no_spacers Keep a tight layout for this panel.
 * @param[in] parent The parent widget.
 */
Number::Number(const QString &section, const QString &key, const QDomNode &options, const bool &no_spacers,
    QWidget *parent) : Atomic(section, key, parent)
{
	/* number widget depending on which type of number to display */
	const QString format( options.toElement().attribute("format") );
	if (format == "decimal" || format.isEmpty()) {
		number_element_ = new QDoubleSpinBox;
		mode_ = NR_DECIMAL;
		//Note that the mode concerns only the QSpinBox, arithmetic expressions are decoupled
	} else if (format == "integer" || format == "integer+") {
		number_element_ = new QSpinBox;
		mode_ = (format == "integer")? NR_INTEGER : NR_INTEGERPLUS;
	} else {
		topLog(tr(R"(XML error: unknown number format for key "%1::%2")").arg(
		    section_, key_), "error");
		return;
	}
	QTimer::singleShot(1, this, [=]{ setEmpty(true); }); //indicate that even if 0 is displayed, nothing is set yet
	setPrimaryWidget(number_element_); //start with SpinBox and switch if needed
	auto *key_label( new Label(QString(), QString(), options, no_spacers, key_, this) );
	number_element_->setFixedWidth(Cst::width_number_min);
	key_filter_ = new KeyPressFilter;
	number_element_->installEventFilter(key_filter_);

	/* free text expression entering  */
	expression_element_ = new QLineEdit(this);
	expression_element_->hide();
	expression_element_->setFixedWidth(Cst::width_number_min);
	expression_element_->setToolTip(number_element_->toolTip());
	connect(expression_element_, &QLineEdit::textChanged, this, &Number::checkStrValue);

	/* switch button and layout for number element plus button */
	switch_button_ = new QToolButton;
	connect(switch_button_, &QToolButton::toggled, this, &Number::switchToggle);
	switch_button_->setAutoRaise(true);
	switch_button_->setCheckable(true);
	switch_button_->setStyleSheet("QToolButton:checked {background-color: " +
	    colors::getQColor("number").name() + "}");
	switch_button_->setIcon(getIcon("displaymathmode"));
	switch_button_->setToolTip(tr("Enter an expression such as ${other_ini_key}, ${env:my_env_var} or ${{arithm. expression}}"));

	switcher_layout_ = new QHBoxLayout;
	switcher_layout_->addWidget(number_element_, 0, Qt::AlignLeft);
	switcher_layout_->addWidget(switch_button_);
	if (options.toElement().attribute("notoggle").toLower() == "true")
		switch_button_->hide();

	/* layout of basic elements */
	auto *number_layout( new QHBoxLayout );
	setLayoutMargins(number_layout);
	number_layout->addLayout(switcher_layout_);
	if (!no_spacers)
		number_layout->addSpacerItem(buildSpacer()); //keep widgets to the left
	addHelp(number_layout, options, no_spacers);

	/* main layout */
	auto *layout( new QHBoxLayout );
	setLayoutMargins(layout);
	if (!key_label->isEmpty())
		layout->addWidget(key_label);
	layout->addLayout(number_layout);
	this->setLayout(layout);

	setOptions(options); //min, max, default, ...
}

/**
 * @brief The destructor with minimal cleanup.
 */
Number::~Number()
{
	delete key_filter_;
}

/**
 * @brief Check if the Number value is mandatory or currently at the default value.
 * @details Usually this is handled in the Atomic base class, but here we want to
 * perform numeric checks instead of string comparison.
 * @param[in] in_value The current value of the panel.
 */
void Number::setDefaultPanelStyles(const QString &in_value)
{
	setPanelStyle(FAULTY, false); //first we disable temporary styles
	setPanelStyle(VALID, false);
	bool is_default = false;
	bool success_inval, success_default;
	double inval = in_value.toDouble(&success_inval);
	double defval = this->property("default_value").toDouble(&success_default);
	is_default = (success_inval && success_default && qFuzzyCompare(inval, defval));

	setPanelStyle(DEFAULT, is_default && !this->property("default_value").isNull() && !in_value.isNull());
	if (this->property("is_mandatory").toBool())
		setPanelStyle(MANDATORY, in_value.isEmpty());
}

/**
 * @brief Reset both input types, then re-set the default value.
 * @param[in] set_default If true, reset the value to default. If false, delete the key.
 */
void Number::clear(const bool &set_default)
{
	QString def_number_val;
	if (auto *spinbox = qobject_cast<QSpinBox *>(number_element_)) {
		if (spinbox->minimum() > 0)
			def_number_val = QString::number(spinbox->minimum());
		else if (spinbox->maximum() < 0)
			def_number_val = QString::number(spinbox->maximum());
		else
			def_number_val = "0";
		spinbox->setValue(def_number_val.toInt());
	} else if (auto *spinbox = qobject_cast<QDoubleSpinBox *>(number_element_)) {
		if (spinbox->minimum() > 0)
			def_number_val = QString::number(spinbox->minimum());
		else if (spinbox->maximum() < 0)
			def_number_val = QString::number(spinbox->maximum());
		else
			def_number_val = "0";
		spinbox->setValue(def_number_val.toDouble());
	}
	expression_element_->setText(QString());

	QString default_value;
	if (set_default)
		default_value = (this->property("default_value").toString());
	if (default_value.isEmpty()) {
		if (switch_button_->isChecked())
			switch_button_->animateClick();
	}

	this->setProperty("ini_value", ini_value_);
	this->setProperty("ini_value", default_value.isEmpty()? def_number_val : default_value);
	if (default_value.isEmpty()) {
		setIniValue(QString());
		QTimer::singleShot(1, this, [=]{ setEmpty(true); });
	}

	setDefaultPanelStyles(set_default? default_value : QString());
}

/**
 * @brief Parse options for a Number panel from XML.
 * @param[in] options XML node holding the Number panel.
 */
void Number::setOptions(const QDomNode &options)
{
	const QDomElement element(options.toElement());
	const QString maximum( element.attribute("max") );
	const QString minimum( element.attribute("min") );
	const QString unit( element.attribute("unit") );
	show_sign = (element.attribute("sign").toLower() == "true");

	if (mode_ == NR_DECIMAL) {
		auto *spinbox( qobject_cast<QDoubleSpinBox *>(number_element_) ); //cast for members

		/* precision */
		if (!element.attribute("precision").isNull()) {
			bool precision_success;
			precision_ = static_cast<int>(element.attribute("precision").toUInt(&precision_success));
			if (!precision_success) {
				topLog(tr(R"(XML error: Could not extract precision for Number key "%1::%2")").arg(
				    section_, key_), "error");
				precision_ = default_precision_;
			} else { //the XML declared precision becomes the new default for this field
				default_precision_ = precision_;
			}
		}
		spinbox->setDecimals(precision_);

		/* minimum and maximum, choose whole range if they aren't set */
		bool success = true;
		double min = minimum.isEmpty()? std::numeric_limits<double>::lowest() : minimum.toDouble(&success);
		if (!success)
			topLog(tr(R"(XML error: Could not parse minimum double value for key "%1::%2")").arg(
			    section_, key_), "error");
		double max = maximum.isEmpty()? std::numeric_limits<double>::max() : maximum.toDouble(&success);
		if (!success)
			topLog(tr(R"(XML error: Could not parse maximum double value for key "%1::%2")").arg(
			    section_, key_), "error");
		spinbox->setRange(min, max);
		if (element.attribute("wrap").toLower() == "true") //circular wrapping when min/max is reached
			spinbox->setWrapping(true);

		/* unit and sign */
		if (!unit.isEmpty()) //for the space before the unit
			spinbox->setSuffix(" " + unit);
		if (show_sign)
			spinbox->setPrefix("+"); //for starting 0

		connect(number_element_, SIGNAL(valueChanged(const double &)), this,
		    SLOT(checkValue(const double &)));
	} else { //NR_INTEGER || NR_INTEGERPLUS
		auto *spinbox( qobject_cast<QSpinBox *>(number_element_) );

		/* minimum, maximum and wrapping */
		bool success = true;
		int min = 0; //for integer+
		if (mode_ == NR_INTEGER)
			min = minimum.isEmpty()? std::numeric_limits<int>::lowest() : minimum.toInt(&success);
		if (!success)
			topLog(tr(R"(XML error: Could not parse maximum integer value for key "%1::%2")").arg(
			    section_, key_), "error");
		int max = maximum.isEmpty()? std::numeric_limits<int>::max() : maximum.toInt(&success);
		if (!success)
			topLog(tr(R"(XML error: Could not parse maximum integer value for key "%1::%2")").arg(
			    section_, key_), "error");
		spinbox->setRange(min, max);
		if (element.attribute("wrap").toLower() == "true") //circular wrapping when min/max is reached
			spinbox->setWrapping(true);

		/* unit */
		if (!unit.isEmpty())
			spinbox->setSuffix(" " + unit);
		if (show_sign)
			spinbox->setPrefix("+"); //for starting 0
		connect(number_element_, SIGNAL(valueChanged(const int &)), this,
		    SLOT(checkValue(const int &)));
	} //endif format

	/* allow to set "empty" via the property system */
	QString bg_color( colors::getQColor("app_bg").name() );
	//find font color to use for hidden spinbox text dependent on background color:
	if (options.toElement().attribute("optional").toLower() == "false")
		bg_color = colors::getQColor("mandatory").name();
	number_element_->setStyleSheet("* [empty=\"true\"] {color: " + bg_color + "}");

	//user-set substitutions in expressions to style custom keys correctly:
	substitutions_ = expr::parseSubstitutions(options);

}

/**
 * @brief Extract how many decimals a number given as string has.
 * @details Looks for "," or ".".
 * @param[in] str_number The number in a string.
 */
int Number::getPrecisionOfNumber(const QString &str_number) const
{
	const QStringList dec( str_number.split(QRegExp("[,.]")) );
	if (dec.size() > 1) //there's a decimal sign
		return dec.at(1).length();
	return 0; //integer
}

/**
 * @brief Set an empty value for the QSpinBox.
 * @details The QSpinBox starts up with the minimum value because it has to display something,
 * even if it should be empty. Here we try to hide this value as best as we can. Qt's mechanism
 * for this (special value = special meaning) is not ideal for unbounded spin boxes.
 * @param[in] is_empty Hide text if true, show if false.
 */
void Number::setEmpty(const bool &is_empty)
{
	number_element_->setProperty("empty", is_empty);
	this->style()->unpolish(number_element_);
	this->style()->polish(number_element_);
}

/**
 * @brief Perform checks on the entered number.
 * @details Default and INI file numbers are already checked in onPropertySet(), because if we
 * receive a number here it comes from a range controlled QSpinBox and is therefore valid.
 * @param[in] to_check The double value to check.
 */
void Number::checkValue(const double &to_check)
{
	if (show_sign) {
		auto *spinbox = dynamic_cast<QDoubleSpinBox *>(number_element_);
		spinbox->setPrefix(to_check >= 0? "+" : "");
	}

	setDefaultPanelStyles(QString::number(to_check));
	//once something is entered it counts towards the INI file (after stylesheets):
	QTimer::singleShot(1, this, [=]{ setEmpty(false); });
	setIniValue(QString::number(to_check, 'f', precision_));
}

/**
 * @brief Perform checks on the entered integer number.
 * @details Coming from a QSpinBox, this value is already validated.
 * @param[in] to_check The integer value to check.
 */
void Number::checkValue(const int &to_check)
{
	if (show_sign) {
		auto *spinbox = dynamic_cast<QSpinBox *>(number_element_);
		spinbox->setPrefix(to_check >= 0? "+" : "");
	}
	setDefaultPanelStyles(QString::number(to_check));
	QTimer::singleShot(1, this, [=]{ setEmpty(false); });
	setIniValue(to_check);

}

/**
 * @brief Check an expression entered in free text mode.
 * @details This function checks for an expression accepted by SLF software, and if positive,
 * sets styles according to if the evaluation of said expression was successful or not.
 * @param[in] str_check The string to check for a valid expression.
 */
void Number::checkStrValue(const QString &str_check)
{
	bool evaluation_success;
	setDefaultPanelStyles(str_check);

	if (expr::checkExpression(str_check, evaluation_success, substitutions_) || !evaluation_success)
		setValidPanelStyle(evaluation_success);
	QTimer::singleShot(1, [=]{ setEmpty(false); });
	setIniValue(str_check); //it is just a hint - save anyway
}

/**
 * @brief Check if a string is free text for an expression or a number.
 * @details This function is used to check which mode to enter. Since some keys can have
 * hardcoded substitutions for numbers we allow all text to switch free text mode, not just
 * expressions.
 * @param[in] expression The string to check for an number.
 * @return True if the string represents a number.
 */
bool Number::isNumber(const QString &expression) const
{ //note that this does not catch scientific notation, meaning it will be written out as such again
	static const QRegularExpression regex_number(R"(^(?=.)([+-]?([0-9]*)(\.([0-9]+))?)$)");
	const QRegularExpressionMatch match(regex_number.match(expression));
	return (match.captured(0) == expression);
}

/**
 * @brief Event listener for changed INI values.
 * @details The "ini_value" property is set when parsing default values and potentially again
 * when setting INI keys while parsing a file.
 */
void Number::onPropertySet()
{
	const QString str_value( this->property("ini_value").toString() );
	if (ini_value_ == str_value)
		return;

	if (!isNumber(str_value)) { //free text mode --> switch element and delegate checks
		expression_element_->setText(str_value);
		switch_button_->setChecked(true);
		return;
	}

	if (auto *spinbox = qobject_cast<QSpinBox *>(number_element_)) { //integer
		bool convert_success;
		int ival = str_value.toInt(&convert_success);
		if (!convert_success) { //could also stem from XML, but let's not clutter the message for users
			topLog(tr(R"(Could not convert INI value to integer for key "%1::%2")").arg(
			    section_, key_), "warning");
			topStatus(tr("Invalid numeric INI value"), "warning");
			return;
		}
		if (ival < spinbox->minimum() || ival > spinbox->maximum()) {
			topLog(tr(R"(Integer INI value out of range for key "%1::%2" - truncated)").arg(
			    section_, key_), "warning");
			topStatus(tr("Truncated numeric INI value"), "warning");
		}
		spinbox->setValue(str_value.toInt());
		if (ival == spinbox->minimum()) {
			emit checkValue(ival); //if the default isn't changed from zero then nothing would be emitted
			QTimer::singleShot(1, this, [=]{ setEmpty(false); }); //avoid keeping empty style when default val is minimum spinbox val
		}
	} else if (auto *spinbox = qobject_cast<QDoubleSpinBox *>(number_element_)) { //floating point
		bool convert_success;
		double dval = str_value.toDouble(&convert_success);
		if (!convert_success) {
			topLog(tr(R"(Could not convert INI value to double for key "%1::%2")").arg(
			    section_, key_), "warning");
			topStatus(tr("Invalid numeric INI value"), "warning");
			return;
		}
		if (dval < spinbox->minimum() || dval > spinbox->maximum()) {
			topLog(tr(R"(Double INI value out of range for key "%1::%2" - truncated)").arg(
			    section_, key_), "warning");
			topStatus(tr("Truncated numeric INI value"), "warning");
		}

		const int ini_precision = getPrecisionOfNumber(str_value); //read number of decimal in INI
		//this also enables to overwrite in expression mode; no default in XML --> use smaller ones too:
		if (ini_precision > spinbox->decimals()) {
			precision_ = ini_precision;
			spinbox->setDecimals(precision_);
		}
		//allow to switch back to a smaller number of digits for new INI files:
		if (spinbox->decimals() > std::max(ini_precision, default_precision_)) {
			precision_ = std::max(ini_precision, default_precision_);
			if (precision_ == 0)
				precision_ = 1; //force at least 1 digit
			spinbox->setDecimals(precision_);
		}

		spinbox->setValue(str_value.toDouble());
		spinbox->setDecimals(precision_); //needs to be re-set every time

		if (qFuzzyCompare(dval, spinbox->minimum())) { //fuzzy against warnings
			emit checkValue(dval); //cf. above
			QTimer::singleShot(1, this, [=]{ setEmpty(false); });
		}
		//At this point the INI value is already set, but since this is coming from an actual INI
		//file (or the XML) we reset to the exact value to have the same precision and circumvent
		//"unsaved changes" warnings resp. a numeric check in the INIParser's == operator:
		setIniValue(str_value);
	}
}

/**
 * @brief Toggle between number and (arithmetic) expression mode.
 * @details This function shows/hides the spin box/text field and initiates the necessary checks.
 * @param[in] checked True if entering expression mode.
 */
void Number::switchToggle(bool checked)
{
	if (checked) { //(arithmetic) expression / free text mode
		switcher_layout_->replaceWidget(number_element_, expression_element_);
		number_element_->hide();
		expression_element_->show();
		setPrimaryWidget(expression_element_); //switch widget to style with properties
		setIniValue(expression_element_->text()); //always use the visible number in INI
		setDefaultPanelStyles(expression_element_->text());
		checkStrValue(expression_element_->text());
	} else { //spin box mode
		switcher_layout_->replaceWidget(expression_element_, number_element_);
		expression_element_->hide();
		number_element_->show();
		setPrimaryWidget(number_element_);
		if (mode_ == NR_DECIMAL) {
			setIniValue(dynamic_cast<QDoubleSpinBox *>(number_element_)->value());
			setDefaultPanelStyles(
			    QString::number(dynamic_cast<QDoubleSpinBox *>(number_element_)->value()));
		} else {
			setIniValue(dynamic_cast<QSpinBox *>(number_element_)->value());
			setDefaultPanelStyles(
			    QString::number(dynamic_cast<QSpinBox *>(number_element_)->value()));
		}
	}
}
