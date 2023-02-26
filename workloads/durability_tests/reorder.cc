#include <fcntl.h>
#include <unistd.h>
#include <string>

using namespace std;

//Cause a crash in FUSE if written
const std::string WOW_KILL_PHRASE = "__WOW_KILL__";

string MESSAGE_ARRAY [] = 
{
	"Value A\n",
	"Value B\n",
	"Value C\n",
	"Value D\n",
	"Value E\n",
	"Value F\n",
	"Value G\n",
	"Commit\n",
	WOW_KILL_PHRASE,
};

int main()
{
	//File we are writing to
	string FILE_PATH = "/tmp/wowfs/durability/reorder.txt";

	int fd = open(FILE_PATH.c_str(), O_CREAT | O_WRONLY, 0777);

	if(fd < 0)
	{
		printf("Open failed : %d\n", errno);
	}

	//Resize file to fit all data	
	int sum = 0;
	for(auto m : MESSAGE_ARRAY)
	{
		sum+=m.size();
	}

	ftruncate(fd, sum);
	fsync(fd);

	//Position write data, WOW_KILL_PHRASE will cause a crash in FUSE
	int offset = 0;
	for(auto m : MESSAGE_ARRAY)
	{
		pwrite(fd, m.c_str(), m.size(), offset);
		offset+=m.size();
	}

	close(fd);

	return 0;
}