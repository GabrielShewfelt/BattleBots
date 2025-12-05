/*
    This is a test script for controllers
*/

#include <stdio.h>
#include <unistd.h>
#include "hal/controller.h"

int main(void) {
    printf("=== CONTROLLER TEST SCRIPT ===\n");
    printf("Initializing Controllers (Looking for /dev/input/js*)...\n");
    
    controller_init();

    printf("Reading inputs. Press Ctrl+C to exit.\n\n");

    ControllerState c1, c2;

    while (1) {
        // Read both controllers
        // Note: If only one is plugged in, the other will just stay 0
        controller_get_state(0, &c1);
        controller_get_state(1, &c2);

        // Print formatted output on a single line
        printf("\r[P1] X:%6d Y:%6d [A:%d B:%d]  ||  [P2] X:%6d Y:%6d [A:%d B:%d]",
            c1.stick_x, c1.stick_y, c1.button_a, c1.button_b,
            c2.stick_x, c2.stick_y, c2.button_a, c2.button_b
        );
        
        // Force print immediately (otherwise it waits for a newline)
        fflush(stdout);

        usleep(50000); // Update every 50ms (20Hz)
    }

    return 0;
}