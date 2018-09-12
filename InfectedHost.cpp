#include "InfectedHost.h"


bool InfectedHost::LoadConfigurations()
{
    ifstream in;
    in.open(infected_host_configuration_file.c_str(), ios::in);
    if (!in.is_open()){
        return false;
    }
    in.ignore(1024, ' ');
    in.getline(temp, sizeof(temp), '\n');
    cnc_port = atoi(temp);
    in.ignore(1024, ' ');
    in.getline(temp, sizeof(temp), '\n');
    temp[strlen(temp)-1] = '\0';
    host_output_filename.assign(temp);
    in.close();
    return true;
}

void InfectedHost::EstablishConnectionWithCNCServer()
{
    cncSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (cncSocket<0)
    {
        perror("Could Not Create Socket");
        return;
    }
    sockaddr_in srv;
    
    int newfd = -1;
    while (newfd<0) //this loop continues until C&C server is up and connected
    {
        srv.sin_family = AF_INET;
        srv.sin_addr.s_addr = inet_addr("127.0.0.1");
        srv.sin_port = htons(cnc_port);
        newfd = connect(cncSocket, (sockaddr*)&srv, sizeof(srv));
    }
}

void InfectedHost::RegisterWithCNCServer()
{        //sending details to C&C server 
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "name:%s pid:%d port:%d\0", myDetails.processName.c_str(), myDetails.processID, myDetails.portNumber);
    send(cncSocket, (void*)temp, strlen(temp), 0);
}

void *HostSendMessageThread(void *args)
{
    SendMessageArgs * messageArgs = (SendMessageArgs *)args;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd<0){
        perror("sending socket");
    }
    sockaddr_in srv;
    srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv.sin_port = htons(messageArgs->targetDetails->portNumber);
    srv.sin_family = AF_INET;
    socklen_t len = sizeof(srv);
    int newfd = connect(sockfd, (sockaddr*)&srv, len);
    if (newfd < 0)
    {
        perror("sending connect");
    }
    send(sockfd, messageArgs->message.c_str(), strlen(messageArgs->message.c_str()), 0);
    close(sockfd);
}

bool InfectedHost::OpenOutputFileHandler()
{
    myDetails.file_writer.open(infected_host_configuration_file.c_str(), ios::out|ios::app);
    return myDetails.file_writer.is_open();
}

void *host_listening_thread(void * args)
{
    ProcessDetails *myDetails = (ProcessDetails*)args;
    fd_set master;
    fd_set read_fds;
    sockaddr_in cli;
    socklen_t len = sizeof(cli);
    int fdmax = myDetails->portAssociated;
    FD_SET(myDetails->portAssociated, &master);
    
    while (true)
    {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
        }
        for (int i=0; i<=fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i==myDetails->portAssociated)
                {
                    len = sizeof(cli);
                    int newfd = accept(i, (sockaddr*)&cli, &len);
                    if (newfd<0){
                        perror("accept");
                    }
                    else{
                        FD_SET(newfd, &master);
                        if (newfd>fdmax)
                        {
                            fdmax = newfd;
                        }
                    }
                }
                else
                {
                    char temp[1024];
                    memset(temp, 0, sizeof(temp));
                    int nbytes = recv(i, (void*)temp, sizeof(temp), 0);
                    if (nbytes<=0)
                    {
                        close(i);
                        FD_CLR(i, &master);
                    }
                    else
                    {
                        //message is received
                        timeval messageTime;
                        gettimeofday(&messageTime, 0);
                        string message;
                        //cout<<"MESSAGE RECEIVED: "<<temp<<endl;
                        message.assign(temp);
                        string sender;
                        sender.assign(message.substr(0, message.find(":")));
                        message = message.substr(message.find(":")+2, message.length());
                        myDetails->file_writer<<"Message received from "<<sender<<" *"<<message<<"* "<<gettimeinbuffer(messageTime)<<endl;
                    }
                }
            }
        }
    }
}


int InfectedHost::CreateListeningSocket()
{
    int sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd2<0)
    {
        perror("CouldNot Create Socket2");
        return 0;
    }
    
    sockaddr_in my_listener;
    srand(time(NULL));
    int myListeningPort;
    int res=-1;
    while (res < 0)
    {
        my_listener.sin_family = AF_INET;
        my_listener.sin_addr.s_addr = inet_addr("127.0.0.1");
        myListeningPort = 1024+rand()%6450;
        my_listener.sin_port = htons(myListeningPort);
        res = bind(sockfd2, (sockaddr*)&my_listener, sizeof(my_listener));
    }
    res = listen(sockfd2, 10);
    if (res<0){
        perror("listen");
        return 0;
    }
    myDetails.portAssociated = sockfd2;
    myDetails.file_writer.open(host_output_filename.c_str(), ios::out|ios::app);
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, &host_listening_thread, &myDetails);
    return myListeningPort;
}

void InfectedHost::LogMessage(char* message)
{
    myDetails.file_writer<<message<<endl;
}

void InfectedHost::ExecuteMessage(char* command)
{
    //parsing message command
    string message;
    message.assign(command);
    message = message.substr(message.find("(")+1, message.length());
    ProcessDetails *receiver = new ProcessDetails();
    receiver->processName = message.substr(0, message.find(")"));
    message = message.substr(message.find("(")+1, message.length());
    receiver->portNumber= atoi(message.substr(0, message.find(")")).c_str());
    
    string body = message.substr(message.find(" "), message.length());
    sprintf(temp, "%s:%s\0", myDetails.processName.c_str(), body.c_str());
    
    //creating message sending thread
    SendMessageArgs * messageArgs = new SendMessageArgs();
    messageArgs->message.assign(temp);
    messageArgs->targetDetails = receiver;
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, HostSendMessageThread, messageArgs);
    
    timeval signalTime;
    gettimeofday(&signalTime, 0);
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%s: Message sent to %s time_stamp:%s", myDetails.processName.c_str(), receiver->processName.c_str(), gettimeinbuffer(signalTime).c_str());
    LogMessage(temp);
    pthread_join(thread_id, NULL);
}

void InfectedHost::ExecuteEmail(char* command)
{
    //parsing email command
    string message;
    message.assign(temp);
    //"Email:"+email+" *"+subject+"*"+body
    message = message.substr(message.find(":")+1, message.length());
    string address, subject;
    address.assign(message.substr(0, message.find(" ")));
    message = message.substr(message.find("*")+1, message.length());
    subject.assign(message.substr(0, message.find("*")));
    message = message.substr(message.find("*")+1, message.length());
    timeval signalTime;
    gettimeofday(&signalTime, 0);
    
    //logging email command
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "%s: echo \"%s\" | mail -s \"%s\" %s %s", myDetails.processName.c_str(), message.c_str(), subject.c_str(), address.c_str(), gettimeinbuffer(signalTime).c_str());
    LogMessage(temp);
}
