#include "searchmessages.h"
#include "main.h"
#include "updatespreadsheet.h"
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

using namespace OpenXLSX;
using namespace std;
void search_messages(dpp::cluster& bot, dpp::snowflake channel_id, time_t since_time,
                     std::function<void(const std::vector<dpp::message>&)> callback) {
    auto collected_matches = std::make_shared<std::vector<dpp::message>>();
    auto current_last_id = std::make_shared<dpp::snowflake>(0); // Start with 0 to get latest

    std::function<void()> fetch_batch_recursive; // Declare for mutual recursion/capture

    fetch_batch_recursive = [=, &bot, &fetch_batch_recursive]() {
     dpp::snowflake before_message_id = *current_last_id;
    dpp::snowflake around_message_id = 0; // Always 0 if paginating with 'before'

      std::cout << "Requesting messages from channel " << channel_id
              << (before_message_id != 0 ? " before message ID " + std::to_string(before_message_id) : " (latest)")
              << ".\n";
    dpp::snowflake message_id_to_fetch_before = *current_last_id;
    bot.messages_get(channel_id, 0, message_id_to_fetch_before, 0, 100,
            [=, &bot, &fetch_batch_recursive](const dpp::confirmation_callback_t& cb) {
           if (cb.is_error()) {
                std::cerr << "Error fetching messages: " << cb.get_error().message << std::endl;
                callback(*collected_matches);
                return;

            }
        std::cout << "Fetched a batch of messages from channel " << channel_id << ".\n";
            dpp::message_map retrieved_messages_map;
        try {
            retrieved_messages_map = std::get<dpp::message_map>(cb.value);
        } catch (const std::bad_variant_access& e) {
                std::cerr << "Error: Bad variant access when getting messages: " << e.what() << std::endl;
                callback(*collected_matches);
                return;
        }

        if (retrieved_messages_map.empty()) {
            callback(*collected_matches);
            return;
        }

        std::vector<dpp::message> sorted_batch_vector;
        for (const auto& pair : retrieved_messages_map) {
            sorted_batch_vector.push_back(pair.second);
        }
        //sort by id descending (newest first) to process cronologically against since_time
        std::sort(sorted_batch_vector.begin(), sorted_batch_vector.end(), [](const dpp::message& a, const dpp::message& b) {
            return a.id > b.id;
        });

        dpp::snowflake oldest_id_in_this_batch = 0;
        bool reached_since_limit = false;

        for (const auto& msg : sorted_batch_vector) {
            oldest_id_in_this_batch = msg.id; //track oldest

            if (msg.sent < since_time) {
                reached_since_limit = true;
                break;
            }
            bool has_image = false;
            for (const auto& attachment : msg.attachments) {
            if (starts_with(attachment.content_type, "image/") ||
                ends_with(attachment.filename, ".png") ||
                ends_with(attachment.filename, ".jpeg") ||
                ends_with(attachment.filename, ".jpg") || // Added jpg
                ends_with(attachment.filename, ".gif")) { // Added gif
                has_image = true;

                break;
            }
        }
        bool has_mention = !msg.mentions.empty();
        if (has_mention && has_image) {
            collected_matches->push_back(msg);
        }
        }
        if (reached_since_limit) {
            callback(*collected_matches);
            return;
        }
        if (retrieved_messages_map.size() == 100 && oldest_id_in_this_batch != 0 && oldest_id_in_this_batch != *current_last_id) {
            *current_last_id = oldest_id_in_this_batch;
            fetch_batch_recursive(); //fetch next batch

        } else {
            callback(*collected_matches); //all done
         }
        });
    }; // End of fetch_batch_recursive lambda definition

    fetch_batch_recursive(); // Initial call
}

void handle_update(dpp::cluster& bot, const dpp::slashcommand_t& event) {
    std::string date_str = std::get<std::string>(event.get_parameter("date"));
    struct tm tm = {};
    if (strptime(date_str.c_str(), "%m-%d-%Y", &tm) == nullptr) {
        event.reply(dpp::message("Invalid date format. Please use MM-DD-YYYY").set_flags(dpp::m_ephemeral));
        return;
    }
    tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
    time_t search_date = mktime(&tm);

    event.thinking();

    auto results_summary = std::make_shared<std::string>("Processing messages since " + date_str + "...\n\n");
    auto processed_count = std::make_shared<int>(0);
    auto successful_updates = std::make_shared<int>(0);

    search_messages(bot, event.command.channel_id, search_date,
                    [&bot, event, date_str, results_summary, processed_count, successful_updates](const std::vector<dpp::message>& results) {
                        if (results.empty() && *processed_count == 0) {
                            event.edit_original_response(dpp::message("No matching messages (with image and mention) found since " + date_str + "."));
                            std::cout << "/update: No matching messages found for date " << date_str << " in channel " << event.command.channel_id << std::endl;
                            return;
                        }

                        // Log to terminal that processing has started
                        std::cout << "-----------------------------------------------------------\n";
                        std::cout << "/update command initiated. Processing messages since: " << date_str << "\n";
                        std::cout << "Found " << results.size() << " messages with images and mentions to process.\n";
                        std::cout << "-----------------------------------------------------------\n";
                        for (const auto& msg: results) {
                            (*processed_count)++;
                            std::string sender_discord_username = msg.author.username;

                            // Ensure there are mentions before trying to access them
                            if (msg.mentions.empty()) {
                                std::cout << "Msg ID " << std::to_string(msg.id) << " by " << sender_discord_username
                                << ": Skipped (unexpectedly no mentions found after search filter).\n";
                                continue;            }
                                std::string target_discord_username = msg.mentions.front().first.username; // Declared target
                                std::string update_status = update_Spreadsheet_for_command(sender_discord_username, target_discord_username); // Declared and assigned update_status

                                // Log detailed status to terminal INSTEAD of results_summary string for Discord
                                if (update_status.empty()) {
                                    std::cout << "  Msg ID " << std::to_string(msg.id) << ": Success for "
                                    << sender_discord_username << " -> " << target_discord_username << ".\n";
                                    (*successful_updates)++;
                                } else {
                                    std::cout << "  Msg ID " << std::to_string(msg.id) << ": Failed for "
                                    << sender_discord_username << " -> " << target_discord_username
                                    << ". Reason: " << update_status << "\n";
                                }
                                if (*processed_count % 50 == 0 || *processed_count == results.size()) {
                                    std::string temp_reply = "Processed " + std::to_string(*processed_count) + "/" + std::to_string(results.size()) + " messages. "
                                    + std::to_string(*successful_updates) + " successful updates.\nWorking...";
                                    if (temp_reply.length() > 1900) temp_reply = temp_reply.substr(0, 1900) + "...";
                                    event.edit_original_response(dpp::message(temp_reply));
                                }
                        }

                        std::cout << "-----------------------------------------------------------\n";
                        std::cout << "Finished processing all messages for /update command.\n";
                        std::cout << "Total messages processed from search: " << *processed_count << "\n";
                        std::cout << "Successful spreadsheet updates: " << *successful_updates << "\n";
                        std::cout << "-----------------------------------------------------------\n";

                        // Construct the FINAL summary message for Discord
                        std::string final_discord_reply = "Finished processing " + std::to_string(*processed_count) +
                        " messages found since " + date_str + ".\n" +
                        std::to_string(*successful_updates) + " successful score updates.\n\n" +
                        "Detailed logs are available in the bot's terminal/console.";

                        // This final message should be well within Discord's character limits.
                        event.edit_original_response(dpp::message(final_discord_reply));    });
}


