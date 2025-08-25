#ifndef MAIN_H
#define MAIN_H
#pragma once

#include <dpp/dpp.h>
#include <OpenXLSX.hpp>
#include <string>
#include <unordered_map>
#include <vector>

const extern std::string SPREADSHEET_FILENAME;
const extern std::string TEMP_SPREADSHEET_FILENAME;
const extern dpp::snowflake GOTCHA_CHANNEL_ID;
const extern dpp::snowflake OUTPUT_THREAD_ID;
extern std::string BOT_TOKEN;
extern std::mutex spreadsheet_mutex;
extern std::string CURRENT_SEM;

std::vector<std::string> split(const std::string& s, char delimiter);
std::pmr::unordered_map<std::string, std::string> createMap();
std::pmr::unordered_map<std::string, std::string>& getDiscordToName();
bool starts_with(const std::string& str, const std::string& prefix);
bool ends_with(const std::string& str, const std::string& suffix);
void safeCloseDocument(OpenXLSX::XLDocument& doc);

#endif
