#include "BotMaster.h"
#include <math.h>

int GetDifferenceInMilliseconds(timeval &a, timeval &b)
{
    timeval result;
    result.tv_sec = a.tv_sec - b.tv_sec;
    result.tv_usec = a.tv_usec - b.tv_usec;
    if (result.tv_usec < 0)
    {
        result.tv_sec = result.tv_sec - 1;
        result.tv_usec = result.tv_usec + 1000000;
    }
    int diff = (result.tv_sec*1000) + ceil(((double)result.tv_usec/1000));
    return diff;
}

bool BotMaster::LoadConfigurationFile()
{
    ifstream in;
    in.open(bot_master_configuration_file.c_str(), ios::in);
    if (!in.is_open()){
        perror("Cannot find master file");
        return false;
    }
    in.ignore(1024, ' ');
    in.getline(temp, sizeof(temp), '\n');
    cnc_port = atoi(temp);
    in.ignore(1024, ' ');
    in.getline(temp, sizeof(temp), '\n');
    authentication_key.assign(temp);
    in.ignore(1024, ' ');
    in.getline(temp, sizeof(temp), '\n');
    temp[strlen(temp)-1] = '\0';
    botmaster_output_file.assign(temp);
    
    in.ignore(1024, ' ');
    in.getline(temp, sizeof(temp), '\n');
    number_of_commands = atoi(temp);
    //cout<<"Number of commands : "<<number_of_commands<<endl;
    commands = new MasterCommand[number_of_commands];
    for (int i=0; i<number_of_commands; i++)
    {
        in.getline(temp, sizeof(temp), '*');
        commands[i].command_text.assign(temp);
        
        in.getline(temp, sizeof(temp), '*');
        commands[i].time_stamp = atoi(temp);
        
        in.ignore(1024, '\n');
    }
    in.close();
    return true;
}

bool BotMaster::OpenOutputFileHandler()
{
    outputHandler.open(botmaster_output_file.c_str(), ios::out);
    if (!outputHandler.is_open()){
        perror("master output file cannot be created");
        return false;
    }
    return true;
}

void BotMaster::LogMessage(char *message)
{
    outputHandler<<message<<endl;
}

void BotMaster::ReceiveListOfHosts(int sockfd)
{
    int count = 0;
    char temp[1024];
    memset(temp, 0, sizeof(temp));
    recv(sockfd, (void*)temp, sizeof(temp), 0);
    while (!strncmp(temp, "<EndOfHosts>", 12)==0)
    {
        LogMessage(temp);
        strcpy(temp, "ok");
        send(sockfd, (void*)temp, strlen(temp), 0);
        count++;
        memset(temp, 0, sizeof(temp));
        recv(sockfd, (void*)temp, sizeof(temp), 0);
    }
}

bool BotMaster::EstablishConnectionWithCNCServer()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket<0)
    {
        perror("Could Not Create Socket");
        return false;
    }
    sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv.sin_port = htons(cnc_port);
    int newfd = -1;
    while (newfd<0)
    {
        newfd = connect(serverSocket, (sockaddr*)&srv, sizeof(srv));
    }
    return true;
}

bool BotMaster::AuthenticateWithCNCServer()
{
    int mypid = getpid();
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "name:master pid:%d port:%d authentication_key:%s \0", mypid, myListeningPort, authentication_key.c_str());
    send(serverSocket, (void*)temp, strlen(temp), 0);
    memset(temp, 0, sizeof(temp));
    recv(serverSocket, (void*)temp, sizeof(temp), 0);
    cout<<"master authentication status message: "<<temp<<endl;
    if (strncmp(temp, "authenticated", 13)==0){
        cout<<"executing authenticated routine"<<endl;
        string time;
        time.assign(temp);
        time = time.substr(time.find(" ")+1, time.length());
        base_signal.tv_sec = atoi(time.substr(0, time.find(" ")).c_str());
        time = time.substr(time.find(" ")+1, time.length());
        base_signal.tv_usec = atoi(time.c_str());
        
        strcpy(temp, "master_authentication_status: authenticated\n-----");
        LogMessage(temp);
    }
    else
    {
        cout<<"executing authentication failed routine"<<endl;
        strcpy(temp, "master_authentication_status: authentication_failed\n-----");
        LogMessage(temp);
        return false;
    }
    return true;
}
