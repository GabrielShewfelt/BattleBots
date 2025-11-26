#include <stdio.h>
#include <stdlib.h>
#include "oled.h"

// Path to the OLED helper binary
#define OLED_BIN_PATH "ssd1306_linux/ssd1306_bin"

#define OLED_BUS      1        // /dev/i2c-1
#define OLED_SIZE     "128x64" // your display resolution

// helper to build and run a command.
static void oled_run(const char *args)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "%s -n %d %s",
             OLED_BIN_PATH, OLED_BUS, args);
    system(cmd);
}

void oled_init(void)
{
    // Initialize 128x64 on I2C bus 1
    oled_run("-I " OLED_SIZE);

    // Clear the screen
    oled_run("-c");

    // First line: "Are you ready?"
    oled_run("-x 0 -y 0 -l \"Are you ready?\"");
}

void oled_update_score(int lives1, int lives2)
{
    char buf[64];

    // Clear and re-draw the title
    oled_run("-c");
    oled_run("-x 0 -y 0 -l \"Lets do it!\"");

    // Line 2: Bot 1 lives
    snprintf(buf, sizeof(buf),
             "-x 0 -y 2 -l \"Bot1: %d\"",
             lives1);
    oled_run(buf);

    // Line 3: Bot 2 lives
    snprintf(buf, sizeof(buf),
             "-x 0 -y 3 -l \"Bot2: %d\"",
             lives2);
    oled_run(buf);
}
