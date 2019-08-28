#include "gpio.h"
#include "uart.h"
#include "mcp3008.h"
#include "printf.h"
#include "shell.h"
#include "pulsesensor.h"
#include "display.h"
#include "gl.h"
#include "timer.h"

volatile reading user = {0};

void main(void) 
{
    display_init();
    pulsesensor_init();
    pulsesensor_user_init();
    int currbpm = 0;

    while (1) {
        pulsesensor_bpmloop();
        if (user.bpm != currbpm) {
            update_bpm(user.bpm);
            write_graph(user.bpm);
            pulsate_heart(user.bpm);
            currbpm = user.bpm;
        }
    }
}
