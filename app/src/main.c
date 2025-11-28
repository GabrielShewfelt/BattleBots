#include <stdio.h>
#include <unistd.h>
#include "hal/controller.h"
#include "bot.h"
#include "bot_controller.h"
#include "network.h"
#include "oled.h"  // <--- ADDED

// Helper to print hit info to console
static void HandleHit(Bot* bot) {
    bot_remove_life(bot);
    int lives = bot_get_lives(bot);
    printf("Bot %i was hit! Lives remaining: %i\n", bot_get_ID(bot), lives);
}

int main (void) {
    // 1. Initialize Subsystems
    controller_init();
    oled_init();  // <--- ADDED: Shows "Are you ready?"

    if (network_init() != 0) {
        printf("Failed to initialize network.\n");
        return 1;
    }

    // 2. Setup Bots
    Bot* bot1 = bot_create(1, 3); 
    Bot* bot2 = bot_create(2, 3);

    // FIX APPLIED HERE:
    // Use controller index 0 (js0) for Bot 1
    // Use controller index 1 (js1) for Bot 2
    BotController* bc1 = bot_controller_create(bot1, 0); 
    BotController* bc2 = bot_controller_create(bot2, 1);

    bot_controller_start(bc1);
    bot_controller_start(bc2);

    // 3. Game Loop Variables
    int game_running = 1;
    
    // State tracking to prevent OLED flickering/lag
    int prev_lives1 = 3;
    int prev_lives2 = 3;

    // Show initial score
    oled_update_score(3, 3);

    while (game_running) {

        // --- A. Read & Send Commands ---
        int drive1, drive2, swing1, swing2;
        bot_get_command(bot1, &drive1, &swing1);
        bot_get_command(bot2, &drive2, &swing2);

        network_send_command_to_bot(1, drive1, swing1);
        network_send_command_to_bot(2, drive2, swing2);

        // --- B. Process Hits ---
        HitEvent event;
        while (network_poll_hit_event(&event)) {
            if (event.bot_id_hit == 1) {
                HandleHit(bot1);
            } else if (event.bot_id_hit == 2) {
                HandleHit(bot2);
            }
        }

        // --- C. Update Logic & Display ---
        int lives1 = bot_get_lives(bot1);
        int lives2 = bot_get_lives(bot2);

        // ONLY update OLED if score changed to avoid lag
        if (lives1 != prev_lives1 || lives2 != prev_lives2) {
            oled_update_score(lives1, lives2);
            prev_lives1 = lives1;
            prev_lives2 = lives2;
        }

        // --- D. Check Game Over ---
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

    // Cleanup
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