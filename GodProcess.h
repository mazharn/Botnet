#ifndef GODPROCESS_H
#define	GODPROCESS_H

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

#include "ProcessDetails.h"
#include "CNCServer.h"
#include "InfectedHost.h"
#include "BotMaster.h"

using namespace std;



class GodProcess
{
private:
    string god_configuration_filename;
    string god_output_filename, cnc_configuration_file, bot_master_configuration_file, infected_host_configuration_file;
    int number_of_infected_hosts, number_of_kill_hosts;
    string myname;
    int god_pid;
    ofstream outputHandler;
    char temp[1024];
    bool LoadConfigurationFile();
    void CreateProcesses();
    bool OpenOutputFileHandler();
    void LogMessage(char * message);
    struct HostKill{
        int hostID;
        int time_to_kill;
    };
    HostKill * hosts_to_kill;
    ProcessDetails cncServer, botMaster, *infectedHosts;
    
public:
    void SetGodConfigurationFilename(string filename)
    {
        god_configuration_filename.assign(filename);
    }
    void Start();
};

#endif	/* GODPROCESS_H */


