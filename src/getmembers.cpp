#include <dpp/dpp.h>
#include <iostream>
#include <unordered_map>

int main() {
    std::string BOT_TOKEN = std::getenv("DISCORD_BOT_TOKEN");
    dpp::snowflake GUILD_ID = 870806579325325342;

    // Use all intents (ensure your bot is whitelisted for privileged intents)
    dpp::cluster bot(BOT_TOKEN, dpp::i_all_intents);

    bot.on_ready([&](const dpp::ready_t& event) {
        std::cout << "discord_user,nickname" << std::endl;

        bot.guild_get_members(GUILD_ID, 1, 100, [&](const dpp::confirmation_callback_t& cc) {
            if (!cc.is_error()) {
                if (auto members = std::get_if<std::unordered_map<dpp::snowflake, dpp::guild_member>>(&cc.value)) {
                    for (const auto& [id, member] : *members) {
                        const dpp::user* user = member.get_user();
                        if (!user) continue;

                        std::string username = user->format_username(); // shows @user for modern usernames
                        std::string nickname = member.get_nickname();   // might be empty

                        std::cout << username << ',' << nickname << std::endl;
                    }
                } else {
                    std::cerr << "Error: Unexpected data from guild_get_members." << std::endl;
                }
            } else {
                std::cerr << "Error fetching members: " << cc.get_error().message << std::endl;
            }

            bot.shutdown();
        });
    });

    bot.start(dpp::st_wait);
    return 0;
}

