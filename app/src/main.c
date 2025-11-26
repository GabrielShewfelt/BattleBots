#include <stdio.h>
#include <unistd.h>
#include "hal/controller.h"
#include "bot.h"
#include "bot_controller.h"
#include "network.h"
#include "oled.h" //added for oled

// Fixed function name to match usage below (CamelCase)
static void HandleHit(Bot* bot) {
    bot_remove_life(bot); // Fixed: was bot_removeLife
    int lives = bot_get_lives(bot); // Fixed: was bot_getLives
    printf("Bot %i was hit! Lives remaining: %i\n", bot_get_ID(bot), lives);
}

int main (void) {
    controller_init();

    if (network_init() != 0) {
        printf("Failed to initialize network.\n");
        return 1;
    }
    oled_init(); // to initilize 
    
    Bot* bot1 = bot_create(1, 3); 
    Bot* bot2 = bot_create(2, 3);

    // ERROR 1 FIX: Pass 'bot1' directly, not '&bot1'
    BotController* bc1 = bot_controller_create(bot1, 1);
    BotController* bc2 = bot_controller_create(bot2, 2);

    bot_controller_start(bc1);
    bot_controller_start(bc2);

    int game_running = 1;

    while (game_running) {

        // 1) Read current commands from bots
        int drive1, drive2, swing1, swing2;
        
        // ERROR 2 FIX: Pass addresses using '&'
        bot_get_command(bot1, &drive1, &swing1);
        bot_get_command(bot2, &drive2, &swing2);

        // 2) Send commands to each Raspberry Pi
        network_send_command_to_bot(1, drive1, swing1);
        network_send_command_to_bot(2, drive2, swing2);
        // NEW: track previous lives for OLED
        int prev_lives1 = -1;
        int prev_lives2 = -1;
        int lives1 = bot_get_lives(bot1);
        int lives2 = bot_get_lives(bot2);
        // 3) Process any hits received
        HitEvent event;
        while (network_poll_hit_event(&event)) {
            if (event.bot_id_hit == 1) {
                HandleHit(bot1);
            } else if (event.bot_id_hit == 2) { // Fixed: Check for Bot 2
                HandleHit(bot2);
            }
        }


        // NEW: update OLED when lives change
        if (lives1 != prev_lives1 || lives2 != prev_lives2) {
            oled_update_score(lives1, lives2);
            prev_lives1 = lives1;
            prev_lives2 = lives2;
        }
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
    
    return 0;
}
