#include "syncserver.h"

int main(int argc, char* argv[])
{
	SyncServer server(argc, argv);
	return server.Execute();
}

SyncServer::SyncServer(int argc, char* argv[])
{

}

SyncServer::Execute()
{
	return 0;
}