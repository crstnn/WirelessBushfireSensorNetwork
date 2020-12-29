# Wireless Bushfire Sensor Network
Joint collaboration implemented in C using POSIX Threads and OpenMPI. 
A simulation of a WSN with sensors communicating with satellites in order 
to detect bushfires within a particular area.

![Network design layout (copyright-free image from
https://unsplash.com/photos/er7EeL-4MqQ)](https://github.com/crstnn/WirelessBushfireSensorNetwork/blob/main/images/network_image.png)

### DESIGN SCHEME FOR SENSOR NETWORK
We created a base station which always existed at the last rank on the MPI
network, and wherein the sensor nodes took occupation of all the
other ranks in the network. There are some strict specifications to
the running of this program as the user specifies command line
arguments of the cartesian grid that must follow an n * m form
factor. Then, a master (base station) node must be specified. If an n * m + 1
nodes are not specified an informative error ensues and the
user must re-specify the parameters given the aforementioned
schema.

Another command line argument that is specified by the user is
it’s interval (in seconds). This allows the user to decide the
network's polling frequency. This is an important feature as if a
bushfire sensor detection network does not have a wired
connection it may rely on solar power or even solely battery power
which could cause a fault in the entire network if the polling
frequency were to be too often to completely diminish all its power
resources.

Also, it should be noted that the user can specify the number of
iterations, in the command line arguments, that they would like the
network to function for. Its total runtime can be calculated by
multiplying the interval with the iterations argument.
The network proceeds to check for any local consensus of
overheated sensor nodes. This means that for a given node that has
achieved the threshold temperature (85° C) it sends a request to its
adjacent neighbor nodes (the number of requests range between 2
to 4 nodes depending where the sensor node is within the cartesian
grid). These packets are then sent back to the node attempting to
achieve local overheat consensus whereby the given sensor node
checks that at least 2 or more adjacent nodes are greater than the
threshold temperature minus 5 (> 85 - 5). If this occurs a struct is
sent back to the base station ready to post the detailed event
information to the specified log files, if this local consensus does
not occur then the packet is never sent and this process continues
to repeat. Of course, at the base station these overheat report
packets are cross-checked with the satellite sensor to ensure no
false positives. If the two temperatures match (from the satellite
and the sensor nodes) then it is logged as a ‘confirmed report’ or
otherwise a ‘false report’.

### TERMINATION
Sensor nodes create an IRecv message at the beginning of
execution listening for a message tagged
BASENODESHUTDOWN. Then, individually, as their final
instruction - they will check to see if this message was received. If
it is, they will run MPI_Finalize, simulating a shutdown.
This form of shutdown, is a generally termed a 'graceful
shutdown', as other processing is not interrupted and data/memory
can be nicely cleaned-up. An ungraceful or forced shutdown could
have been used in the situation where the responsiveness of the
shutdown is critical. For example, if the WSN nodes were unable
to be powered on in temperatures exceeding the threshold - forcing
them to shutdown would do so immediately and potentially
prevent the network from further damage.
