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

/*
 * This file provides some common mostly light-weight functionalty across INIshell.
 * 2019-11
 */

#ifndef COMMON_H
#define COMMON_H

#include <QIcon>
#include <QKeyEvent>
#include <QKeySequence>
#include <QString>
#include <QStringList>
#include <QtXml>
#include <QtXmlPatterns/QAbstractMessageHandler>

namespace html {

QString bold(const QString &text);
QString color(const QString &text, const QString &color);

} //namespace html

/**
 * @struct CaseInsensitiveCompare
 * @brief A weak ordered comparison struct for case insensitive key-value mapping.
 * @details This struct can be used in containers for key lookup.
 */
struct CaseInsensitiveCompare {
	bool operator() (const QString &first_str, const QString &second_str) const
	{ //<0: less than; =0: equal; >0: greater than; STL comparison is done via a less-type operator
		return (QString::compare(first_str, second_str, Qt::CaseInsensitive) < 0);
	}
};

/**
 * @class MessageHandler
 * @brief Message handler to conveniently retrieve Qt internal messages.
 * @details This class is used for example to get XML schema validation errors.
 */
class MessageHandler : public QAbstractMessageHandler {
	public:
		MessageHandler() : QAbstractMessageHandler(nullptr) {}
		QString status() const { return description_; }
		int line() const { return static_cast<int>(location_.line()); }
		int column() const { return static_cast<int>(location_.column()); }

	protected:
		void handleMessage(QtMsgType type, const QString &description,
		    const QUrl &identifier, const QSourceLocation &location) override;

	private:
		QString description_;
		QSourceLocation location_;
};

/**
 * @brief Check if an XML node has a certain INI section associated with i.
 * @param[in] section Check if this section is present.
 * @param[in] options XML node to check for the section.
 * @return True if the section is available.
 */
inline bool hasSectionSpecified(const QString &section, const QDomElement &options)
{
	/*
	 * Sections can be specified in an attribute and also as a separate element. This function
	 * checks if either is true for a given section and panel and tells the element factory that
	 * the panel should be constructed. This is useful if multiple sections are given, but not
	 * for every element that follows.
	 * (Furthermore, they could be in a dedicated <section>...</section> node,
	 * but then the section is fixed to a single one.)
	 */
	if (!options.attribute("section").isNull()) //<section name="name"/>
		return (QString::compare(options.attribute("section"), section, Qt::CaseInsensitive) == 0);
	int counter = 0;
	for (QDomElement section_element = options.firstChildElement("section"); !section_element.isNull();
	    section_element = section_element.nextSiblingElement("section")) { //read all <section> tags
		counter++; //"<parameter key=... section="name">
		if (QString::compare(section_element.attribute("name"), section, Qt::CaseInsensitive) == 0)
			return true;
	}
	return (counter == 0); //no section specified means all sections are good to go
}

inline QIcon getIcon(const QString& icon_name)
{
#ifdef Q_OS_WIN
	return QIcon(":/icons/flat-bw/svg/"+icon_name+".svg");
#endif
#ifdef Q_OS_MAC
	return QIcon(":/icons/elementary/svg/"+icon_name+".svg");
#endif
#if !defined Q_OS_WIN && !defined Q_OS_WIN //on Linux, try to use system icons
	return QIcon::fromTheme(icon_name, QIcon(":/icons/flat-bw/svg/"+icon_name+".svg"));
#endif
}

QStringList getSearchDirs(const bool &include_user_set = true, const bool &include_nonexistent_folders = true);
QKeySequence keyToSequence(QKeyEvent *event);


#endif //COMMON_H
