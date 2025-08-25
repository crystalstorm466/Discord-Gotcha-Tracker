#ifndef UPDATESPREADSHEET_H
#define UPDATESPREADSHEET_H
#pragma once

#include "main.h"

std::string update_spreadsheet_core_logic(const std::string& sender_discord_username, const std::string& target_discord_username, bool& out_shooter_score_updated, bool& out_victim_score_updated);
void update_Spreadsheet_and_reply(const dpp::message_create_t event, const std::string& sender_discord_username, const std::string& target_discord_username);
std::string update_Spreadsheet_for_command(const std::string& sender_discord_username, const std::string& target_discord_username);
#endif
