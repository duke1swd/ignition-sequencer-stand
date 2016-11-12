/*
 * run.cpp is common code for both ignition testing and main engine burn.
 * run.h defines entry and exit into run.cpp
 */

#define	RUN_IG_ONLY	0
#define	RUN_MAIN_ENGINE	1

void runInit(unsigned char mode);
