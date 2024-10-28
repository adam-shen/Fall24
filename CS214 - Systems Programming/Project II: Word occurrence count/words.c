#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      // for read, write, close
#include <fcntl.h>       // for open
#include <dirent.h>      // for opendir, closedir, readdir
#include <sys/stat.h>    // for stat
#include <sys/types.h>   // for types used in stat

void list_directory(const char *path) {
    DIR *dir = opendir(path);

    if (dir == NULL) {
        write(2, "Error opening directory\n", 25);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries to avoid infinite recursion
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path to the entry
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // Use stat() to check if the entry is a directory
        struct stat entry_stat;
        if (stat(full_path, &entry_stat) == -1) {
            perror("stat");
            continue;
        }

        // Check if the entry is a directory
        if (S_ISDIR(entry_stat.st_mode)) {
            // Recursive call to process the subdirectory
            list_directory(full_path);
        } else {
            // Check if the file has a ".txt" extension
            if (strstr(entry->d_name, ".txt") != NULL) {
                // Print only .txt files
                printf("Text File: %s\n", full_path);
            }
        }
    }

    closedir(dir);
}

int main() {

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
    printf("\n");

    close(fd);


    // Start by listing the contents of the "sample" directory
    //list_directory("sample");  // Replace "sample" with your starting directory
    return 0;
}
