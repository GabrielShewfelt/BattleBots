#ifndef RPI_COMMUNICATION_H
#define RPI_COMMUNICATION_H

//Call at startup
int setup(void);

// Movement/Arm commands. 'player' is 1 or 2.
void moveForward(int player);
void moveBack(int player);
void moveStop(int player);

void armUp(int player);
void armDown(int player);
void armStop(int player);

#endif
