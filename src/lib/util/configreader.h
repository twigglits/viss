#ifndef CONFIGREADER_H

#define CONFIGREADER_H

/**
 * \file configreader.h
 */

#include "booltype.h"
#include <string>
#include <vector>
#include <map>

/** A helper class to read config files.
 *
 *  The config files should contain lines like this:
 * 
@code
 
# Lines starting with a hash sign are ignored, can be used for comments
# Other lines should be of the form
key = value

@endcode
 * Everything to the left of the first '=' sign is considered to be the
 * key and everything to the right of the same '=' sign is considered to 
 * be the value. Leading and trailing spaces/tabs are ignored for both key
 * and value.
 *
 * This class just stores the key/value pairs as strings, it does not attempt
 * to interpret values in any way.
 */
class ConfigReader
{
public:
	ConfigReader();
	virtual ~ConfigReader();

	/** Reads the config file specified by \c fileName. */
	bool_t read(const std::string &fileName);

	/** Stores all keys found in the config file in \c keys. */
	void getKeys(std::vector<std::string> &keys) const;

	/** For the key specified by the \c key parameter, the corresponding value
	 *  will be stored in \c value. */
	bool_t getKeyValue(const std::string &key, std::string &value) const;

	/** Prints all key/value pairs to the standard output. */
	void printAll() const;

	/** Clears the stored key/value pairs. */
	void clear();
private:
	std::string substituteVariables(const std::string &s);

	std::map<std::string, std::string> m_keyValues;
	std::map<std::string, std::string> m_variables;
};

#endif // CONFIGREADER_H

