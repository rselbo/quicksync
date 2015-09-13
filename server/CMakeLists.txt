cmake_minimum_required (VERSION 2.6)

project(SyncServer)

set (SOURCES 
		syncserver.cpp
	)
	
add_executable(SyncServer $SOURCES)