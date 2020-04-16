/*
Author Sajid Choudhry	Mar 2020
Based on Dr.Carey Williamson's code

Program that runs simulations of 5 routing algorithms, on a set of provided data.
The program displays statistics about successful/blocked calls, and average hops/propogation delay for each algorithm.
The 5 algorithms are:
    SHPF
    SDPF
    LLP
    MFC
    SHPO(bonus)


Usage:
	Ensure that 'topology.dat' and 'callworkload.dat' are in the directory this program is running from.
    Compile the program using:
        g++ routing.cpp
    Run the program using:
        ./a.out



References:

Dr.Carey Williamson - Professor, University of Calgary
Sina Keshvadi- TA, University of Calgary
Rachel Mclean- TA, University of Calgary

*/



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <cstring>
using namespace std;







//store individual event from call file
class CallEvent
{
    public:
        double startTime;
        double duration;
        double endTime;
        int source;
        int destination;
        bool running = false;
        int resources[26][26]{};

        CallEvent(double s, double d, int sr, int dst)
        {
            startTime = s;
            duration = d;
            endTime = s+d;
            source = sr;
            destination = dst;
            running = false;
        }

        void reset()
        {
            running = false;

            for (int i = 0; i < 26; i++)
            {
                for (int j = 0; j < 26; j++)
                    resources[i][j] = 0;
            } 
        }
};






//Global arrays- index # represents node #
float propDelay[26][26]{};      //propogation for edges between nodes
float capacity[26][26]{};         //total capacity on each edge. Immutable
float availCap[26][26]{};         //available capacity on each edge
float cost[26][26]{};           //updates with the cost of edges based on the currently chosen algorithm
vector<CallEvent> eventQueue;

//statistics
double blockedCalls = 0;
double succCalls = 0;
double totalSuccCalls = 0;
double totalCalls = 0;
double totalHops = 0;
double totalProp = 0;

double blockedPercent = 0;
double succPercent = 0;
double avgHop = 0;
double avgProp = 0;



//clear values for next algorithm use
void clearValues()
{
    blockedCalls = 0;
    succCalls = 0;
    totalSuccCalls = 0;
    totalHops = 0;
    totalProp = 0;

    blockedPercent = 0;
    succPercent = 0;
    avgHop = 0;
    avgProp = 0;

    memcpy(availCap, capacity, 26*26*sizeof(int));

    //empty event attributes
    for(int i = 0; i < eventQueue.size(); i++)
        eventQueue[i].reset();

    memset(cost, 0, sizeof(cost[0][0]) * 26 * 26);


}


//printer function for debugging; takes in 2D array and prints its elements
void pr(int (&ar)[26][26])
{
    int l = 65;
    int n = 65;
    int flag = 0;
    for (int k = 0; k < 26; k++)
    {
                    if ( k== 0)
                    {  
                        cout << "\t";
                        while(l <= (65+25) )
                        {
                            cout << (char)l << "\t";
                            l++;
                        }       
                     }
            cout << endl;
            for (int m = 0; m < 26; m++)
            {
                        if(m == 0 && flag == 0)
                        {   
                                    if(n <= (65+25) )
                                    {
                                        cout << (char)n << "\t";
                                        n++;
                                        flag = 1;
                                    }
                        }

                        if(m == 25)
                            flag = 0;

                        if ( k == m)
                             cout << "//" << "\t";

                        else
                            cout << ar[k][m] << "\t";
            }
    }
}//end printer











//print titles
void printInit()
{
    printf("%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\t%-12s\n%-12s\n",
        "Policy","Total Calls","Successful","Success(%)","Blocked", "Blocked(%)", "Avg Hops(ms)", "Avg Delay",
        "=========================================================================================================================");
}

//print resulting stats
void printRes(const char *s)
{
    printf("%-12s\t%-12.0f\t%-12.0f\t%-12.2f\t%-12.0f\t%-12.2f\t%-12.4f\t%-12.4f\n",
       s, totalCalls, succCalls, succPercent, blockedCalls, blockedPercent, avgHop, avgProp);

}





//recover resources for finished calls
void timeUpdate(int currentIndex)
{
    //for all events
    for (int j = 0; j < currentIndex ; j++)
    {
        //that are running
        if (eventQueue.at(j).running)
        {
            //if the event has an end time <= then current events start time, then end that event
            if(eventQueue.at(j).endTime <= eventQueue.at(currentIndex).startTime )
            {
                //end event
                eventQueue.at(j).running = false;

                //reclaim that events resources
                for (int k = 0; k < 26; k++)
                {
                    for (int m = 0; m < 26; m++)
                    {   
                        //remove accumulated resources from finished event, and place it back in available capacity array
                        if(eventQueue.at(j).resources[k][m] > 0)
                        {
                            availCap[k][m] = availCap[m][k]
                                    += eventQueue.at(j).resources[k][m];    
                            eventQueue.at(j).resources[k][m]
                                    = eventQueue.at(j).resources[m][k] = 0;
                        }
                    }    
                }
            }
        }
    }
}





//update cost array associated with SHPF algorithm
void updateSHPF()
{
    //init costs to 1 for SHPF cost array
    for (int k = 0; k < 26; k++)
    {
        for (int m = 0; m < 26; m++)
        {
            if(availCap[k][m] > 0)
            {
                cost[k][m] = 1;
                cost[m][k] = 1;
            }
            else
            {
                cost[k][m] = 0;
                cost[m][k] = 0;
            }
        }
        
    }

}




//update edge costs for LLP algorithm
void updateLLP()
{

    for (int k = 0; k < 26; k++)
    {
        for (int m = 0; m < 26; m++)
        {
            cost[k][m] = 1-((float)(availCap[k][m]) / (float)(capacity[k][m]));
        }
        
    }
}




//update edge costs for MFC algorithm
void updateMFC()
{

    for (int k = 0; k < 26; k++)
    {
        for (int m = 0; m < 26; m++)
        {
            cost[k][m] = ((float)(availCap[k][m]) / (float)(capacity[k][m]));
        }
        
    }
}





//determine the minimal entry in an array and return the index of it
int minDistVertexInQueue( int dist[26], int queue[26])
{
    int minVal = 999;       //infinity
    int minIndex = 50;      //infinity index
    for (int i = 0; i < 26; i++)
    {
        //if node is unvisited, and distance of that node is 
        if( (queue[i] == 1) && (dist[i] < minVal) )
        {
            minVal = dist[i];
            minIndex = i;
        }      
    }

    return minIndex;
}





//run djikstras algorithm on a Graph between 2 points, and provided edge weights FOR SHPO
int emptySHPO(int source, int destination, float (&edgeCost)[26][26], float (&capZero)[26][26], int current)
{

    //initialize queue of vertices
    //      (elem=1: index is a vertex in queue; elem=0: index is a vertex not in queue)
    int queue[26]{};
    int size = 0;  //queue size
    
    //initialize previous vertex for path tracking
    //  (index- the vertex; elem- the previous vertex of the index vertex)
    int previousVertex[26]{};
    int dist[26]{};

    for (int i = 0; i < 26; i++)
    {
        //initialize distance from each vertex to source to INF (index= vertex; elem=distance to it)
        dist[i] = 999;

        //init all prev vertex(element) or a vertex(index) to null
        previousVertex[i] = -1;
    }

    //init queue to determine which vertices exist on empty net
    for (int i = 0; i < 26; i++)
    {
        for (int j = 0; j < 26; j++)
        {
            if(capacity[i][j] > 0)
            {
                if(queue[i] == 0)
                {
                    queue[i] = 1;
                    size = size+1;

                }
                    
            }
        }
        
    }

    //set source vertex's distance as 0
    dist[source] = 0;

    int foundFlag = 0;



    //run djikstra's algorithm on empty network to determine hops
    while(size > 0 )
    {
        //determine which vertex(index) in queue has the current minimum distance
        int u = minDistVertexInQueue(dist, queue);

        //stop searching  if landed in terminal of a forest
        if( u > 27)
        {
            break;
        }

        //remove vertex u from queue
        queue[u] = 0;

        //stop searching if destination vertex found
        if(u == destination)
        {
            foundFlag = 1;
            break;
        }

        //decrease size of queue
        size = size-1;




        //for all the vertices
        for (int j = 0; j < 26; j++)
        {
            //other than 'u' that remain in the queue (aka are set to 1)
            if(queue[j] > 0)
            {
                //find out which ones(j) are neighbours of u

                    //[u- current vertex from queue][j- its potential neighbour]
                    if(capZero[u][j] > 0)
                    {
                        //determine distance to these neighbours from u
                        int alt = dist[u] + edgeCost[u][j];

                        //update distance of j from source, if its less than existing distance
                        if(alt < dist[j])
                        {
                            dist[j] = alt;
                            previousVertex[j] = u;  //the previous vertex of j is now the vertex u.
                        }
                    }
            }
        }
          

    }
    //quit if no path was found
    if(foundFlag != 1)
    {
        return 999; //hopPath 2 = infinity
    }
        
    //determine hops for this run
     int curr = destination;
     int hopPath1 = 0; 

    while(previousVertex[curr] > -1)
    { 

        //update total number of hops
        hopPath1++;

        curr = previousVertex[curr];
    }

    return hopPath1;

} //end updateStateSHPO




//run djikstras algorithm on a Graph between 2 points, and provided edge weights
bool updateState(int source, int destination, float (&edgeCost)[26][26], int current)
{

    //initialize queue of vertices
    //      (elem=1: index is a vertex in queue; elem=0: index is a vertex not in queue)
    int queue[26]{};
    int size = 0;  //queue size

    //initialize previous vertex for path tracking
    //  (index- the vertex; elem- the previous vertex of the index vertex)
    int previousVertex[26]{};
    int dist[26]{};

    for (int i = 0; i < 26; i++)
    {
        //initialize distance from each vertex to source to INF (index= vertex; elem=distance to it)
        dist[i] = 999;

        //init all prev vertex(element) or a vertex(index) to null
        previousVertex[i] = -1;
    }

    //init queue to determine which vertices exist in this run(1-exists, 0- not exists)
    for (int i = 0; i < 26; i++)
    {
        for (int j = 0; j < 26; j++)
        {
            if(availCap[i][j] > 0)
            {
                if(queue[i] == 0)
                {
                    queue[i] = 1;
                    size = size+1;

                }
                    
            }
        }
        
    }

    //set source vertex's distance as 0
    dist[source] = 0;

    int foundFlag = 0;



    //run djikstra's algorithm
    while(size > 0 )
    {
        //determine which vertex(index) in queue has the current minimum distance
        int u = minDistVertexInQueue(dist, queue);

        //stop searching  if landed in terminal of a forest
        if( u > 27)
        {
            break;
        }

        //remove vertex u from queue
        queue[u] = 0;

        //stop searching if destination vertex found
        if(u == destination)
        {
            foundFlag = 1;
            break;
        }

        //decrease size of queue
        size = size-1;




        //for all the vertices
        for (int j = 0; j < 26; j++)
        {
            //other than 'u' that remain in the queue (aka are set to 1)
            if(queue[j] > 0)
            {
                //find out which ones(j) are neighbours of u

                    //[u- current vertex from queue][j- its potential neighbour]
                    if(availCap[u][j] > 0)
                    {
                        //determine distance to these neighbours from u
                        int alt = dist[u] + edgeCost[u][j];

                        //update distance of j from source, if its less than existing distance
                        if(alt < dist[j])
                        {
                            dist[j] = alt;
                            previousVertex[j] = u;  //the previous vertex of j is now the vertex u.
                        }
                    }
            }
        }
          

    }
    //quit if no path was found
    if(foundFlag != 1)
    {
        return false;
    }
        
                
    //update topology state and statistics
    int curr = destination; 

    while(previousVertex[curr] > -1)
    { 
        //decrease available circuits for current link
        availCap[curr][previousVertex[curr]] = --availCap[previousVertex[curr]][curr];

        //update current events attained resources
        eventQueue.at(current).resources[curr][previousVertex[curr]] = ++eventQueue.at(current).resources[previousVertex[curr]][curr];
        
        //update total propDelay
        totalProp += propDelay[curr][previousVertex[curr] ];


        //update loop iterator
        curr = previousVertex[curr];

        //update total number of hops
        totalHops++;
    }

    return true;
            


} //end updateState
















///Main function////////////////////////////////////////////



int main( int argc, char ** argv)
{

//Load in the topology file
    ifstream file;
    file.open("topology.dat");

    //load in topology
    if(file.is_open() )
    {
        char a,b;
        int c,d;

        //while !EOF
        while(file >> a >> b >> c >> d)
        {
            int src = a - 65;
            int dest = b - 65;

            //create seperate arrays with index representing Node Letter
            propDelay[src][dest] = propDelay[dest][src] = c;
            capacity[src][dest] = capacity[dest][src] = d;
            availCap[src][dest] = availCap[dest][src] = d;

        }      

    }
    else
    {
        cout << "error" << endl;
    }
    

//Load in the Calls file
    ifstream file2;
    file2.open("callworkload.dat");

    //load in calls
    if(file2.is_open() )
    {
        char b,c;
        double a,d;

        //while !EOF
        while(file2 >> a >> b >> c >> d)
        {
            //increment # of calls in file
            totalCalls++;

            //add event to queue
            int src = b - 65;
            int dest = c - 65;
            eventQueue.push_back( CallEvent(a,d,src,dest)  );
        }
    }



///print titles
    printInit();        




///////Run the algorithms////////////////////////////////////////////////////





//SDPF
    //event tracker
    int i= 0;

    //current event attributes
    int src;
    int dst;
    double start;
    double end;


    while(1)
    {
                while(1)
                    {
                                //load in event
                                if(i < eventQueue.size() )
                                {
                                    
                                    //handle time related event updates
                                    timeUpdate(i);
                                    
                                    
                                    //load in current event
                                    src = eventQueue.at(i).source;
                                    dst = eventQueue.at(i).destination;
                                    start = eventQueue.at(i).startTime;
                                    end = eventQueue.at(i).endTime;
                                    eventQueue.at(i).running = true;

                                    //update edge costs
                                    updateSHPF();
                                }
                                else
                                {
                                    //All events processed, print and exit
                                    break;
                                }
                                



                                //run djikstra's algorithm on current event to determine reachability and update statistics as such
                                if((updateState(src, dst, cost, i) == false) )
                                {
                                    
                                    //no path available
                                    blockedCalls++;

                                    //set event to finished
                                    eventQueue.at(i).running = false;
                                }
                                else
                                {
                                    //path available
                                    succCalls++;
                                }

                                //next event
                                i++;
                    }//end while

                    //calculate and print statistics
                    avgProp = totalProp/succCalls;
                    avgHop = totalHops/succCalls;
                    succPercent = succCalls/totalCalls *100;
                    blockedPercent = blockedCalls/totalCalls *100;
                    printRes("SHPF");
                    clearValues();
                    break;
    }








//SDPF
    //event tracker
    i= 0;

    //current event attributes
    src = 0;
    dst = 0;
    start = 0;
    end = 0;



    while(1)
    {
                while(1)
                    {
                                //load in event
                                if(i < eventQueue.size() )
                                {
                                    
                                    //handle time related event updates
                                    timeUpdate(i);
                                    
                                    
                                    //load in current event
                                    src = eventQueue.at(i).source;
                                    dst = eventQueue.at(i).destination;
                                    start = eventQueue.at(i).startTime;
                                    end = eventQueue.at(i).endTime;
                                    eventQueue.at(i).running = true;

                                }
                                else
                                {
                                    //All events processed, print and exit
                                    break;
                                }
                                



                                //run djikstra's algorithm on current event to determine reachability and update statistics as such
                                if((updateState(src, dst, propDelay, i) == false) )
                                {
                                    
                                    //no path available
                                    blockedCalls++;

                                    //set event to finished
                                    eventQueue.at(i).running = false;
                                }
                                else
                                {
                                    //path available
                                    succCalls++;
                                }

                                //next event
                                i++;
                    }//end while

                    //calculate and print statistics
                    avgProp = totalProp/succCalls;
                    avgHop = totalHops/succCalls;
                    succPercent = succCalls/totalCalls *100;
                    blockedPercent = blockedCalls/totalCalls *100;
                    printRes("SDPF");
                    clearValues();
                    break;
    }
            









//LLP
        //event tracker
    i= 0;

    //current event attributes
    src = 0;
    dst = 0;
    start = 0;
    end = 0;



    while(1)
    {
                while(1)
                    {
                                //load in event
                                if(i < eventQueue.size() )
                                {
                                    
                                    //handle time related event updates
                                    timeUpdate(i);
                                    
                                    
                                    //load in current event
                                    src = eventQueue.at(i).source;
                                    dst = eventQueue.at(i).destination;
                                    start = eventQueue.at(i).startTime;
                                    end = eventQueue.at(i).endTime;
                                    eventQueue.at(i).running = true;

                                    //update edge costs
                                    updateLLP();
                                }
                                else
                                {
                                    //All events processed, print and exit
                                    break;
                                }
                                



                                //run djikstra's algorithm on current event to determine reachability and update statistics as such
                                if((updateState(src, dst, cost, i) == false) )
                                {
                                    
                                    //no path available
                                    blockedCalls++;

                                    //set event to finished
                                    eventQueue.at(i).running = false;
                                }
                                else
                                {
                                    //path available
                                    succCalls++;
                                }

                                //next event
                                i++;
                    }//end while

                    //calculate and print statistics
                    avgProp = totalProp/succCalls;
                    avgHop = totalHops/succCalls;
                    succPercent = succCalls/totalCalls *100;
                    blockedPercent = blockedCalls/totalCalls *100;
                    printRes("LLP");
                    clearValues();
                    break;
    }







//MFC

    //event tracker
    i= 0;

    //current event attributes
    src = 0;
    dst = 0;
    start = 0;
    end = 0;


    while(1)
    {
                while(1)
                    {
                                //load in event
                                if(i < eventQueue.size() )
                                {
                                    
                                    //handle time related event updates
                                    timeUpdate(i);
                                    
                                    
                                    //load in current event
                                    src = eventQueue.at(i).source;
                                    dst = eventQueue.at(i).destination;
                                    start = eventQueue.at(i).startTime;
                                    end = eventQueue.at(i).endTime;
                                    eventQueue.at(i).running = true;

                                    //update edge costs
                                    updateMFC();
                                }
                                else
                                {
                                    //All events processed, print and exit
                                    break;
                                }
                                



                                //run djikstra's algorithm on current event to determine reachability and update statistics as such
                                if((updateState(src, dst, cost, i) == false) )
                                {
                                    
                                    //no path available
                                    blockedCalls++;

                                    //set event to finished
                                    eventQueue.at(i).running = false;
                                }
                                else
                                {
                                    //path available
                                    succCalls++;
                                }

                                //next event
                                i++;
                    }//end while

                    //calculate and print statistics
                    avgProp = totalProp/succCalls;
                    avgHop = totalHops/succCalls;
                    succPercent = succCalls/totalCalls *100;
                    blockedPercent = blockedCalls/totalCalls *100;
                    printRes("MFC");
                    clearValues();
                    break;
    }






//SHPO

    //event tracker
    i= 0;

    //current event attributes
    src = 0;
    dst = 0;
    start = 0;
    end = 0;


    while(1)
    {
                while(1)
                    {
                                //load in event
                                if(i < eventQueue.size() )
                                {
                                    
                                    //handle time related event updates
                                    timeUpdate(i);
                                    
                                    
                                    //load in current event
                                    src = eventQueue.at(i).source;
                                    dst = eventQueue.at(i).destination;
                                    start = eventQueue.at(i).startTime;
                                    end = eventQueue.at(i).endTime;
                                    eventQueue.at(i).running = true;

                                    //update edge costs
                                    updateSHPF();
                                }
                                else
                                {
                                    //All events processed, print and exit
                                    break;
                                }
                                



                                //run djikstra's algorithm on current event to determine unreachability , or if hop2 > hopEmpty. block calls on both
                                if( emptySHPO(src, dst, cost, availCap , i) > emptySHPO(src, dst, cost, capacity , i)    )
                                {
                 
                                    //no path available
                                    blockedCalls++;

                                    //set event to finished
                                    eventQueue.at(i).running = false;
                                }
                                else
                                {
                                    //update stats with hop2 results
                                    updateState(src, dst, cost, i);

                                    //path available
                                    succCalls++;
                                }

                                //next event
                                i++;
                    }//end while

                    //calculate and print statistics
                    avgProp = totalProp/succCalls;
                    avgHop = totalHops/succCalls;
                    succPercent = succCalls/totalCalls *100;
                    blockedPercent = blockedCalls/totalCalls *100;
                    printRes("SHPO");
                    clearValues();
                    break;
    }
}//end mian


