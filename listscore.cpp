#include <cstdint>
#include <dpp/dpp.h> // Includes most common DPP headers
#include <OpenXLSX.hpp>

#include <cstdlib> // For std::getenv
#include <filesystem>
#include <fstream> // Potentially for future config file reading
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "listscores.h"
using namespace std;
using namespace OpenXLSX;

std::vector<int> getScores(const std::string& target_spreadsheet_name, const dpp::slashcommand_t& event) {
    std::vector<int> scores = {0,0,0};
    std::lock_guard<std::mutex> lock(spreadsheet_mutex);

    if(!std::filesystem::exists(SPREADSHEET_FILENAME)) {
        event.edit_original_response(dpp::message("The spreadsheet is missing. Please contact a Computer Science Major"));
        std::cerr << "error: spreadsheet file " << SPREADSHEET_FILENAME << "is missing" << std::endl;
        return scores;
    }

    XLDocument doc;
    try {
        doc.open(SPREADSHEET_FILENAME);
    } catch (const std::exception& e) {
        event.edit_original_response(dpp::message("Could not load spreadsheet."));
        std::cerr << "Error. Cannot open spreadsheet." << std::endl;
        return scores;
    }

    OpenXLSX::XLWorksheet wks;
    const std::string worksheet_name = CURRENT_SEM;
    try {
        wks = doc.workbook().worksheet(CURRENT_SEM); //Change this for whatever semester we are in.
    } catch (const std::exception& e) {
      std::cerr << "Error accessing worksheet " << CURRENT_SEM <<" " << e.what() << std::endl;
        safeCloseDocument(doc);
        return scores;
    }
    int numRows = 0;
    try {
        numRows = wks.rowCount();
    } catch (const std::exception& e) {
        event.edit_original_response(dpp::message("Error reading spreadsheet structure. Please contact an admin,"));
        safeCloseDocument(doc);
        return scores;
    }


    bool target_found = false;
    for (int row = 1; row <= numRows; ++row) {
        try {
            OpenXLSX::XLCell cell = wks.cell(row, 1); //Names in COL A
            if (cell.value().type() == XLValueType::String) {
                std::string cellValueStr = cell.value().get<std::string>();

                if (cellValueStr == target_spreadsheet_name ) {
                    //move over one to left to get to Gotcha!
                    target_found = true;
                    //cols: gotcha - 2, gotgot - 3, meeks - 4
                    scores[0] = wks.cell(row, 2).value().get<int>(); //Gotcha!
                    scores[1] = wks.cell(row, 3).value().get<int>(); //Got Got!
                    scores[2] = wks.cell(row, 4).value().get<int>(); //Meeks
                    break; //found target
                }
            }
        }  catch (const std::exception& e) {
            std::cerr << "Warning: Error reading cell at row " << row << ", column 1: " << e.what() << std::endl;
            continue;
        }

    }
    if (!target_found) {
        event.reply("Scores for " + target_spreadsheet_name + " could not be found in the spreadsheet.");
    }
    safeCloseDocument(doc);


    return scores;
}


void list_gotcha(dpp::cluster& bot, const dpp::slashcommand_t& event,
                 const std::string& sender_discord_username,
                 dpp::snowflake mentioned_user_id) {
            event.thinking();
        std::vector<int> scores= {0, 0, 0};
        std::string target_discord_username;
        std::string target_avatar_url;
        dpp::user target_user_object;
        std::cout << "Checking for username" << std::endl;
        if (mentioned_user_id == 0) {
          target_discord_username = sender_discord_username;
          target_avatar_url = event.command.get_issuing_user().get_avatar_url();
          //proceed directly to fetch scores
          const auto& discordToNameMap = getDiscordToName();
          auto target_it = discordToNameMap.find(target_discord_username);
          if (target_it == discordToNameMap.end()) {
              event.edit_original_response(dpp::message("Your discord username '" + target_discord_username + "' is not registered."));
              return;
        }
        std::string target_spreadsheet_name = target_it->second;
        std::vector<int> scores = getScores(target_spreadsheet_name, event);


        dpp::embed embed = dpp::embed()
        .set_color(dpp::colors::sti_blue)
        .set_title("Gotcha Scores for " + target_spreadsheet_name)
        .set_thumbnail(target_avatar_url) // Using bot's avatar
        .add_field("Gotcha!:", std::to_string(scores[0]), true)
        .add_field("Got Got!: ", std::to_string(scores[1]), true)
        .add_field("Dr. Meeks Points: ", std::to_string(scores[2]), true)
        .set_timestamp(time(0));

        dpp::message msg(event.command.channel_id, embed);
        event.edit_original_response(msg);

        } else {   //Asynchronously fetch the mentioned user's details'

            bot.user_get(mentioned_user_id, [&, event, sender_discord_username](const dpp::confirmation_callback_t& cb) {
                if (cb.is_error()) {
                    std::cerr << "Error fetching mentioned user from Discord: " << cb.get_error().message << std::endl;
                    event.reply("error fetching user information");
                    return;
                }
                if (std::holds_alternative<dpp::user>(cb.value)) {
                    dpp::user mentioned_user = std::get<dpp::user>(cb.value);
                    std::string target_discord_username_async = mentioned_user.username;
                    std::string target_avatar_url_async = mentioned_user.get_avatar_url();

                    const auto& discordToNameMap = getDiscordToName();
                    auto target_it = discordToNameMap.find(target_discord_username_async);
                    if (target_it == discordToNameMap.end()) {
                        event.edit_original_response(dpp::message("Your discord username '" + target_discord_username_async + "' is not registered."));
                        return;
                    }
                    std::string target_spreadsheet_name = target_it->second;
                    std::vector<int> scores = getScores(target_spreadsheet_name, event);
                    dpp::embed embed = dpp::embed()
                    .set_color(dpp::colors::sti_blue)
                    .set_title("Gotcha Scores for " + target_spreadsheet_name)
                    .set_thumbnail(target_avatar_url_async) // Using bot's avatar
                    .add_field("Gotcha!:", std::to_string(scores[0]), true)
                    .add_field("Got Got!: ", std::to_string(scores[1]), true)
                    .add_field("Dr. Meeks Points: ", std::to_string(scores[2]), true)
                    .set_timestamp(time(0));

                    dpp::message msg(event.command.channel_id, embed);
                    event.edit_original_response(msg);
                 } else {
                     if (std::holds_alternative<dpp::user_identified>(cb.value)) {
                         dpp::user mentioned_user = std::get<dpp::user_identified>(cb.value);
                         std::string target_discord_username_async = mentioned_user.username;
                         std::string target_avatar_url_async = mentioned_user.get_avatar_url();

                         const auto& discordToNameMap = getDiscordToName();
                         auto target_it = discordToNameMap.find(target_discord_username_async);
                         if (target_it == discordToNameMap.end()) {
                             event.edit_original_response(dpp::message("Your discord username '" + target_discord_username_async + "' is not registered."));
                             return;
                         }
                         std::string target_spreadsheet_name = target_it->second;
                         std::vector<int> scores = getScores(target_spreadsheet_name, event);
                         dpp::embed embed = dpp::embed()
                         .set_color(dpp::colors::sti_blue)
                         .set_title("Gotcha Scores for " + target_spreadsheet_name)
                         .set_thumbnail(target_avatar_url_async) // Using bot's avatar
                         .add_field("Gotcha!:", std::to_string(scores[0]), true)
                         .add_field("Got Got!: ", std::to_string(scores[1]), true)
                         .add_field("Dr. Meeks Points: ", std::to_string(scores[2]), true)
                         .set_timestamp(time(0));

                         dpp::message msg(event.command.channel_id, embed);
                         event.edit_original_response(msg);
                         return;
                    }

                    std::cerr << "Error: unexpected value in user_get callback: " << cb.value.index()  << std::endl;
                    event.edit_original_response(dpp::message("An unexpected error occured while fetching user data. Please try again."));
                    return;
                }
            });


            }
            return;
}


