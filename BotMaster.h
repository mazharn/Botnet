#ifndef BOTMASTER_H
#define	BOTMASTER_H

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

#include "CNCServer.h"
#include "InfectedHost.h"
using namespace std;

int GetDifferenceInMilliseconds(timeval &val1, timeval &val2);

class BotMaster 
{
private:
    char temp[1024];
    string bot_master_configuration_file;
    int cnc_port, number_of_commands;
    string authentication_key;
    string botmaster_output_file;
    int myListeningPort;
    ofstream outputHandler;
    int serverSocket;
    timeval base_signal;
    int god_pid;
    bool LoadConfigurationFile();
    bool OpenOutputFileHandler();
    void LogMessage(char * message);
    void ReceiveListOfHosts(int sockfd);
    bool EstablishConnectionWithCNCServer();
    bool AuthenticateWithCNCServer();
    struct MasterCommand {
        string command_text;
        int time_stamp;
    };
    MasterCommand *commands;
public:
    void setGodPid(int pid)
    {
        god_pid = pid;
    }
    void setConfigurationFileName(string conf_file_name)
    {
        bot_master_configuration_file.assign(conf_file_name);
    }
    void Start()
    {
        //Load configurations
        if (!LoadConfigurationFile())
        {
            perror("Bot Master configurations file not found");
            return;
        }
        if (!OpenOutputFileHandler()){
            perror("Bot master log file cannot be created");
        }

        //sorting the commands in ascending time order
        for (int i=0; i<number_of_commands; i++)
        {
            for (int j=i+1; j<number_of_commands; j++)
            {
                if (commands[i].time_stamp > commands[j].time_stamp){
                    MasterCommand tempCommand = commands[i];
                    commands[i] = commands[j];
                    commands[j] = tempCommand;
                }
            }
        }
        if (!EstablishConnectionWithCNCServer())
        {
            cout<<"Could not open socket for C&C server: master closing"<<endl;
            return;
        }

        memset(temp, 0, sizeof(temp));
        sprintf(temp, "cnc_connected: port_%d", cnc_port);
        LogMessage(temp);
        
        if (!AuthenticateWithCNCServer())
        {
            cout<<"could not authenticate bot master"<<endl;
            return;
        }

        //now sending commands to c&c server
        timeval timestamp;
        gettimeofday(&timestamp, 0);
        for (int i=0; i<number_of_commands; i++)
        {
            int timeSpent = GetDifferenceInMilliseconds(timestamp, base_signal);
            int sleeptime = commands[i].time_stamp - timeSpent;
            if (sleeptime<0){
                sleeptime = 0;
            }
            usleep(sleeptime*1000);

            cout<<"master: executing command number "<<1+i<<endl;
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "command: %s\ncommand_result", commands[i].command_text.c_str());
            LogMessage(temp);
            send(serverSocket, (void*)commands[i].command_text.c_str(), strlen(commands[i].command_text.c_str()), 0);
            if (strncmp(commands[i].command_text.c_str(), "<get_list_of_connected_hosts>", 28)==0)
            {
                ReceiveListOfHosts(serverSocket);
            }
            else if (strncmp(commands[i].command_text.c_str(), "<Message>", 9)==0)
            {
                memset(temp, 0, sizeof(temp));
                recv(serverSocket, (void*)temp, sizeof(temp), 0);
                LogMessage(temp);
            }
            else if (strncmp(commands[i].command_text.c_str(), "<Email>", 7)==0)
            {
                memset(temp, 0, sizeof(temp));
                recv(serverSocket, (void*)temp, sizeof(temp), 0);
                LogMessage(temp);
            }
            gettimeofday(&timestamp, 0);
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "time_stamp(%s)\n-----", gettimeinbuffer(timestamp).c_str());
            LogMessage(temp);
        }
        outputHandler.close();
        sleep(5);
        cout<<"master: Sending god process a signal to terminate everything"<<endl;
        kill(god_pid, SIGUSR1);
    }
};

#endif	/* BOTMASTER_H */


