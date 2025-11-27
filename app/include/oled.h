#ifndef OLED_H
#define OLED_H

// Initialize the OLED display once at program startup.
// "Are you ready?"
void oled_init(void);

// Update the scoreboard display with current lives.
// Keeps "Are you ready?" at the top and shows Bot1/Bot2 below.
void oled_update_score(int lives1, int lives2);

#endif
