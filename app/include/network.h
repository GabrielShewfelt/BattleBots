#ifndef NETWORK_H
#define NETWORK_H

typedef struct {
    int bot_id_hit;   // 1 or 2, etc.
} HitEvent;

int network_init();

// sends drive/swing command to pi
void network_send_command_to_bot(int index, int drive, int swing);

// returns 1 if an event was read, 0 if none
int network_poll_hit_event(HitEvent *out);


#endif