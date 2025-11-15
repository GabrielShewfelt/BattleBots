#ifndef BOT_CONTROLLER_H
#define BOT_CONTROLLER_H

#include "bot.h"

typedef struct BotController BotController;

BotController* bot_controller_create(Bot *bot, int controller_index);
int  bot_controller_start(BotController *bc);
void bot_controller_stop(BotController *bc);
void bot_controller_join(BotController *bc);
void bot_controller_destroy(BotController *bc);

#endif
