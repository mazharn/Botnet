#ifndef PROCESSDETAILS_H
#define	PROCESSDETAILS_H

#include <string>
#include <string.h>
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
using namespace std;

enum MACHINE_STATUS { OFF = 0, ON = 1 };

struct ProcessDetails 
{
    int processID;
    string processName;
    int portNumber;
    MACHINE_STATUS status;
    int portAssociated;
    ofstream file_writer;
    static ProcessDetails * FindProcessWithPortNumber(int port, ProcessDetails * listOfProcesses, int numberOfProcesses)
    {
        for (int i=0; i<numberOfProcesses; i++)
        {
            if (port == listOfProcesses[i].portAssociated)
            {
                return &listOfProcesses[i];
            }
        }
        return NULL;
    }
    static ProcessDetails * FindProcessWithProcessName(string processName, ProcessDetails *listOfProcesses, int numberOfProcesses)
    {
        for (int i=0; i<numberOfProcesses; i++)
        {
            if (processName.compare(listOfProcesses[i].processName)==0)
            {
                return &listOfProcesses[i];
            }
        }
        return NULL;
    }
    static void SendListOfHosts(ProcessDetails *listOfProcesses, int total_communicators, int sockfd)
    {
        cout<<"CNC function reached"<<endl;
        char temp[1024];
        for (int i=0; i<total_communicators; i++)
        {
            if (listOfProcesses[i].status == ON && strncmp(listOfProcesses[i].processName.c_str(), "h", 1)==0)
            {
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "%s port=%d pid=%d", listOfProcesses[i].processName.c_str(), listOfProcesses[i].portNumber, listOfProcesses[i].processID);
                send(sockfd, (void*)temp, strlen(temp), 0);
                recv(sockfd, (void*)temp, sizeof(temp), 0);
                if (strncmp(temp, "ok", 2)!=0){
                    //retransmit
                    i--;
                }
            }
        }
        strcpy(temp, "<EndOfHosts>");
        send(sockfd, (void*)temp, strlen(temp), 0);
    }
};



#endif	/* PROCESSDETAILS_H */


