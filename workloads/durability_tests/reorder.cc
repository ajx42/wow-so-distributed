#include <fcntl.h>
#include <unistd.h>
#include <string>

using namespace std;

//Cause a crash in FUSE if written
const std::string WOW_KILL_PHRASE = "__WOW_KILL__";

int main()
{
	//string FILE_PATH = "./reorder.txt";
	string FILE_PATH = "/tmp/wowfs/durability/reorder.txt";
	string MESSAGE_A = "Value A\n";
	string MESSAGE_B = "Value B\n";
	string MESSAGE_C = "COMMIT\n";

	int fd = open(FILE_PATH.c_str(), O_CREAT | O_WRONLY, 0777);

	if(fd < 0)
	{
		printf("Open failed : %d\n", errno);
	}

	ftruncate(fd, MESSAGE_A.size() + MESSAGE_B.size() + MESSAGE_C.size());
	fsync(fd);

	int offset = 0;

	pwrite(fd, MESSAGE_A.c_str(), MESSAGE_A.size(), offset);
	offset+=MESSAGE_A.size();

	pwrite(fd, MESSAGE_B.c_str(), MESSAGE_B.size(), offset);
	offset+=MESSAGE_B.size();

	pwrite(fd, MESSAGE_C.c_str(), MESSAGE_C.size(), offset);
	offset+=MESSAGE_C.size();

	pwrite(fd, WOW_KILL_PHRASE.c_str(), WOW_KILL_PHRASE.size(), offset);
	offset+=WOW_KILL_PHRASE.size();

	close(fd);

	return 0;
}