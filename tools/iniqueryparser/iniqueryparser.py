###############################################################################
#   Copyright 2019 WSL Institute for Snow and Avalanche Research  SLF-DAVOS   #
###############################################################################
# This file is part of INIshell.
# INIshell is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# INIshell is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with INIshell.  If not, see <http://www.gnu.org/licenses/>.

# Quick script to parse SLF's code base for the use of INI file queries.
# The goal is to get a list of INI keys the software uses, and compare that to
# the INI keys found in the INIshell XMLs to get hints for missing documentation.
# Synopsis: python3 iniqueryparser.py
# Cf. parseSourceFile() to add new syntax patterns.
# Michael Reisecker, 2019-12

import os, re

def getCodeBaseFiles(listing_file):
	"""Read files of code base to parse from a settings file.

    Keyword arguments:
    listing_file -- The settings file with file extensions, and a file list
    """
	extensions = list()
	files = list()
	exclusions = list()
	ignore_keys = list()
	xml_path = None
	base_path = ''

	infile = open(listing_file, 'r')
	file_content = infile.read().splitlines()

	for line in file_content:
		if not line or line.startswith('#'): #skip empty lines and comments
			continue
		if line.startswith('xmlpath='):
			xml_path = line.split("=")[1]
			continue
		if line.startswith('extensions='): #line holds the extensions
			extensions.extend(line.split('=')[1].split(',')) #format "extensions=ext,ext2,..."
			continue
		if line.startswith('ignore='): #keys to ignore
			ignore_keys.extend(line.split('=')[1].split(','))
			continue
		if line.startswith('base='): #base source code directory
			base_path = line.split('=')[1]
			continue
		if (line.startswith('exclude=')):
			exclusions.extend(line.split('=')[1].split(','))
		else:
			files.append(line)

	infile.close()
	if base_path: #prepend optional base path ("base" surely read by now if available)
		for i, path in enumerate(exclusions):
			exclusions[i] = base_path + path
	return xml_path, base_path, extensions, files, exclusions, ignore_keys

def walkXmlFiles(path, keys_in_xml, ignore_keys):
	"""Return a list of all XML files in a folder with subdirectories and hand them to the parser.

	Keyword arguments:
	path -- The path in which to recursively look for XML files.
	keys_in_xml -- Output parameter for found XML keys.
	ignore_keys -- List of keys that should not be reportet.
	"""
	if path is None:
		print("---------- [E] No path specified")
		return
	for base_dir, dirs, file_list in os.walk(path):
		for xml_file in file_list:
			extension = os.path.splitext(xml_file)[1]
			if (extension.lower() == '.xml'):
				parseXmlFile(os.path.join(base_dir, xml_file), keys_in_xml, ignore_keys)

def parseXmlFile(file_name, keys_in_xml, ignore_keys):
	"""Parse an XML file for INI keys.

	Keyword arguments:
	file_name -- The XML file to parse.
	keys_in_xml -- Output parameter for found XML keys.
	ignore_keys -- List of keys that should not be reportet.
	"""
	shortcut_keys = list()
	prefix = 'key='
	last_key_without_sub = ''
	valid_lines = [line for line in open(file_name) if prefix in line]
	valid_lines = [re.search('(.*)' + prefix + '"([^"]*).*', line).group(2) for line in valid_lines]
	for line in valid_lines:
		key = cleanKey(line)
		if '@' in key:
			key = key.replace('@', last_key_without_sub)
			#shortcut_keys are partial keys (e. g. key of a Horizontal panel) that aren't a key by themselves:
			shortcut_keys.append(last_key_without_sub) #remove retrospectively
		else:
			last_key_without_sub = key
		if not key in ignore_keys:
			keys_in_xml.append(key)
	keys_in_xml[:] = list(set(keys_in_xml) - set(shortcut_keys)) #remove partial keys

def walkProjectFiles(path, extension_list, exclusions, keys_in_source, ignore_keys):
	"""Traverse a path's subdirectories, look for appropriate files, and hand them to the parser.

	Keyword arguments:
	path -- Base path of the code base.
	extension_list -- List of file extensions to consider.
	exclusions -- List of directories to skip.
	keys_in_source -- Output parameter for found keys in the source code.
	ignore_keys -- List of keys that should not be reportet.
	"""
	for base_dir, dirs, file_list in os.walk(path):
		for d in dirs[:]: #in place so that everything is handled by os.walk()
			if os.path.join(base_dir, d) in exclusions: #remove exclusion dirs with all subdirs
				dirs.remove(d)

		for file_name in file_list:
			if os.path.islink(os.path.join(base_dir, file_name)):
				continue #guard against broken symlinks
			extension = os.path.splitext(file_name)[1]
			if not extension_list or extension[1:] in extension_list:
				try:
					parseSourceFile(os.path.join(base_dir, file_name), keys_in_source, ignore_keys)
				except UnicodeDecodeError: #binary file
					pass

def parseSourceFile(file_name, keys_in_source, ignore_keys):
	"""Parse an SLF software source file for INI keys.

	Keyword arguments:
	file_name -- The source code file to parse.
	keys_in_source -- Output parameter for found keys in the source code.
	ignore_keys -- List of keys that should not be reportet.
	"""
	prefix_list=['cfg.getValue(', 'cfg.get(', 'cfg.keyExists(', 'vecArgs[ii].first==', 'vecArgs[0].first==', \
	    'outputConfig[', 'inputConfig[', 'advancedConfig[']

	try:
		infile = open(file_name, 'r')
	except PermissionError:
		print("---------- [E] Can not open file for reading:", file_name)
		return
	file_content = infile.read().splitlines()
	for line in file_content:
		for prefix in prefix_list:
			if prefix in line.replace(' ', ''): #allow different whitespace styles
				key_match = re.search('(.*)' + re.escape(prefix) + '[^"]*"([^"]*).*', line.replace(' ', ''))
				if key_match is not None:
					key = cleanKey(key_match.group(2))
					if not key in ignore_keys:
						keys_in_source.append(key)

	#Examples of settings query calls:
	#    cfg.getValue("ZRXP_STATUS_UNALTERED_NODATA", "Output", qa_unaltered_nodata, IOUtils::nothrow);
	#    const double in_TZ = cfg.get("TIME_ZONE", "Input");
	#    if (cfg.keyExists("ZRXP_STATUS_NODATA", "Output"))
	#    if (vecArgs[ii].first=="SENSITIVITY") {
	#    } else if (vecArgs[0].first=="SUPPR") {
	#    outputConfig["AGGREGATE_PRO"] = "false";
	#    inputConfig["METEOPATH"] = "./input";
	#    advancedConfig["WIND_SCALING_FACTOR"] = "1.0";

def removeDuplicates(listing):
	"""Remove duplicate items of a list.

	Keyword arguments:
	listing -- The list to remove duplicates from.
	"""
	listing = list(dict.fromkeys(listing))

def cleanKey(key):
	"""Get the clean name of a key that is embedded in section markers etc.

	Keyword arguments:
	key -- The key to clean.
	"""
	idx = key.rfind(':')
	key = key[idx+1:]
	return key.upper()

def printDifferences(keys_in_xml, keys_in_source):
	"""Print keys found in XML but not in source and vice versa.

	Keyword arguments:
	keys_in_xml -- List of found XML keys.
	keys_in_source -- List of found keys in the source code.
	"""
	xml_minus_source = set(keys_in_xml) - set(keys_in_source)
	print('---------- Keys in XMLs but not in source code (%i): ----------' % len(xml_minus_source))
	print(sorted(xml_minus_source))
	source_minus_xml = set(keys_in_source) - set(keys_in_xml)
	print('---------- Keys in source code but not in XMLs (%i): ----------' % len(source_minus_xml))
	print(sorted(source_minus_xml))

def real_main():
	"""Called by the entry point and performs all work.
	"""
	xml_path, base_path, extensions, folder_list, exclusions, ignore_keys = getCodeBaseFiles("code_base_files.ini")
	keys_in_xml = list()
	walkXmlFiles(xml_path, keys_in_xml, ignore_keys)

	keys_in_source = list()
	for folder in folder_list:
		folder = base_path + folder
		if not os.path.isdir(folder):
			print("---------- [E] Not a directory:", folder)
			continue
		walkProjectFiles(folder, extensions, exclusions, keys_in_source, ignore_keys)

	removeDuplicates(keys_in_xml)
	removeDuplicates(keys_in_source)

	printDifferences(keys_in_xml, keys_in_source)

if __name__ == '__main__':
	real_main()
