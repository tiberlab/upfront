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
 * The top level XML interface.
 * This class is an XMLReader which reads from the file system into XML.
 * 2019-10
 */

#ifndef XMLREADER_H
#define XMLREADER_H

#include <QFile>
#include <QMap>
#include <QString>
#include <QtXml>

#ifdef DEBUG
	#include <QTextStream>
	#include <iostream>
#endif //def DEBUG

QDomNode prependParent(const QDomNode &child);

class XMLReader {
	public:
		XMLReader() = default;
		XMLReader(const QString &filename, QString &xml_error, const bool &no_references = false);
		QString read(QFile &file, QString &xml_error, const bool &no_references = false);
		QString read(const QString &filename, QString &xml_error, const bool &no_references = false);
		void parseReferences();
		void parseIncludes(const QDomDocument &xml_, const QString &parent_file, QString &xml_error);
		QString parseAutoloadIni() const;
		QDomDocumentFragment fragmentFromNodeChildren(const QDomNode &node);
		QDomDocument getXml() const;
#ifdef DEBUG
		template<class T>
		static void debugPrintNode(T node) { //call with QDomElement or QDomNode
			QString node_txt;
			QTextStream stream(&node_txt);
			node.save(stream, 4); //indentation
			std::cout << (node_txt.isEmpty()? "- empty -" : node_txt.toStdString()) << std::endl;
		}
		static void debugPrintNode(const QDomDocument &node) { //call with QDomDocument
			std::cout << node.toString().toStdString() << std::endl;
		}
#endif //def DEBUG

	private:
		void validateSchema(QString &xml_error) const;

		QString master_xml_file_;
		QDomDocument xml_;
};

#endif //XMLREADER_H
