/*
	Name: Ahmad Wattad
	Project: ext2 parsing
*/
/**************************************************** libraries ***********************************************************/

#include <fcntl.h>	/*O_RDONLY*/
#include <stdio.h> /*printf fprintf*/
#include <stdlib.h> /*exit*/
#include <unistd.h> /*pread*/
#include <linux/types.h> /**us/*/
#include <unistd.h> /*pread*/
#include <sys/stat.h> /*ISDIR*/
#include <assert.h> /*assert*/
#include <string.h>	/*memcpy, strcat*/

#include "ext2_fs.h"
#include "utils.h"

/**************************************************** enums/macros *******************************************************/

int main(void)
{
	char *name = "/dev/ram1";
	char *path = "/dir_test/dir3/file3.txt";

	char path_name[100] = {0};
	int fd;
    
	memcpy(path_name, path, strlen(path));
	
	if ((fd = open(name, O_RDONLY)) < 0)
    {
        perror(name);
        exit(1);
    }

	PrintSuperBlock(name, fd);
	PrintGroupDes(name, fd);
	PrintRootFilesContent(name, fd);
	//FindFileByPath(path_name, name, fd);

	close(fd);

} 
