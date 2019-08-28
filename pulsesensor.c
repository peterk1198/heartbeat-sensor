#include "pulsesensor.h"
#include "gpio.h"
#include "uart.h"
#include "printf.h"
#include "timer.h"
#include "strings.h"
#include "mcp3008.h"
#include "interrupts.h"
#include "gpioextra.h"
#include "ringbuffer.h"
#include "assert.h"

extern reading user;
volatile rb_t* rb;  

const unsigned int CLK = GPIO_PIN11;

/*
 * Interface for the Pulse sensor, given that the MCP3008 and SPI modules 
 * function correctly. We see that the values of the Pulse sensor outputted
 * fluctuate between 0 and 999. Not sure what the values represent in terms
 * of units, but through empirical experimentation, we know that each apex 
 * represents a beat.
 */
void wait_for_falling_clock_edge() {
    while (gpio_read(CLK) == 0) {}
    while (gpio_read(CLK) == 1) {}
}

void pulsesensor_handler(unsigned pc) {
    // ignore D_in and t_SAMPLE
    for (int i = 0; i < 7; i++) {
        wait_for_falling_clock_edge(); 
    }

    //if (gpio_check_and_clear_event(CLK) == 1) {
        int signal = mcp3008_read(0);
        rb_enqueue(rb, signal);
    //}
}

void setup_interrupts() {
    gpio_enable_event_detection(CLK, GPIO_DETECT_FALLING_EDGE);
    assert(interrupts_attach_handler(pulsesensor_handler));
    interrupts_enable_source(INTERRUPTS_GPIO3);
    interrupts_global_enable();
    rb = rb_new();
}

void pulsesensor_init() {
    gpio_init();
    uart_init();
    mcp3008_init();
    gpio_set_output(GPIO_PIN23);
}
   
// a beat occurs each time the sensor reading hits a max
int pulsesensor_read() {
    static int pulse = 0;
    // read channel 0
    int signal = mcp3008_read(0);
    
    /* 
     * Estimates:
     * diastole -> < 955
     * systole -> ~0
     */
    if (signal > 600 && pulse == 0) {
        gpio_write(GPIO_PIN23, 1);
        pulse = 1;
    } else if (signal < 600) {
        gpio_write(GPIO_PIN23, 0);
        pulse = 0;
    }

    return signal;
}

void pulsesensor_user_init() {
    user.bpm = 0;
    memset(user.rate, 0, sizeof(user.rate)); // track the last 10 intervals to derive a BPM reading

    user.count = 0;  // track when the pulse arrives
    user.lastBeatTime = 0;

    // on an EKG, a waveform has characteristics which physicians label using PQRST
    user.P = 512;    // peak
    user.T = 512;    // trough

    user.threshold = 525;
    user.amplitude = 0;

    // to accommodate lack of data for the first few seconds
    user.firstBeat = 1;
    user.secondBeat = 0;

    user.interval = 600; // track the interval between beats
    user.pulse = 0;
    user.lastTime = timer_get_ticks() / 1000;
}

void pulsesensor_bpmloop() {
    interrupts_global_disable();
    int signal = pulsesensor_read();
    int currentTime = timer_get_ticks() / 1000;
    user.count += currentTime - user.lastTime;   // maintain count in milliseconds
    user.lastTime = currentTime;

    int N = user.count - user.lastBeatTime;

    // peak and trough of pulse wave
    if (signal < user.threshold && N > ((double) user.interval / 5) * 3) {   // prevent interfering noise from previous beat
        if (signal < user.T) {
            user.T = signal;
        }
    }

    if (signal > user.threshold && signal > user.P) {
        user.P = signal;
    }

    if (N > 250) {  // avoid bouncing in signal
        if (signal > user.threshold && user.pulse == 0 && N > ((double) user.interval / 5) * 3) {
            user.pulse = 1;
            user.interval = user.count - user.lastBeatTime;
            user.lastBeatTime = user.count;
        }

        if (user.secondBeat) {
            user.secondBeat = 0;
            for (int i = 0; i < 10; i++) {
                user.rate[i] = user.interval;
            }
        }

        if (user.firstBeat) {
            user.firstBeat = 0;
            user.secondBeat = 1;
            interrupts_global_enable();
            return;
        }

        int pseudoTotal = 0;
        // populate rate array to eventually calculate BPM
        for (int i = 0; i < 9; i++) {
            user.rate[i] = user.rate[i + 1];
            pseudoTotal += user.rate[i];
        }
        user.rate[9] = user.interval;
        pseudoTotal += user.rate[9];

        pseudoTotal /= 10;
        user.bpm = 60000 / pseudoTotal; // adjust per minute
   }

   // mark where the beat is over
   if (signal < user.threshold && user.pulse == 1) {
       user.pulse = 0;
       user.amplitude = user.P - user.T;
       user.threshold = user.amplitude / 2 + user.T;
       user.P = user.threshold;
       user.T = user.threshold;
   }

   // reset values if we don't get any signal for a long time
   // probably switched users
   if (N > 2500) {
       pulsesensor_resetUser();
   }

   interrupts_global_enable();
}

void pulsesensor_resetUser() {
   user.threshold = 512;
   user.P = 512;
   user.T = 512;
   user.lastBeatTime = user.count;
   user.firstBeat = 1;
   user.secondBeat = 1;
   user.bpm = 0;
}

