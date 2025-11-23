#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "bot_controller.h"
#include "hal/controller.h"   // for ControllerState + hw_get_controller_state

struct BotController {
    Bot *bot;
    int controller_index;
    int running;
    pthread_t thread;
};

static void map_input_to_command(const ControllerState *s, int *drive, int *swing) {
    // Drive Logic
    if (s->stick_y > 10000)       *drive = 1;   // Forward
    else if (s->stick_y < -10000) *drive = -1;  // Back
    else                          *drive = 0;   // Stop

    // Swing Logic
    // Prioritize buttons, fallback to stick
    if (s->button_a)              *swing = 1;   // Arm Up
    else if (s->button_b)         *swing = -1;  // Arm Down
    else if (s->stick_x > 10000)  *swing = 1;
    else if (s->stick_x < -10000) *swing = -1;
    else                          *swing = 0;   // Arm Stop
}

static void* controller_thread(void *argc) {
    BotController *bc = argc;
    ControllerState state;

    while (bc->running) {
        if (controller_get_state(bc->controller_index, &state)) {
            int drive, swing;
            map_input_to_command(&state, drive, swing);
        }

        usleep(5000); // 5 ms sleep
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
    pthread_join(&bc->thread, NULL);
}

void bot_controller_destroy(BotController *bc) {
    if (!bc) return;
    free(bc);

    bc = NULL;
}