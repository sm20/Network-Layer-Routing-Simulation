# Network-Layer-Routing-Simulation

Exploration of the effectiveness of different routing algorithms (described below) for a circuit-switched network when provided with a network topology and a network usage log. This is done through this program using discrete-event simulation. Statistics are provided after the program completes reading the input log file.

### Format of the network call warkload input file
The call workload for your network is specified in a simple four-column format, like the following:
* Column 1: time (in minutes) at which the call arrives at the network
* Column 2: the source node for the call
* Column 3: the destination node for the call
* Column 4: the time duration (in minutes) that the call remains in the network
* Example
    * 0.123456   A   D   12.527453

## Routing Algorithms considered:
* **Shortest Hop Path First (SHPF)**: This algorithm tries to find the shortest path currently available from source to destination, where the length of a path refers to the number of hops (i.e., links) traversed. Note that this algorithm ignores the propagation delay associated with each link.
* **Shortest Delay Path First (SDPF)**: This algorithm tries to find the shortest path currently available from source to destination, where the length of a path refers to the cumulative total propagation delay for traversing the chosen links in the path. Note that this algorithm ignores the number of hops.
* **Least Loaded Path (LLP)**: This algorithm tries to find the least loaded path currently available from source to destination, where the load of a path is determined by the "busiest" link on the path, and the load on a link is its current utilization (i.e., the ratio of its current number of active calls to the capacity C of that link for carrying calls). Note that this algorithm ignores propagation delays and the number of hops.
* **Maximum Free Circuits (MFC)**: This algorithm tries to find the currently available path from source to destination that has the most free circuits, where the number of free circuits for a candidate path is the smallest number of free circuits currently observed on any link in that possible path. Note that this algorithm ignores propagation delays, the number of hops, and the utilization of each link.

### Statistics Provided:
* the total number of calls read from the workload file;
* the number (and percentage) of successfully routed calls;
* the number (and percentage) of blocked calls;
* the average number of hops (links) consumed per successfully routed call; and
* the average end-to-end (source-to-destination) propagation delay per successfully routed call.



