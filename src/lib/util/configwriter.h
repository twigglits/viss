#ifndef CONFIGWRITER_H

#define CONFIGWRITER_H

#include "booltype.h"
#include <stdint.h>
#include <vector>
#include <map>

class ConfigWriter
{
public:
	ConfigWriter();
	virtual ~ConfigWriter();

	bool_t addKey(const std::string &key, double value);
	bool_t addKey(const std::string &key, const std::vector<double> &values);
	bool_t addKey(const std::string &key, int value);
	bool_t addKey(const std::string &key, int64_t value);
	bool_t addKey(const std::string &key, bool value);
	bool_t addKey(const std::string &key, const char *pStr);
	bool_t addKey(const std::string &key, const std::string &value);

	void getKeys(std::vector<std::string> &keys) const;
	bool_t getKeyValue(const std::string &key, std::string &value) const;
private:
	std::map<std::string,std::string> m_keyValues;
};

#endif // CONFIGWRITER_H
