#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // for read, write, close
#include <fcntl.h>       // for open
#include <dirent.h>      // for opendir, closedir, readdir
#include <sys/stat.h>    // for stat
//#include <sys/types.h>   // for types used in stat

int main(){

    int fd = open("foo.txt", O_RDONLY);
    
    if (fd == -1){
        write(2, "Error opening file\n", 19);
        return 1;
    }

    char buffer[100];

    int bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate to treat as a string
        write(1, buffer, bytes_read);  // Print to standard output
    }

    if (bytes_read == -1) {
        write(2, "Error reading file\n", 19);
    }

    close(fd);


    return 0;
}