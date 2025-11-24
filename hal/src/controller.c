#include <stdio.h>  
#include "hal/controller.h"

void controller_init(void) {
    printf("Controller Init (Dummy)\n");
}

int controller_get_state(int controller_index, ControllerState *out) {
    // 1. Silence "unused parameter" error by casting to void
    (void)controller_index;

    // 2. actually use 'out' to silence its warning and prevent crashes
    if (out) {
        out->stick_x = 0;
        out->stick_y = 0;
        out->button_a = 0;
        out->button_b = 0;
        out->button_x = 0;
        out->button_y = 0;
    }

    // 3. Silence "control reaches end of non-void function" error
    return 0; 
}