#include "updatespreadsheet.h"
#include "main.h"
#include <dpp/dpp.h>
#include <OpenXLSX.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace OpenXLSX;

// --- Spreadsheet Update Logic ---
std::string update_spreadsheet_core_logic (const std::string& sender_discord_username,
const std::string& target_discord_username,
bool& out_shooter_score_updated,
bool& out_victim_score_updated) {
    out_shooter_score_updated = false;
    out_victim_score_updated = false;
    std::lock_guard<std::mutex> lock(spreadsheet_mutex);

    if (!std::filesystem::exists(SPREADSHEET_FILENAME)) {
        std::cerr << "Error: Spreadsheet file '" << SPREADSHEET_FILENAME << "' not found.\n";
        return "The scoring spreadsheet is missing. Please contact an admin.";
    }

    XLDocument doc;
    try {
        doc.open(SPREADSHEET_FILENAME);
    } catch (const std::exception& e) {
        std::cerr << "Error: Cannot open spreadsheet '" << SPREADSHEET_FILENAME << "': " << e.what() << "\n";
        return "Could not load the scoring spreadsheet. Please contact a CS major. @crystal_storm";
    }


    XLWorksheet wks;
    try {
        wks = doc.workbook().worksheet(CURRENT_SEM); //Change this for whatever semester we are in.
    } catch (const std::exception& e) {
        std::cerr << "Error accessing worksheet " << CURRENT_SEM << " " << e.what() << std::endl;
        try { doc.close(); } catch(...) {}
        return "Error accessing worksheet " + CURRENT_SEM + ". It might not exist or is corrupted.";
    }

    const auto& discordToNameMap = getDiscordToName();
    std::string shooter_spreadsheet_name, victim_spreadsheet_name;

    auto shooter_it = discordToNameMap.find(sender_discord_username);
    if (shooter_it == discordToNameMap.end()) {
        try { doc.close(); } catch(...) {}
        return "Your Discord username ('" + sender_discord_username + "') isn't registered for scoring. Please contact an admin.";
    }
    shooter_spreadsheet_name = shooter_it->second;


    bool Meeks = false;
    auto victim_it = discordToNameMap.find(target_discord_username);
    if (target_discord_username == "Dr. Freeks") {
        Meeks = true;
    } else {
        victim_it = discordToNameMap.find(target_discord_username);
        if (victim_it == discordToNameMap.end()) {
            safeCloseDocument(doc);
            return "The mentioned user ('" + target_discord_username + "') isn't registered for scoring.";
        }
        victim_spreadsheet_name = victim_it->second;
    }
    bool shooter_found = false;
    bool victim_found = false;

    int numRows = 0;

    try {
        numRows = wks.rowCount();
    } catch (const std::exception& e) {
        std::cerr << "Error getting row count for worksheet " << CURRENT_SEM << e.what() << std::endl;
        try { doc.close(); } catch(...) {}
        return ("Error reading spreadsheet structure (rowCount) for ." + CURRENT_SEM);
    }
    int numCols = wks.columnCount();



    int current_score;

    for (int row = 1; row <= numRows; ++row) {
        try {
            auto cell = wks.cell(row, 1); //Names in COL A
            XLCellReference cellRef(row, 1);
            if (cell.value().type() == XLValueType::String) {
                std::string cellValueStr = cell.value().get<std::string>();

                if (!out_shooter_score_updated && cellValueStr == shooter_spreadsheet_name) {
                    //move over one to left to get to Gotcha!
                    shooter_found = true;
                    auto score_cell = wks.cell(row, 2); //gotcha! cell
                    if (Meeks) {
                        score_cell = wks.cell(row, 4); //Meeks points
                        if (score_cell.value().type() == XLValueType::Integer) {
                            current_score = score_cell.value().get<int>();
                        } else {
                            current_score = 0;
                        }
                        score_cell.value() = current_score + 1;
                        std::cout << "Updated '" << shooter_spreadsheet_name << "' Meeks points to " << current_score + 1 << ".\n";
                        out_shooter_score_updated = true;
                        continue;

                    }

                    if (score_cell.value().type() == XLValueType::Integer) {
                        current_score = score_cell.value().get<int>();
                    }
                    score_cell.value() = current_score + 1;;
                    out_shooter_score_updated = true;
                    std::cout << "Updated '" << shooter_spreadsheet_name << "' Gotcha! score to " << current_score +1 << ".\n";
                    std::cout <<" Currrent value of cell: " << score_cell.value().get<int>() << "\n";
                }
                // Update victim's "Got Got!" score (Column C)
                if (!out_victim_score_updated && cellValueStr == victim_spreadsheet_name) {
                    if (Meeks) {
                        continue;
                    }

                    victim_found = true;
                    auto score_cell = wks.cell(row, 3);

                    if (score_cell.value().type() == XLValueType::Integer) {
                        current_score = score_cell.value().get<int>();
                    }
                    score_cell.value() = current_score + 1;;
                    out_victim_score_updated = true;
                    std::cout << "Updated '" << victim_spreadsheet_name << "' Got Got! score to " << current_score +1 << ".\n";
                    std::cout <<" Currrent value of cell: " << score_cell.value().get<int>() << "\n";
                }
            }
        } catch (const OpenXLSX::XLValueTypeError& e) {
            continue;
        } catch (const std::exception& e) {
            continue;
        }
        if (out_shooter_score_updated && out_victim_score_updated) break;
    }




    if (out_shooter_score_updated || out_victim_score_updated) {
        std::cout << "Attempting to save changes to spreadsheet...\n";
        try {
            doc.saveAs(TEMP_SPREADSHEET_FILENAME);
            doc.close();
            std::cout << "Successfully saved to temporary file: " << TEMP_SPREADSHEET_FILENAME << "\n";
            std::filesystem::rename(TEMP_SPREADSHEET_FILENAME, SPREADSHEET_FILENAME);
            std::cout << "Successfully renamed temporary file to: " << SPREADSHEET_FILENAME << "\n";
            return "";
        } catch (const std::exception& e) {
            std::cerr << "Error during spreadsheet save/rename: " << e.what() << "\n";
            std::error_code ec_remove;

            if (std::filesystem::exists(TEMP_SPREADSHEET_FILENAME)) {
                if (std::filesystem::remove(TEMP_SPREADSHEET_FILENAME, ec_remove)) {
                    std::cout << "Successfully removed temporary file " << TEMP_SPREADSHEET_FILENAME << " after error.\n";
                } else {
                    std::cerr << "Failed to remove temporary file " << TEMP_SPREADSHEET_FILENAME << " after error: " << ec_remove.message() << "\n";
                }
            }
            return "Could not save spreadsheet changes. Please check bot logs.";
        }
    } else {
        safeCloseDocument(doc);
        std::string reason;
        if (!shooter_found) reason += shooter_spreadsheet_name + " (Gotcha!) not found in Column A. ";
        if (!victim_found) reason += victim_spreadsheet_name + " (Got Got) not found in Column A. ";
        if (shooter_found && !out_shooter_score_updated) reason += shooter_spreadsheet_name + " found but score not updated (may indicate no change needed or prior update). ";
        if (victim_found && !out_victim_score_updated) reason += victim_spreadsheet_name + " found but score not updated (may indicate no change needed or prior update). ";
        if (Meeks) reason += "Meeks points updated!";

        if (reason.empty() && (shooter_found || victim_found)) reason = "Users found, but no score updates were performed (possibly already up-to-date or issue with score cells).";
        else if (reason.empty()) reason = "Neither shooter nor victim found in the spreadsheet, or no updates were applicable.";
        std::cerr << "No scores changed. " << reason << "\n";
        return "Scores not updated. " + reason;
    }
    return " ";
}


void update_Spreadsheet_and_reply(const dpp::message_create_t event,
                                  const std::string& sender_discord_username,
                                  const std::string& target_discord_username) {
    bool shooter_updated, victim_updated;
    std::string status_msg = update_spreadsheet_core_logic(sender_discord_username, target_discord_username, shooter_updated, victim_updated);
    if (status_msg.empty()) {

        //event.reply("Scores processed for " + sender_discord_username + " and " + target_discord_username + ". The spreadsheet uploads every night @ 1AM.");
    } else {
        event.reply("Error processing scores: " + status_msg);
    }
}



std::string update_Spreadsheet_for_command(const std::string& sender_discord_username,
                                           const std::string& target_discord_username)
{
    bool shooter_updated, victim_updated;
    return update_spreadsheet_core_logic(sender_discord_username, target_discord_username, shooter_updated, victim_updated);
}

