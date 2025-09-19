#include "main.h"
#include "listscores.h"
#include "searchmessages.h"
#include "updatespreadsheet.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;
using namespace OpenXLSX;

const std::string SPREADSHEET_FILENAME = "Gotcha.xlsx";
const std::string TEMP_SPREADSHEET_FILENAME = "Gotcha_temp_save.xlsx";
const dpp::snowflake GOTCHA_CHANNEL_ID = 1010207718885314620;
const dpp::snowflake OUTPUT_THREAD_ID = 1407783022295519292;
std::string BOT_TOKEN = std::getenv("DISCORD_BOT_TOKEN");
std::mutex spreadsheet_mutex;
std::string CURRENT_SEM = "Fall 2025";

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    // Loop through the string stream, extracting tokens separated by the delimiter
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token); // Add the extracted token to the vector
    }

    return tokens; // Return the vector of tokens
}


std::pmr::unordered_map<std::string, std::string> createMap() {
  std::pmr::unordered_map<std::string,std::string> discordToName;
  std::ifstream file("discordToName.csv"); //open csv

  if (!file.is_open()) {
      std::cerr << "Error: Could not open CSV file: " << "discordToName.csv" << std::endl;
      return discordToName;
  }

  std::string line;
  while(std::getline(file, line)) {
      std::vector<std::string> col = split(line, ',');

      if (col.size() >= 2) {
          discordToName[col[0]] = col[1]; //first col is keym second is val
    } else {
        std::cerr << "Warning: Skipping malformed line" << line << std::endl;
    }
}
    file.close();
    return discordToName;
}
//Testing somethng

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

void safeCloseDocument(OpenXLSX::XLDocument& doc) {
    try {
        doc.close();
    } catch (const std::exception& e) {
        std::cerr << "Warning: error closing spreadsheet doc" << e.what() << std::endl;
    }
}

//std::string sender_username = sender_discord_username.username;
//dpp::snowflake mentioned_user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
   


  // --- Bot Event Handlers ---
void on_ready_handler(dpp::cluster& bot, const dpp::ready_t& event) {
    if (dpp::run_once<struct register_bot_commands>()) {
        // std::cout << "Clearing out all commands first..." << std::endl;
        //bot.global_bulk_command_delete()
         std::cout << "Registering slash commands...\n";

        bot.global_command_create(dpp::slashcommand("ping", "Ping Pong!", bot.me.id));
        bot.message_create(dpp::message(OUTPUT_THREAD_ID, "Bot is ready..."));

        dpp::slashcommand update_command("update", "Update the Gotcha! spreadsheet from past messages.", bot.me.id);
        update_command.add_option(
            dpp::command_option(dpp::co_string, "date", "Date to search from (MM-DD-YYYY)", true)
        );
        dpp::slashcommand list_command("listscore", "List all scores for the intended target. ", bot.me.id);
        list_command.add_option(
            dpp::command_option(dpp::co_user, "target", "The user to list scores for.", false) // Optional mention
        );
        bot.global_command_create(list_command);
        bot.global_command_create(update_command);

         std::cout << "Slash commands registered.\n";

    }
    std::cout << "Bot is ready: " << bot.me.username << "\n";
}

void on_slash_command_handler(dpp::cluster& bot, const dpp::slashcommand_t& event) {
    if (event.command.get_command_name() == "ping") {
        event.reply("Pong!");
    } else if (event.command.get_command_name() == "listscore") {
        std::cout << "Executed listscore" << std::endl;
      const dpp::user& sender_user = event.command.get_issuing_user();
      const std::string sender_username = sender_user.username;

      dpp::snowflake mentioned_user_id = 0;

      const dpp::command_value& param = event.get_parameter("target");
        if (std::holds_alternative<dpp::snowflake>(param)) {

            mentioned_user_id = std::get<dpp::snowflake>(param); // Only get if it actually holds a snowflake
        }
        //list person's gotcha! and got got score!
      list_gotcha(bot, event, sender_username, mentioned_user_id);


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
            bot.message_create(dpp::message(OUTPUT_THREAD_ID, "Gotcha! " + msg.author.get_mention() + " taged " + msg.mentions.front().first.get_mention() + ". Processing score..."));
            // Initial reply to acknowledge
            
//            event.reply("Gotcha! " + msg.author.get_mention() + " tagged " + msg.mentions.front().first.get_mention() + ". Processing score...");

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

