#ifndef CONTOLLER_H
#define CONTROLLER_H

#include <stdint.h>

typedef struct {
    int16_t stick_x;  // -32768 .. +32767
    int16_t stick_y;
    int button_a;
    int button_b;
    int button_x;
    int button_y;
} ControllerState;

void controller_init(void);
int controller_get_state(int controller_index, ControllerState *out);

#endif
