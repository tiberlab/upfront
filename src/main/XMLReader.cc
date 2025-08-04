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

#include "XMLReader.h"
#include "src/main/colors.h"
#include "src/main/common.h"
#include "src/main/inishell.h"

#include <QCoreApplication> //for translations
#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>

/**
 * @brief Add a dummy parent node to an XML node for when a function expects to iterate through children.
 * @param[in] child XML node to add a parent for.
 * @return XML node with added dummy parent.
 */
QDomNode prependParent(const QDomNode &child)
{
	static const QByteArray parent_array("<dummy_parent></dummy_parent>");
	QDomDocument parent_doc;
	parent_doc.setContent(parent_array, false);
	parent_doc.firstChildElement().appendChild(child.cloneNode(true));
	return parent_doc.firstChildElement();
}

/**
 * @class XMLReader
 * @brief Default constructor for an XML reader.
 * @details This constructor takes a file name as input and parses its contents.
 * @param[in] filename XML file to read.
 * @param[in] no_references If true, do not perform schema validation, or run include file and
 * reference systems. Set this if you parse for example the settings file which would fail
 * schema validation.
 * @param[out] xml_error XML operations error string to add to.
 */
XMLReader::XMLReader(const QString &filename, QString &xml_error, const bool &no_references)
{
	this->read(filename, xml_error, no_references);
}

/**
 * @brief Parse XML contents of a file.
 * @param[in] filename XML file to read.
 * @param[out] xml_error xml_error XML operations error string to add to.
 * @param[in] no_references If set to true, no reference tags will be resolved
 * (unneccessary e. g. for INIshell's settings file).
 * @param[in] no_references If true, turn off schema validation, include and reference systems.
 * @return An INI file name to open automatically if requested in the XML.
 */
QString XMLReader::read(const QString &filename, QString &xml_error, const bool &no_references)
{
	QFile infile(filename);
	//remember the main XML file from which the parser could cascade into includes:
	master_xml_file_ = filename;
	if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		xml_error = QApplication::tr(R"(XML error: Could not open file "%1" for reading (%2))").arg(
		    QDir::toNativeSeparators(filename), infile.errorString()) + "\n";
		return QString();
	}
	QString autoload_ini( this->read(infile, xml_error, no_references) );
	infile.close();
	return autoload_ini;
}

/**
 * @brief Parse XML contents of a file.
 * @param[in] file XML file to read.
 * @param[out] xml_error XML operations error string to add to.
 * @param[in] no_references If true, don't resolve include tags.
 * @return An INI file name to open automatically if requested in the XML.
 */
QString XMLReader::read(QFile &file, QString &xml_error, const bool &no_references)
{
	QString error_msg;
	int error_line, error_column;
	xml_error = QString();

	if (!xml_.setContent(&file, false, &error_msg, &error_line, &error_column)) {
		xml_error = QString(QCoreApplication::tr(
		    "XML error: %1 (line %2, column %3)")).arg(error_msg).arg(error_line).arg(error_column) + "\n";
	}

	if (no_references) //e. g. static GUI settings file
		return QString();

	parseIncludes(xml_, master_xml_file_, xml_error); //"<include file='...'/>" tags
	validateSchema(xml_error);
	parseReferences(); //"<reference name='...'/>" tags
	return parseAutoloadIni();
}

/**
 * @brief Parse references within the XML document.
 * @details This function parses our own reference syntax and can inject parts of the XML into
 * other nodes, for example a list of parameter names.
 */
void XMLReader::parseReferences()
{
	/*
	 * The node that will be subsituted into a reference is referred to via the "parametergroup" tag,
	 * and the one referencing via the "reference" tag.
	 * Ex.:
	 * <reference name="PARAMS"/>
	 * <parametergroup name="PARAMS>
	 *     <option value="TA"/>
	 *     <option value="RH"/>
	 * </parametergroup>
	 */
	const QDomNodeList par_groups( getXml().elementsByTagName("parametergroup") );

	while (true) {
		const QDomNodeList to_substitute( getXml().elementsByTagName("reference") ); //find at all levels
		if (to_substitute.isEmpty()) //no more substitutions left
			break;
		const QString sub_name( to_substitute.at(0).toElement().attribute("name") );
		bool found_reference = false;
		for (int jj = 0; jj < par_groups.count(); ++jj) { //check nodes we can refer to
			if (QString::compare(par_groups.at(jj).toElement().attribute("name"), sub_name, Qt::CaseInsensitive) == 0) {
				found_reference = true;
				const QDomDocumentFragment replacement( fragmentFromNodeChildren(par_groups.at(jj)) );
				const QDomNode success( to_substitute.at(0).parentNode().replaceChild(
				    replacement, to_substitute.at(0)) );
				if (success.isNull()) { //should never happen
					topLog(QCoreApplication::tr(R"(XML error: Replacing a node failed for parametergroup "%1".)").arg(
					    sub_name), "error");
					return; //avoid endless loop
				}
				break;
			}
		}
		if (!found_reference) {
			to_substitute.at(0).parentNode().removeChild(to_substitute.at(0)); //remove unavailable include node
			topLog(QCoreApplication::tr(R"(XML error: Replacement parametergroup "%1" not found.)").arg(
			    sub_name), "error");
		}
	} //end while
}

/**
 * @brief Parse include files in an XML.
 * @details This function scans an XML document for our own simple include system.
 * @param[in] xml_ XML document to check for include files.
 * @param[in] parent_file The parent file this file is included in.
 * @param[out] xml_error XML operations error string to add to.
 */
void XMLReader::parseIncludes(const QDomDocument &xml_, const QString &parent_file, QString &xml_error)
{
	/*
	 * XML documents are scanned for the "<include file="..."/>" tag. If found, the file
	 * is opened, parsed, and copied into the main XML. File paths remain relative to
	 * their including parent file.
	 * The include file must be a valid XML file, so it requires a root node where all
	 * children go.
	 */

	//extract path from including parent file as anchor for inclusion file names:
	const QFileInfo parent_finfo( parent_file );
	const QString parent_path( parent_finfo.absolutePath() );

	//We use a while loop here and re-search "include" nodes after each insertion, because the
	//replacements invalidate XML iterators of a for loop.
	while (true) {
		const auto include_element( xml_.firstChildElement().firstChildElement("include") );
		if (include_element.isNull())
			break;
		const QString include_file_name( include_element.toElement().attribute("file") );

		QFile include_file;
		if (QDir::isAbsolutePath(include_file_name))
			include_file.setFileName(include_file_name);
		else
			include_file.setFileName(parent_path + "/" + include_file_name);

		if (include_file.open(QIODevice::ReadOnly)) {
			QDomDocument inc; //the new document to include
			QString error_msg;
			int error_line, error_column;
			if (!inc.setContent(include_file.readAll(), false, &error_msg, &error_line, &error_column)) {
				xml_error += QString(QCoreApplication::tr(
				    R"(XML error: [Include file "%1"] %2 (line %3, column %4))")).arg(
				    QDir::toNativeSeparators(include_file_name), error_msg).arg(error_line).arg(error_column) + "\n";
			}
			//We need a full QDomDocument (inc) to parse an XML file, which we now transform
			//to a document fragment, because fragments do exactly what we want: When replacing
			//a node with a fragment, all the fragment's children are inserted into the parent:
			const QDomDocument inc_doc( inc.toDocument() ); //point to inc as document
			parseIncludes(inc_doc, include_file.fileName(), xml_error); //recursive inclusions
			const QDomDocumentFragment new_fragment( fragmentFromNodeChildren(inc.firstChildElement()) );
			//perform actual replacement:
			const QDomNode success( include_element.parentNode().replaceChild(new_fragment, include_element) );

			if (success.isNull()) { //should never happen
				topLog(QCoreApplication::tr(R"(XML error: Replacing a node failed for inclusion system in master file "%1")").arg(
				    include_file.fileName()), "error");
				return; //endless loop otherwise
			}
		} else {
			xml_error += QCoreApplication::tr(
			    "XML error: Unable to open XML include file \"%1\" for reading (%2)\n").arg(
			    QDir::toNativeSeparators(parent_path + "/" + include_file_name), include_file.errorString());
			return;
		}
	} //endfor include_element
}

/**
 * @brief Check XML file for an autoload INI tag.
 * @return The INI file name to autoload.
 */
QString XMLReader::parseAutoloadIni() const
{
	const QDomNode autoload( getXml().firstChildElement().firstChildElement("autoload") );
	if (!autoload.isNull())
		return QFileInfo( master_xml_file_ ).absolutePath() +
		    "/" + autoload.toElement().attribute("inifile");
	return QString();
}

/**
 * @brief Convert list of node children to a XML document fragment.
 * @details .toDocumentFragment() only works if the node is already a fragment, so we need
 * to loop and copy.
 * @param[in] node Node of which the children will be listed in the fragment.
 * @return The created XML document fragment.
 */
QDomDocumentFragment XMLReader::fragmentFromNodeChildren(const QDomNode &node)
{
	QDomDocumentFragment fragment( xml_.createDocumentFragment() );
	for (QDomNode nd = node.firstChildElement(); !nd.isNull(); nd = nd.nextSibling())
		fragment.appendChild(nd.cloneNode(true));
	return fragment;
}

/**
 * @brief Provide the XMLReader's XML document.
 * @return The XMLReader's XML document.
 */
QDomDocument XMLReader::getXml() const
{
	return xml_;
}

/**
 * @brief Perform schema validation on an XML file.
 * @param[out] xml_error XML operations error string to add to.
 */
void XMLReader::validateSchema(QString &xml_error) const
{
	/*
	 * The logger already displays a lot of info and the XML parser itself also logs errors,
	 * but what it doesn't do is check for outdated parameter names, unknown attributes, etc.,
	 * so the schema validation is still useful.
	 */
	MessageHandler msgHandler;
	QXmlSchema schema;
	schema.setMessageHandler(&msgHandler);
	schema.load( QUrl("qrc:config_schema.xsd") );
	if (!schema.isValid()) {
		xml_error += QCoreApplication::tr("XML error: there is an error in the internal xsd file. Skipping schema validation.") + "\n";
	} else {
		const QXmlSchemaValidator validator(schema);
		const bool validation_success = validator.validate(xml_.toString().toUtf8());
		if (!validation_success) {
			QTextDocument error_msg;
			error_msg.setHtml(msgHandler.status()); //strip HTML
			xml_error += "[XML error: schema validation failed] " + error_msg.toPlainText() +
			    QString(" (line %1, column %2)").arg(msgHandler.line(), msgHandler.column()) + "\n";
		}
	}
}
