#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

// Function prototypes
void process_file(const char *file_path);
void process_directory(const char *path);
//void add_word(const char word, WordCountword_counts, int size);
int is_valid_character(char c);
void extract_words(const chartext, WordCount word_counts, int size);
void print_sorted_word_counts(WordCount *word_counts, int size);

typedef struct {
    char word[50]; // Adjust size based on max word length
    int count;
} WordCount;

WordCount words[1000]; // Array of WordCount structs



void process_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char buffer[BUFFER_SIZE];
    while (fscanf(file, "%1023s", buffer) == 1) {
        add_word(buffer);
    }

    fclose(file);
}

void process_directory(const char *path) {
    
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
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); // concatenates the directory path and the specific file or directory name to form the full path.
        
        // Use stat() to check if the entry is a directory
        struct stat entry_stat;
        if (stat(full_path, &entry_stat) == -1) {
            perror("stat");
            continue; // Skip to the next entry (so if one file fails, it doesn't stop the whole program)
        }

        // Check if the entry is a directory
        if (S_ISDIR(entry_stat.st_mode)) {
            // Recursive call to process the subdirectory
            process_directory(full_path);
        } else {

            // Check if the file has a ".txt" extension
            if (strstr(entry->d_name, ".txt") != NULL) {
                // Open & Process only .txt files
                process_file(full_path);
            }
        }
    }

    closedir(dir);
}

void add_word(const char *word) {
    // Check if the word is already in the array
    for (int i = 0; i < 1000; i++) {
        if (strcmp(words[i].word, word) == 0) { // If the word is already in the array, increment the count and return
            words[i].count++;
            return;
        }
    }

    // Add the word to the array
    for (int i = 0; i < 1000; i++) {
        if (words[i].count == 0) { // If the count is 0, the word is not in the array, therefore we add it and set count to 1
            strcpy(words[i].word, word);
            words[i].count = 1;
            return;
        }
    }
}

int main(int argc, char *argv[]) {
    
    return 0;

}