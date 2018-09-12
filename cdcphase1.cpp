#include <cstdlib>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include  <signal.h>
#include <list>
#include <pthread.h>

#include "GodProcess.h"

using namespace std;

/*
 * 
 */

void SIGINT_handler(int sig)
{
    
}

string gettimeinbuffer(timeval time_val)
{
    char time[100];
    time_t timet;
    timet = time_val.tv_sec;
    tm *a;
    a = localtime(&timet);
    strftime(time, sizeof(time), "%H:%M:%S", a);
    string timestring;
    timestring.assign(time);
    int microseconds = time_val.tv_usec/1000;
    sprintf(time, " %d", microseconds);
    timestring.append(time);
    return timestring;
}

int main(int argc, char** argv) {

    //god process will record its start time
    string god_configuration_filename;
    god_configuration_filename.assign(argv[1]);
    GodProcess godProcess;
    godProcess.SetGodConfigurationFilename(god_configuration_filename);
    godProcess.Start();
    return 0;
}

