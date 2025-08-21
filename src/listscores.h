#ifndef listscores
#define listscores
#pragma once 
using namespace std;
using namespace OpenXLSX;

std::vector<int> getScores(const std::string& target_spreadsheet_name, const dpp::slashcommand_t& event);
void list_gotcha(dpp::cluster& bot, const dpp::slashcommand_t& event,
                 const std::string& sender_discord_username,
                 dpp::snowflake mentioned_user_id); 
#endif // !listscores

