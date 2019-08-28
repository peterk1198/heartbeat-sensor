#ifndef PULSESENSOR_H
#define PULSESENSOR_H

typedef struct {
    int bpm;
    int rate[10];
    int count;
    int lastBeatTime;
    int P;
    int T;
    int threshold;
    int amplitude;
    int firstBeat;
    int secondBeat;
    int interval;
    int pulse;
    int lastTime;
} reading;

void pulsesensor_init();
void pulsesensor_user_init();
int pulsesensor_read();
void pulsesensor_bpm();
void pulsesensor_bpmloop();
void pulsesensor_resetUser();

#endif
