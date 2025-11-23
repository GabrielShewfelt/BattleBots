#include <stdio.h>
#include <unistd.h> // For sleep() and usleep()
#include "network.h"

int main(void) {
    printf("=== NETWORK TEST SCRIPT ===\n");
    printf("1. Initializing Network (Scanning for Raspberry Pis)...\n");

    // Step 1: Initialize Network
    // This will block until both Pis respond to the broadcast
    if (network_init() != 0) {
        fprintf(stderr, "Failed to initialize network.\n");
        return 1;
    }
    printf("SUCCESS: Network initialized and bots connected.\n");
    sleep(1); // Brief pause

    // Step 2: Test Bot 1 Motors
    printf("\n2. Testing BOT 1 Movement...\n");
    
    printf("   -> Bot 1: FORWARD\n");
    network_send_command_to_bot(1, 1, 0); // Drive=1, Swing=0
    sleep(2);

    printf("   -> Bot 1: STOP\n");
    network_send_command_to_bot(1, 0, 0);
    sleep(1);

    printf("   -> Bot 1: SWING ARM UP\n");
    network_send_command_to_bot(1, 0, 1); // Drive=0, Swing=1
    sleep(1);

    printf("   -> Bot 1: ARM STOP\n");
    network_send_command_to_bot(1, 0, 0);
    sleep(1);

    // Step 3: Test Bot 2 Motors
    printf("\n3. Testing BOT 2 Movement...\n");
    
    printf("   -> Bot 2: REVERSE\n");
    network_send_command_to_bot(2, -1, 0); // Drive=-1, Swing=0
    sleep(2);

    printf("   -> Bot 2: STOP\n");
    network_send_command_to_bot(2, 0, 0);
    sleep(1);

    // Step 4: Test Hit Detection (Receiver)
    printf("\n4. Listening for incoming 'hit' packets for 10 seconds...\n");
    printf("   (Manually trigger a hit on the Pis now if possible)\n");

    for (int i = 0; i < 100; i++) { // Loop for roughly 10 seconds
        HitEvent event;
        
        // Poll for event
        if (network_poll_hit_event(&event)) {
            printf("   [!!!] HIT DETECTED on Bot ID: %d\n", event.bot_id_hit);
        }

        usleep(100 * 1000); // Sleep 100ms
    }

    printf("\n=== TEST COMPLETE ===\n");
    return 0;
}