#include <stdio.h>
#include <unistd.h>
#include "hal/controller.h" // used for controller communication
#include "bot.h"
#include "bot_controller.h"
#include "network.h"   // used for network communication

static void Handle_Hit(Bot* bot) {
    bot_remove_life(bot);
    int lives = bot_get_lives(bot);
    printf("Bot %i was hit! Lives remaing: %i\n", bot_get_ID(bot), lives);
}

int main (void) {
    controller_init();
    
    network_init();

    Bot* bot1 = bot_create(1, 3);
    Bot* bot2 = bot_create(2, 3);

    BotController* bc1 = bot_controller_create(&bot1, 1);
    BotController* bc2 = bot_controller_create(&bot2, 2);

    bot_controller_start(bc1);
    bot_controller_start(bc2);

    int game_running = 1;

    while (game_running) {

        // 1) Read current commands from bots
        int drive1, drive2, swing1, swing2;
        bot_get_command(bot1, drive1, swing1);
        bot_get_command(bot2, drive2, swing2);

        // 2) Send commands to each Raspberry Pi
        network_send_command_to_bot(1, drive1, swing1);
        network_send_command_to_bot(2, drive2, swing2);

        // 3) Process any hits received
        HitEvent event;
        while (network_poll_hit_event(&event)) {
            if (event.bot_id_hit == 1) {
                Handle_Hit(bot1);
            } else if (event.bot_id_hit == 1) {
                Handle_Hit(bot1);
            }
        }

        int lives1 = bot_get_lives(bot1);
        int lives2 = bot_get_lives(bot2);

        if (lives1 <= 0 || lives2 <= 0) {
            printf("Game over! ");

            if (lives1 <= 0 && lives2 <= 0) {
                printf("Draw.\n");
            } else if (lives1 <= 0) {
                printf("Bot 2 wins!\n");
            } else {
                printf("Bot 1 wins!\n");
            }

            game_running = 0;
            break;
        }

        // update game stats
        usleep(20 * 1000); // ~50 Hz loop
    }

    bot_controller_stop(bc1);
    bot_controller_stop(bc2);
    bot_controller_join(bc1);
    bot_controller_join(bc2);

    bot_controller_destroy(bc1);
    bot_controller_destroy(bc2);
    bot_destroy(bot1);
    bot_destroy(bot2);
}