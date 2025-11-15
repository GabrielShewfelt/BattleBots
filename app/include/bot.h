#ifndef BOT_H
#define BOT_H

#include <pthread.h>

typedef struct Bot Bot;

Bot* bot_create(int id, int lives);

void bot_remove_life(Bot *bot); 
int bot_get_lives(Bot *bot);
int bot_get_ID(Bot *bot);

void bot_destroy(Bot *bot);


// drive: -1 = reverse, 0 = stop, 1 = forward
// swing: -1 = left, 0 = stop, 1 = right
void bot_set_command(Bot *bot, int drive, int swing);
void bot_get_command(Bot *bot, int *drive, int *swing);

#endif