#include "GodProcess.h"

bool GodProcess::LoadConfigurationFile()
{
    ifstream in;
    in.open(god_configuration_filename.c_str(), ios::in);
    if (!in.is_open())
    {
        return false;
    }

    while (!in.eof())
    {
        in.getline(temp, sizeof(temp), ':');
        //cout<<"Command : "<<temp<<endl;
        if (strcmp(temp, "god_output_file")==0)
        {
            in.ignore(1024, ' ');
            in.getline(temp, sizeof(temp), '\n');
            temp[strlen(temp)-1] = '\0';
            god_output_filename.assign(temp);
            //cout<<god_output_filename<<endl;
        }
        else if (strcmp(temp, "cnc_configuration_file")==0)
        {
            in.ignore(1024, ' ');
            in.getline(temp, sizeof(temp), '\n');
            temp[strlen(temp)-1] = '\0';
            cnc_configuration_file.assign(temp);
        }
        else if (strcmp(temp, "bot_master_configuration_file")==0)
        {
            in.ignore(1024, ' ');
            in.getline(temp, sizeof(temp), '\n');
            temp[strlen(temp)-1] = '\0';
            bot_master_configuration_file.assign(temp);
        }
        else if (strcmp(temp, "infected_host_configuration_file")==0)
        {
            in.ignore(1024, ' ');
            in.getline(temp, sizeof(temp), '\n');
            temp[strlen(temp)-1] = '\0';
            infected_host_configuration_file.assign(temp);
        }
        else if (strcmp(temp, "number_of_infected_hosts")==0)
        {
            in.ignore(1024, ' ');
            in.getline(temp, sizeof(temp), '\n');
            number_of_infected_hosts = atoi(temp);
        }
        else if (strcmp(temp, "kill_hosts")==0)
        {
            in.ignore(1024, ' ');
            in.getline(temp, sizeof(temp), ' ');
            number_of_kill_hosts = atoi(temp);
            hosts_to_kill = new HostKill[number_of_kill_hosts];
            for (int i=0; i<number_of_kill_hosts; i++)
            {

                in.getline(temp, sizeof(temp), ':');
                hosts_to_kill[i].hostID = atoi(temp);
                
                in.getline(temp, sizeof(temp), ' ');
                hosts_to_kill[i].time_to_kill = atoi(temp);
            }
        }
    }
    in.close();
    return true;
}

void GodProcess::CreateProcesses()
{
    god_pid = getpid();
    myname.assign("god");
    int forkValue = fork();
    if (forkValue==0){
        myname.assign("cnc");
    }
    else
    {
        //cout<<"cnc_server_pid: "<<forkValue<<endl;
        cncServer.processName.assign("cnc");
        cncServer.processID = forkValue;
    }
    
    if (myname.compare("god")==0)
    {
        forkValue = fork();
        if (forkValue==0){
            myname.assign("master");
        }
        else
        {
            botMaster.processName.assign("master");
            botMaster.processID = forkValue;
        }
    }
    
    for (int i=0; i<number_of_infected_hosts; i++)
    {
        if (myname.compare("god")==0)
        {
            forkValue = fork();
            if (forkValue==0)
            {
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "h%d\0", i+1);
                myname.assign(temp);
            }
            else
            {
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "h%d\0", i+1);
                infectedHosts[i].processName.assign(temp);
                infectedHosts[i].processID = forkValue;
            }
        }
    }
}

bool GodProcess::OpenOutputFileHandler()
{
    outputHandler.open(god_output_filename.c_str(), ios::out);
    if (!outputHandler.is_open()){
        return false;
    }
    return true;
}

void GodProcess::LogMessage(char* message)
{
    outputHandler<<message<<endl;
}

void GodProcess::Start()
{
    timeval god_start_time;
    gettimeofday(&god_start_time, 0);
    
    if (!LoadConfigurationFile()){
        perror("God configuration file not found");
        return;
    }
    
    if (!OpenOutputFileHandler()){
        perror("Cannot create god output file");
    }
    
    infectedHosts = new ProcessDetails[number_of_infected_hosts];
    
    CreateProcesses();
    
    if (myname.compare("cnc")==0)
    {
        CNCServer cncserver;
        cncserver.set_cnc_configuration_file(cnc_configuration_file);
        cncserver.Start();
    }
    else if (myname.compare("god")==0)
    {
        //god process code goes here
        //logging god process start time 
        string starttime = gettimeinbuffer(god_start_time);
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "god_start_time_stamp: %s", starttime.c_str());
        LogMessage(temp);
        
        //sorting kill hosts
        for (int i=0; i<number_of_kill_hosts; i++)
        {
            for (int j=i; j<number_of_kill_hosts; j++)
            {
                if (hosts_to_kill[i].time_to_kill > hosts_to_kill[j].time_to_kill)
                {
                    HostKill tempHost = hosts_to_kill[i];
                    hosts_to_kill[i] = hosts_to_kill[j];
                    hosts_to_kill[j] = tempHost;
                }
            }
        }
        //subtracting elapsed times from kill times
        for (int i=number_of_kill_hosts - 1; i>0; i--)
        {
            hosts_to_kill[i].time_to_kill = hosts_to_kill[i].time_to_kill - hosts_to_kill[i-1].time_to_kill;
        }
        
        //logging pids
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "cnc_server_pid: %d", cncServer.processID);
        LogMessage(temp);
        sprintf(temp, "bot_master_pid: %d", botMaster.processID);
        LogMessage(temp);
        for (int i=0; i<number_of_infected_hosts; i++)
        {
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "host%d_pid: %d", i+1, infectedHosts[i].processID);
            LogMessage(temp);
        }
        //sending base signal
        sleep(2);
        
        timeval signalTime;
        gettimeofday(&signalTime, 0);
        usleep(signalTime.tv_usec/1000);
        gettimeofday(&signalTime, 0);
        cout<<"Base signal time on god process : "<<gettimeinbuffer(signalTime)<<endl;
        kill(cncServer.processID, SIGUSR1);
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "base_signal: %s", gettimeinbuffer(signalTime).c_str());
        LogMessage(temp);
        //now killing hosts at their kill times.
        for (int i=0; i<number_of_kill_hosts; i++)
        {
            usleep(hosts_to_kill[i].time_to_kill*1000);
            gettimeofday(&signalTime, 0);
            kill(infectedHosts[hosts_to_kill[i].hostID-1].processID, SIGINT);
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "killed host%d: time_stamp(%s)", hosts_to_kill[i].hostID, gettimeinbuffer(signalTime).c_str());
            LogMessage(temp);
        }
        
        outputHandler.close();

        signal(SIGUSR1, SIGINT_handler);
        sleep(60);
        cout<<"god: signal received from bot master to terminate the simulation"<<endl;
        for (int i=0; i<number_of_infected_hosts; i++)
        {
            kill(infectedHosts[i].processID, SIGKILL);
        }
        kill(cncServer.processID, SIGKILL);
        kill(botMaster.processID, SIGKILL);
    }
    else if (myname.compare("master")==0)
    {
        BotMaster master;
        master.setConfigurationFileName(bot_master_configuration_file);
        master.setGodPid(god_pid);
        master.Start();
    }
    else if (strncmp(myname.c_str(), "h", 1)==0)
    {
        InfectedHost host;
        host.SetConfigurationFileName(infected_host_configuration_file);
        host.SetHostName(myname);
        host.Start();
    }

}
