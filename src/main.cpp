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
#include <vector>
#include <algorithm>
#include <iomanip>
using namespace std;
using namespace OpenXLSX;

const std::string SPREADSHEET_FILENAME = "Gotcha.xlsx";
const std::string TEMP_SPREADSHEET_FILENAME = "Gotcha_temp_save.xlsx";
// Consider making this configurable (e.g., from an environment variable or config file)
//const dpp::snowflake GOTCHA_CHANNEL_ID = 1376356171417649222;// Regular Gotcha Channel ID
const dpp::snowflake GOTCHA_CHANNEL_ID = 1376356171417649222; // Testing Channel ID
static std::string BOT_TOKEN = std::getenv("DISCORD_BOT_TOKEN");
static std::mutex spreadsheet_mutex;

std::pmr::unordered_map<std::string, std::string> createMap() {
  std::pmr::unordered_map<std::string,std::string> discordToName;
  //REDACTED FOR PRIVACY
  return discordToName;
}
std::pmr::unordered_map<std::string, std::string>& getDiscordToName() {
  static std::pmr::unordered_map<std::string, std::string> map = createMap();
  return map; 
}
bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() &&
      str.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
//std::string sender_username = sender_discord_username.username;
//dpp::snowflake mentioned_user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
//list_gotcha(bot, event, sender_username, mentioned_user_id);
int64_t list_gotcha(dpp::cluster& bot, const dpp::slashcommand_t& event,
                 const std::string& sender_discord_username,
                 dpp::snowflake mentioned_user_id, 
                int scoreType) {
    std::string target_discord_username;
    if (mentioned_user_id == 0) {
        target_discord_username = sender_discord_username; // If no mention, use sender
    } else {
        auto user = bot.user_get(mentioned_user_id, dpp::cache_policy::cp_default, dpp::co_synchronous);
        if (user.is_error()) {
            event.reply("Error fetching mentioned user: " + user.get_error().message);
            return;
        }
        target_discord_username = user.get().username;
    }
    std::lock_guard<std::mutex> lock(spreadsheet_mutex);

    if (!std::filesystem::exists(SPREADSHEET_FILENAME)) {
        std::cerr << "Error: Spreadsheet file '" << SPREADSHEET_FILENAME << "' not found.\n";
        std::cerr <<  "The scoring spreadsheet is missing. Please contact an admin." << std::endl
    }

    XLDocument doc;
    try {
        doc.open(SPREADSHEET_FILENAME);
    } catch (const std::exception& e) {
        std::cerr << "Error: Cannot open spreadsheet '" << SPREADSHEET_FILENAME << "': " << e.what() << "\n";
        std::cerr << "Could not load the scoring spreadsheet. Please contact a CS major. @crystal_storm" << std::endl;
    }
    
   
    XLWorksheet wks;
    try {
        wks = doc.workbook().worksheet("Spring 2025"); //Change this for whatever semester we are in. 
    } catch (const std::exception& e) {
         std::cerr << "Error accessing worksheet 'Spring 2025': " << e.what() << std::endl;
         try { doc.close(); } catch(...) {}
        std::cerr << "Error accessing worksheet 'Spring 2025'. It might not exist or is corrupted." << std::endl; 
    }

    const auto& discordToNameMap = getDiscordToName();
    std::string shooter_spreadsheet_name, victim_spreadsheet_name;

    auto shooter_it = discordToNameMap.find(sender_discord_username);
    if (shooter_it == discordToNameMap.end()) {
        try { doc.close(); } catch(...) {}
        event.reply("Your Discord username ('" + sender_discord_username + "') isn't registered for scoring. Please contact an admin.");
    }
    shooter_spreadsheet_name = shooter_it->second;
  

    bool Meeks = false;
    auto victim_it = discordToNameMap.find(target_discord_username);
    if (target_discord_username == "Dr. Freeks") {
    Meeks = true;
    } else {
      victim_it = discordToNameMap.find(target_discord_username);
       if (victim_it == discordToNameMap.end()) {
        try { doc.close(); } catch(...) {}
        event.reply( "The mentioned user ('" + target_discord_username + "') isn't registered for scoring.");
      }
     victim_spreadsheet_name = victim_it->second;
    }
    bool shooter_found = false;
    bool victim_found = false;

    int numRows = 0;

    try {
        numRows = wks.rowCount();
    } catch (const std::exception& e) {
        std::cerr << "Error getting row count for worksheet 'Spring 2025': " << e.what() << std::endl;
        try { doc.close(); } catch(...) {}
        std::cerr <<  "Error reading spreadsheet structure (rowCount) for 'Spring 2025'." << cerr
    }
    int numCols = wks.columnCount();
    

  
    int current_score;

    for (int row = 1; row <= numRows; ++row) {
          try {
            auto cell = wks.cell(row, 1); //Names in COL A
            XLCellReference cellRef(row, 1);
            if (cell.value().type() == XLValueType::String) {
              std::string cellValueStr = cell.value().get<std::string>();
            
            if (cellValueStr == shooter_spreadsheet_name ) {
                if (scoreType == 0) {
                //move over one to left to get to Gotcha! 
                shooter_found = true;
                auto score_cell = wks.cell(row, 2); 
                return score_cell.value().get<int>();//maybe? 
                if (scoreType == 2) {
                    score_cell = wks.cell(row, 4); //Meeks points
                    return score_cell.value().get<int>();//maybe? 
                }
                
                } else if (scoreType == 2) {
                    shooter_found = true;
                    auto score_cell = wks.cell(row, 4); //Meeks points
                    return score_cell.value().get<int>();//maybe? 
                } else if (scoreType == 1) {
                    shooter_found = true;
                    auto score_cell = wks.cell(row, 3); //Got Got! points
                    return score_cell.value().get<int>();//maybe? 

                
                } 
            // Update victim's "Got Got!" score (Column C)
           
          }             
        }
        }  catch (const std::exception& e) {
        continue;
    }
    
    }
    


}
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
        wks = doc.workbook().worksheet("Spring 2025"); //Change this for whatever semester we are in. 
    } catch (const std::exception& e) {
         std::cerr << "Error accessing worksheet 'Spring 2025': " << e.what() << std::endl;
         try { doc.close(); } catch(...) {}
         return "Error accessing worksheet 'Spring 2025'. It might not exist or is corrupted.";
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
        try { doc.close(); } catch(...) {}
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
        std::cerr << "Error getting row count for worksheet 'Spring 2025': " << e.what() << std::endl;
        try { doc.close(); } catch(...) {}
        return "Error reading spreadsheet structure (rowCount) for 'Spring 2025'.";
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
                auto score_cell = wks.cell(row, 2); 
                if (Meeks) {
                    score_cell = wks.cell(row, 4); //Meeks points
                  if (score_cell.value().type() != XLValueType::Integer) {
                        current_score = score_cell.value().get<int>();
                    } else {
                      current_score = 0;
                      }
                    score_cell.value() = current_score + 1;
                    std::cout << "Updated '" << shooter_spreadsheet_name << "' Meeks points to " << current_score + 1 << ".\n";
                    out_victim_score_updated = true;
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
                       //move over one to left to get to Gotcha! 
                victim_found = true;
                auto score_cell = wks.cell(row, 3); 

                if (score_cell.value().type() == XLValueType::Integer) {
                    current_score = score_cell.value().get<int>();
                }
                score_cell.value() = current_score + 1;;
                out_shooter_score_updated = true;
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
        try { doc.close(); } catch (...){}
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
}
void update_Spreadsheet_and_reply(const dpp::message_create_t event,
                                  const std::string& sender_discord_username,
                                  const std::string& target_discord_username) {
        bool shooter_updated, victim_updated;
        std::string status_msg = update_spreadsheet_core_logic(sender_discord_username, target_discord_username, shooter_updated, victim_updated);
        if (status_msg.empty()) {
            event.reply("Scores processed for " + sender_discord_username + " and " + target_discord_username + ". The spreadsheet uploads every night @ 1AM.");
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



  // --- Bot Event Handlers ---
void on_ready_handler(dpp::cluster& bot, const dpp::ready_t& event) {
    if (dpp::run_once<struct register_bot_commands>()) {
        std::cout << "Registering slash commands...\n";
        // bot.global_bulk_command_delete(); // Use with caution, uncomment if you need to clear all commands
        bot.global_command_create(dpp::slashcommand("ping", "Ping Pong!", bot.me.id));
        
        
        dpp::slashcommand update_command("update", "Update the Gotcha! spreadsheet from past messages.", bot.me.id);
        update_command.add_option(
            dpp::command_option(dpp::co_string, "date", "Date to search from (MM-DD-YYYY)", true)
        );
        dpp::slashcommand gotcha_command("gotcha", "List all scores for the intended target. ", bot.me.id);
        gotcha_command.add_option(
            dpp::command_option(dpp::co_user, "target", "The user to list scores for.", false, true) // Optional mention
        );
        bot.global_command_create(gotcha_command);
         std::cout << "Slash commands registered.\n";
        
    }
    std::cout << "Bot is ready: " << bot.me.username << "\n";
}

void on_slash_command_handler(dpp::cluster& bot, const dpp::slashcommand_t& event) {
    if (event.command.get_command_name() == "ping") {
        event.reply("Pong!");
    } else if (event.command.get_command_name() == "gotcha") {
        //list person's gotcha! and got got score!
    dpp::user sender_discord_username = event.command.get_issuing_user();
       std::string sender_username = sender_discord_username.username;
       dpp::snowflake mentioned_user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
       int gotcha_score = list_gotcha(bot, event, sender_username, mentioned_user_id, 0);
       int gotgot_score = list_gotcha(bot, event, sender_username, mentioned_user_id, 1);
       int meeks_score = list_gotcha(bot, event, sender_username, mentioned_user_id, 2);

       dpp::embed embed = dpp::embed()
            .set_color(dpp::colors::sti_blue)
            .set_title("Gotcha Scores for " + sender_username)
          //  .set_url("https://dpp.dev/")
           // .set_description("")
            .set_thumbnail("/xijinpging.jpg") // Using bot's avatar
            .add_field("Gotcha!:", gotcha_score)
            .add_field("Got Got!: ", gotgot_score, true)
            .add_field("Dr. Meeks Points: ", meeks_score, true)
            .set_timestamp(time(0));
        dpp::message msg(event.command.channel_id, embed);
        event.reply(msg);
    } else if (event.command.get_command_name() == "update") {
        handle_update(bot, event);
    }
}

void on_message_create_handler(dpp::cluster& bot, const dpp::message_create_t& event) {
    const dpp::message& msg = event.msg;

    if (msg.author.is_bot()) {
        return;
    }
       if (msg.channel_id == GOTCHA_CHANNEL_ID && !msg.mentions.empty()) {
        bool image_found = false;
        for (const auto& attachment : msg.attachments) {
            if (starts_with(attachment.content_type, "image/") ||
                ends_with(attachment.filename, ".png") ||
                ends_with(attachment.filename, ".jpeg") ||
                ends_with(attachment.filename, ".jpg") || // Added jpg
                ends_with(attachment.filename, ".gif")) { // Added gif
                image_found = true;
                  break;
            }
        }

        if (image_found) {
            
            std::string sender_discord_username = msg.author.username;
            std::string target_discord_username = msg.mentions.front().first.username; // Using .front() for simplicity, assumes at least one mention
            if (target_discord_username == bot.me.username) {
                event.reply("Dr Meeks points");
                std::cout << "Gotcha event: Sender '" << sender_discord_username
                          << "' tagged Dr. Meeks" << msg.channel_id
                          << " with an image.\n";
                update_Spreadsheet_and_reply(event, sender_discord_username, bot.me.username);
                return; // Early return if tagged Meeks
            }
            std::cout << "Gotcha event: Sender '" << sender_discord_username
                      << "' tagged '" << target_discord_username
                      << "' in channel " << msg.channel_id
                      << " with an image.\n";

            // Initial reply to acknowledge
            event.reply("Gotcha! " + msg.author.get_mention() + " tagged " + msg.mentions.front().first.get_mention() + ". Processing score...");

            update_Spreadsheet_and_reply(event, sender_discord_username, target_discord_username);
            // The final reply about spreadsheet update status will now come from updateGotchaSpreadsheet
        }
    }
}
// Helper function to format time
std::string format_time(time_t t) {
    char buf[64];
    strftime(buf, sizeof(buf), "%m-%d-%Y %H:%M", localtime(&t));
    return std::string(buf);
}




int main() {
  if (BOT_TOKEN.empty()) {
        std::cerr << "Error: DISCORD_BOT_TOKEN environment variable is not set.\n";
        return 1;
    }
    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
    bot.on_log(dpp::utility::cout_logger()); // DPP logging


    // Bind event handlers using the standalone functions
    bot.on_ready([&bot](const dpp::ready_t& event) {
        on_ready_handler(bot, event); // Call the separate handler
    });
    
    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event){ 
        on_slash_command_handler(bot, event); // Call the separate handler
    });

    bot.on_message_create([&bot](const dpp::message_create_t& event){ 
        on_message_create_handler(bot, event); // Call the separate handler
    });


 try {
        bot.start(dpp::st_wait);
    } catch (const dpp::exception& e) {
        std::cerr << "DPP Exception: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main." << std::endl;
        return 1;
    }
        
  return 0;

}

