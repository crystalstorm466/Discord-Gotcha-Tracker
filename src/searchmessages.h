#ifndef SEARCHMESSAGES_H
#define SEARCHMESSAGES_H
#pragma once

#include "main.h"

void search_messages(dpp::cluster& bot, dpp::snowflake channel_id, time_t since_time, std::function<void(const std::vector<dpp::message>&)> callback);
void handle_update(dpp::cluster& bot, const dpp::slashcommand_t& event);
#endif
