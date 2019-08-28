Heartbeat sensor with EKG readings

We divided our project so that Justin handled the system side and obtaining the BPM, while Peter handled the graphics, plotting the raw data of the values read from the pulse sensor, displaying BPM changes in real time, and pulsating the heart graphic. 

We used Professor Levis's code for the SPI bus and the ADC so that we could get the raw data that the pulse sensor was meant to be sending. It was tweaked a little in order to get the speed at which we sampled data from the pulse sensor. 

In the BPM implementation, we found some source code that the pulse sensor makers used to calculate BPM on the Arduino. 

Specifically, the idea to use a buffer to store only the 10 latest intervals between beats to calculate BPM weighted towards recent beats was from the manufacturers.
We got the idea to animate the heart from an online Pinterest post.

In terms of project goals, we aimed to get a working display with a clean UI that would accurately display BPM and the EKG graph. While we originally thought of making this project a stress test, we thought that taking a bit of a spin on the project to make it more about medical devices could be interesting. 

We designed the project with two primary files: display.c and pulsesensor.c. In pulsesensor, we wrote the driver to convert the raw data taken from the MCP and converting it to a BPM value. In display, we handled the graphics that would show on the screen.

Overall, we achieved a working display with accurate BPM measures (compared with wearable heartbeat trackers like a FitBit and an Apple watch). Shortly after the demo, we incorporated interrupts and a tweaked SPI bus and ADC to get a more graph display. The demo didn't go precisely how we would have wanted it to due to a missing team member, however we believe that with our work after the demo, we were able to get a more cohesive project with the incoporation of more systems code.

Some challenges we faced included trying to figure out how to get interrupts working with the SPI and determining when and where to swap buffers to create a smooth graphic display. Figuring out when to swap buffers was somewhat difficult in trying to write decomposed, robust code.
