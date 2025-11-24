#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "bot_controller.h"
#include "hal/controller.h"  

struct BotController {
    Bot *bot;
    int controller_index;
    int running;
    pthread_t thread;
};

// Helper to map raw controller inputs to -1, 0, 1
static void map_input_to_command(const ControllerState *s, int *drive, int *swing) {
    // Drive Logic
    if (s->stick_y > 10000)       *drive = 1;   // Forward
    else if (s->stick_y < -10000) *drive = -1;  // Reverse
    else                          *drive = 0;   // Stop

    // Swing Logic (Buttons override stick)
    if (s->button_a)              *swing = 1;   // Swing Right/Up
    else if (s->button_b)         *swing = -1;  // Swing Left/Down
    else if (s->stick_x > 10000)  *swing = 1;
    else if (s->stick_x < -10000) *swing = -1;
    else                          *swing = 0;
}

static void* controller_thread(void *arg) {
    BotController *bc = arg;
    ControllerState state;

    while (bc->running) {
        // Poll the hardware abstraction layer
        if (controller_get_state(bc->controller_index, &state)) {
            int drive = 0;
            int swing = 0;

            // FIX 1: Pass addresses (&) so function can modify values
            map_input_to_command(&state, &drive, &swing);

            // LOGIC FIX: Actually update the bot object!
            bot_set_command(bc->bot, drive, swing);
        }

        usleep(10000); // Check every 10ms
    }

    return NULL;
}
 
BotController* bot_controller_create(Bot *bot, int controller_index) {
    BotController* bc = malloc(sizeof(BotController));
    if (!bc) return NULL;

    bc->bot = bot;
    bc->controller_index = controller_index;
    bc->running = 0;

    return bc;
}

int bot_controller_start(BotController *bc) {
    if (!bc) return -1;
    bc->running = 1;

    if (pthread_create(&bc->thread, NULL, controller_thread, bc)) {
        bc->running = 0;
        return -1;
    }

    return 0;
}

void bot_controller_stop(BotController *bc) {
    if (!bc) return;
    bc->running = 0;
}

void bot_controller_join(BotController *bc) {
    if (!bc) return;
    // FIX 2: pthread_join takes the value, not the address
    pthread_join(bc->thread, NULL);
}

void bot_controller_destroy(BotController *bc) {
    if (!bc) return;
    free(bc);
}