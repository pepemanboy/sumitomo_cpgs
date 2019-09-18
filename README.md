# Production monitor

We developed electronics, firmware, and software to create a system that monitors a production line in a wire assembly factory.

The following diagram illustrates the system:
CPG is the customer's propietary hardware, that emits a digital signal when a part is completed.

There can exist multiple systems in the same physical space, thus we have to deal with different RF channels to avoid interference.



## Algorithm

### CPG signal
Slave polls the digital signal of the CPG, and increments an internal counter.

### RF channels
Each master has its own RF channel. 
Each master will broadcast in the main (default) channel every few seconds, a packet indicating which slaves shall go to their channel.
For example... Master 1 broadcasts a packet saying "Slave 1, 4, 27 and 13 go to channel 55".

If a slave listens to this, and its ID corresponds to an ID in the packet, it will go to the desired channel.
This way, we can have multiple masters in the same physical area, without having interference in the same channel.

### Polling for slaves
In its channel, master will send a message with a slave ID, requesting to get the value of its counter.
For example, master sends "Slave ID 4, message ID 5"
If a slave recieves this, and its ID corresponds to the slave ID in the packet, it will transmit the counter value, as well as a message ID.
This message ID is autoincremented in the slave, and is useful to know if the last message ID was received by the master, so that we can assure that the counter in the master and the counter in the slave are in sync. More details can be found in the implementation.

Master will do this sequentially for all its slaves over and over again.

## Installation instructions
Clone ciropkt repo in the same level.
Run src/external/ciropkt.bat
