import time, os

file = open("version.h", "w")
#file.write("#define SVNVERSION ")
#ver = os.popen("svnversion")
#file.write("svn")
#file.write(ver.readline())
#ver.close()
file.write("\n#define BUILDTIME \"" + time.ctime() +"\"\n")
file.close()
