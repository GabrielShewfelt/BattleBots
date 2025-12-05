/*
    This is the main script used to run the game
*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h> 
#include "hal/controller.h"
#include "bot.h"
#include "bot_controller.h"
#include "network.h"
#include "oled.h"

// Global flag to control the main loop
static volatile int keep_running = 1;

// Signal Handler
void stop_game_handler(int signum) {
    (void)signum;
    printf("\n Caught Stop Signal! Shutting down gracefully...\n");
    
    // FAIL-SAFE: If cleanup hangs for more than 2 seconds, force kill the program
    alarm(2); 
    
    keep_running = 0;
}

static void HandleHit(Bot* bot) {
    bot_remove_life(bot);
    int lives = bot_get_lives(bot);
    printf("Bot %i was hit! Lives remaining: %i\n", bot_get_ID(bot), lives);
    fflush(stdout);
}

int main (void) {
    // 1. Register the Signal Handler
    signal(SIGINT, stop_game_handler);
    signal(SIGTERM, stop_game_handler);

    // 2. Initialize Subsystems
    controller_init();
    oled_init();

    if (network_init() != 0) {
        printf("Failed to initialize network.\n");
        return 1;
    }

    Bot* bot1 = bot_create(1, 3); 
    Bot* bot2 = bot_create(2, 3);

    // Make sure we check for nulls 
    if (!bot1 || !bot2) return 1;

    BotController* bc1 = bot_controller_create(bot1, 0); 
    BotController* bc2 = bot_controller_create(bot2, 1);

    bot_controller_start(bc1);
    bot_controller_start(bc2);

    int prev_lives1 = 3;
    int prev_lives2 = 3;
    oled_update_score(3, 3);

    printf("Game Loop Started. Waiting for inputs...\n");
    fflush(stdout);

    // 3. The Loop now checks 'keep_running'
    while (keep_running) {

        int drive1, drive2, swing1, swing2;
        bot_get_command(bot1, &drive1, &swing1);
        bot_get_command(bot2, &drive2, &swing2);

        network_send_command_to_bot(1, drive1, swing1);
        network_send_command_to_bot(2, drive2, swing2);

        HitEvent event;
        while (network_poll_hit_event(&event)) {
            if (event.bot_id_hit == 1) HandleHit(bot1);
            else if (event.bot_id_hit == 2) HandleHit(bot2);
        }

        // Update OLED
        int lives1 = bot_get_lives(bot1);
        int lives2 = bot_get_lives(bot2);

        if (lives1 != prev_lives1 || lives2 != prev_lives2) {
            oled_update_score(lives1, lives2);
            prev_lives1 = lives1;
            prev_lives2 = lives2;
        }

        // Check Win Condition
        if (lives1 <= 0 || lives2 <= 0) {
            printf("Game over! ");
            if (lives1 <= 0 && lives2 <= 0) printf("Draw.\n");
            else if (lives1 <= 0) printf("Bot 2 wins!\n");
            else printf("Bot 1 wins!\n");
            
            fflush(stdout);
            keep_running = 0; // Break the loop manually
        }

        usleep(20 * 1000); 
    }

    printf("Cleaning up resources...\n");
    
    // EMERGENCY STOP: Send Stop commands to robots in case they were moving when the game quit
    for(int i=0; i<5; i++) {
        network_send_command_to_bot(1, 0, 0);
        network_send_command_to_bot(2, 0, 0);
        usleep(10000);
    }

    bot_controller_stop(bc1);
    bot_controller_stop(bc2);
    bot_controller_join(bc1);
    bot_controller_join(bc2);

    bot_controller_destroy(bc1);
    bot_controller_destroy(bc2);
    bot_destroy(bot1);
    bot_destroy(bot2);
    
    printf("Shutdown complete.\n");
    fflush(stdout);

    return 0;
}