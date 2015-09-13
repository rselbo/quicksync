#ifndef SERVER_SYNCSERVER_H
#define SERVER_SYNCSERVER_H

class SyncServer
{
public:
	SyncServer(int argc, char* argv[]);

	int Execute();
};

#endif//SERVER_SYNCSERVER_H