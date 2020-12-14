/*
 * @Author: your name
 * @Date: 2020-11-25 19:47:47
 * @LastEditTime: 2020-11-26 15:50:47
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/json_parse.h
 */
#ifndef JSON_PARSE_H
#define JSON_PARSE_H
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define IN
#define OUT

class JsonParse
{
public:
    JsonParse(const std::string& src);
    JsonParse(boost::property_tree::ptree& Json_tree);
    ~JsonParse();

    int GetStringValue( IN const std::string& name, OUT std::string& value);
    int GetStringValue( IN const char* name, OUT std::string& value);

    int GetInt16Value( IN const std::string& name, OUT uint16_t& value);
    int GetInt16Value( IN const char* name, OUT uint16_t& value);

    int GetIntValue( IN const std::string& name, OUT int& value);
    int GetIntValue( IN const char* name, OUT int& value);

    int GetInt64Value( IN const std::string& name, OUT uint64_t& value);
    int GetInt64Value( IN const char* name, OUT uint64_t& value);
    
    JsonParse GetChild( IN const std::string& name);

private:
    boost::property_tree::ptree item;
};

#endif // JSON_PARSE_H
