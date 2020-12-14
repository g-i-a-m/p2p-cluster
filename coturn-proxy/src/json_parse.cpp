/*
 * @Author: your name
 * @Date: 2020-11-25 19:47:47
 * @LastEditTime: 2020-11-26 15:51:57
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /coturn-proxy/src/json_parse.cpp
 */
#include "json_parse.h"

JsonParse::JsonParse(const std::string& src)
{
    std::stringstream str_stream(src);
    boost::property_tree::read_json(str_stream,item);
}
JsonParse::JsonParse(boost::property_tree::ptree& Json_tree)
{
    item = Json_tree;
}
JsonParse::~JsonParse()
{

}

int JsonParse::GetStringValue(const std::string& name, std::string& value)
{
    int iRet = 0;
    try {
        value = item.get<std::string>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

int JsonParse::GetStringValue(const char* name, std::string& value)
{
    int iRet = 0;
    try {
        value = item.get<std::string>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

int JsonParse::GetInt16Value( IN const std::string& name, OUT uint16_t& value) {
    int iRet = 0;
    try {
        value = item.get<uint16_t>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

int JsonParse::GetInt16Value( IN const char* name, OUT uint16_t& value) {
    int iRet = 0;
    try {
        value = item.get<uint16_t>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}
    
int JsonParse::GetIntValue(const std::string& name,int& value)
{
    int iRet = 0;
    try {
        value = item.get<int>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

int JsonParse::GetIntValue(const char* name,int& value)
{
    int iRet = 0;
    try {
        value = item.get<int>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

int JsonParse::GetInt64Value(const std::string& name,uint64_t& value)
{
    int iRet = 0;
    try {
        value = item.get<uint64_t>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

int JsonParse::GetInt64Value(const char* name,uint64_t& value)
{
    int iRet = 0;
    try {
        value = item.get<uint64_t>(name);
    } catch (...) {
        iRet = -1;
    }
    return iRet;
}

JsonParse JsonParse::GetChild(const std::string& name)
{
    boost::property_tree::ptree item_child = item.get_child(name);
    JsonParse json(item_child);
    return json;
}

