all:
	g++ cdcphase1.cpp InfectedHost.cpp CNCServer.cpp BotMaster.cpp GodProcess.cpp -lpthread -o botnet
clean:
	rm a.out
