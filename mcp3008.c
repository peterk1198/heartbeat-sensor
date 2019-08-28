#include "spi.h"
#include "mcp3008.h"
#include "ringbuffer.h"

void mcp3008_init(void) 
{
    spi_init(SPI_CE0, 500000); // aiming at (25 * 10^7) / (5 * 10^5) = 500Hz, but according to people online, will probably be ~600Hz
                             // either way, we're still getting close to the speed we need to accurately sample a human heartbeat
}

unsigned int mcp3008_read( unsigned int channel ) {
    unsigned char tx[3];
    unsigned char rx[3];

    // "Start bit", wakes up the ADC chip. 
    tx[0] = 1;
    // "Configuration byte", single mode + channel
    tx[1] = 0x80 | ((channel & 0x7) << 4);
    tx[2] = 0;

    spi_transfer(tx, rx, 3);
    return ((rx[1] & 0x3) << 8) + rx[2];
}
