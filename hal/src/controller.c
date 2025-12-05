#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <string.h>
#include <errno.h>
#include "hal/controller.h"

#define MAX_CONTROLLERS 2

static int js_fds[MAX_CONTROLLERS] = {-1, -1};
static ControllerState current_states[MAX_CONTROLLERS];

void controller_init(void) {
    char path[32];
    // Try to open js0 and js1
    for (int i = 0; i < MAX_CONTROLLERS; i++) {
        snprintf(path, sizeof(path), "/dev/input/js%d", i);
        
        // Open in NON-BLOCKING mode so we don't freeze the game loop
        js_fds[i] = open(path, O_RDONLY | O_NONBLOCK);
        
        if (js_fds[i] < 0) {
            printf("[HAL] Warning: Could not open %s. Is the adapter running?\n", path);
        } else {
            printf("[HAL] Controller %d connected at %s\n", i, path);
        }
        
        // Clear state
        memset(&current_states[i], 0, sizeof(ControllerState));
    }
}

int controller_get_state(int controller_index, ControllerState *out) {
    if (controller_index < 0 || controller_index >= MAX_CONTROLLERS) return 0;
    
    int fd = js_fds[controller_index];
    if (fd < 0) {
        return 0; 
    }

    struct js_event e;
    
    // Drain the event queue for this frame
    // "read" will return -1 with errno=EAGAIN when there are no more events
    while (read(fd, &e, sizeof(e)) > 0) {
        
        // Mask out the INITIAL_STATE bit to just get the event type
        int type = e.type & ~JS_EVENT_INIT;

        if (type == JS_EVENT_BUTTON) {
            // Mapping for wii-u-gc-adapter (Standard Layout)
            switch (e.number) {
                case 0: current_states[controller_index].button_a = e.value; break;
                case 1: current_states[controller_index].button_b = e.value; break;
                case 2: current_states[controller_index].button_x = e.value; break;
                case 3: current_states[controller_index].button_y = e.value; break;
            }
        } 
        else if (type == JS_EVENT_AXIS) {
            // Mapping for wii-u-gc-adapter
            switch (e.number) {
                case 0: // Main Stick X
                    current_states[controller_index].stick_x = e.value; 
                    break;
                case 1: // Main Stick Y
                    // NOTE: Y-axis is inverted on joysticks (Up is negative)
                    current_states[controller_index].stick_y = -e.value; 
                    break;
            }
        }
    }

    // Copy the latest state to the output
    if (out) {
        *out = current_states[controller_index];
    }

    return 1;
}