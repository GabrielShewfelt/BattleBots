#include <stdlib.h>
#include <pthread.h>
#include "bot.h"
struct Bot {
    int id;
    int drive;
    int swing;
    unsigned int lives;
    pthread_mutex_t lock;
};

Bot* bot_create(int id, int lives) {
    Bot *bot = malloc(sizeof(struct Bot));
    if (bot == NULL) return NULL;

    bot->lives = lives;
    bot->id = id;
    bot->drive = 0;
    bot->swing = 0;

    pthread_mutex_init(&bot->lock, NULL);

    return bot;
}

void bot_remove_life(Bot *bot) {
    pthread_mutex_lock(&bot->lock);
    if (bot->lives > 0) bot->lives--;
    pthread_mutex_unlock(&bot->lock);
}

int bot_get_lives(Bot *bot) {
    int lives;
    pthread_mutex_lock(&bot->lock);
    lives = bot->lives;
    pthread_mutex_unlock(&bot->lock);
    return lives;
}

int bot_get_ID(Bot *bot) {
    return bot->id;
}

void bot_set_command(Bot *bot, int drive, int swing) {
    pthread_mutex_lock(&bot->lock);
    bot->drive = drive;
    bot->swing = swing;
    pthread_mutex_unlock(&bot->lock);
}

void bot_get_command(Bot *bot, int *drive, int *swing) {
    pthread_mutex_lock(&bot->lock);
    if (drive) *drive = bot->drive;
    if (swing) *swing = bot->swing;
    pthread_mutex_unlock(&bot->lock);
}

void bot_destroy(Bot *bot) {
    if (!bot) return;
    pthread_mutex_destroy(&bot->lock);
    free(bot);
}