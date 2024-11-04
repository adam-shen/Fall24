#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MAX_SIZE 10240  // Define a maximum size for the file_contents array

// Function prototypes
void process_file(const char *filename); //Done
void process_directory(const char *path); //Done
void add_word(const char *word); //Done
void extract_words(const char *chunk);
void print_sorted_word_counts(); //Done
int compare_counts(const void *a, const void *b); //Done
int is_valid_character(char c, int in_word); //Done

char file_contents[MAX_SIZE];  // Define the array to store the file contents

typedef struct {
    char word[50]; // Adjust size based on max word length
    int count;
} WordCount;

WordCount words[1000]; // Array of WordCount structs

int word_count = 0; // Number of words in the array

void print_file_contents() {
    printf("%s", file_contents);
}

void process_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror(filename);
        return;
    }

    char buffer[BUFFER_SIZE];  // Define the buffer array
    int total_bytes = 0;  // Keep track of the total bytes read
    int bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate to treat as a string
        if (total_bytes + bytes_read < MAX_SIZE) {
            strncpy(file_contents + total_bytes, buffer, bytes_read);  // Copy to file_contents array
            total_bytes += bytes_read;
            extract_words(buffer);  // Pass the chunk to extract_words
        } else {
            // Handle the case where the array is not large enough
            write(2, "Output array is too small\n", 26);
            break;
        }
    }

    if (bytes_read == -1) {
        write(2, "Error reading file\n", 19);
    }
    printf("\n");

    close(fd);
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
            word_count++;
            return;
        }
    }

    // Add the word to the array
    for (int i = 0; i < 1000; i++) {
        if (words[i].count == 0) { // If the count is 0, the word is not in the array, therefore we add it and set count to 1
            strcpy(words[i].word, word);
            words[i].count = 1;
            word_count++;
            return;
        }
    }
}

void print_sorted_word_counts() {
    qsort(words, word_count, sizeof(WordCount), compare_counts);
    for (int i = 0; i < word_count; i++) {
        if (words[i].count > 0) {
            printf("%s %d\n", words[i].word, words[i].count);
        }
    }
}

int compare_counts(const void *a, const void *b) {
   WordCount *wordA = (WordCount *)a;
   WordCount *wordB = (WordCount *)b;

    if (wordB->count == wordA->count) {
        return strcmp(wordA->word, wordB->word);
    }
    return wordB->count - wordA->count;
}
/*
void extract_words(const char *chunk) {
    const char delimiters[] = ",.!:;?\"0123456789"; // Expanded delimiters for more robustness
    char buffer[BUFFER_SIZE];
    int buffer_index = 0;
    int in_word = 0;

    for (int i = 0; chunk[i] != '\0'; i++) {
        // Allow letters and apostrophes anywhere in the word
        if (isalpha(chunk[i]) || chunk[i] == '\'') {
            buffer[buffer_index++] = chunk[i];
            in_word = 1;
        }
        // Allow hyphens between letters, treating it as part of the word
        else if (chunk[i] == '-' && isalpha(chunk[i-1]) && isalpha(chunk[i+1])) {
            buffer[buffer_index++] = chunk[i];
        }
        // Delimiters signify the end of a word
        else {
            if (in_word) {
                buffer[buffer_index] = '\0'; // Null-terminate the word
                add_word(buffer);           // Add the word to the count
                buffer_index = 0;           // Reset buffer for next word
                in_word = 0;
            }
        }
    }

    // Process the last word if the chunk ends with a valid character
    if (in_word) {
        buffer[buffer_index] = '\0';
        add_word(buffer);
    }
}
*/
void extract_words(const char *chunk) {
    char buffer[BUFFER_SIZE];
    int buffer_index = 0;
    int in_word = 0;

    for (int i = 0; chunk[i] != '\0'; i++) {
        // Allow letters, apostrophes and hyphens as part of a word
        if (isalpha(chunk[i]) || chunk[i] == '\'' || (chunk[i] == '-' && isalpha(chunk[i - 1]) && isalpha(chunk[i + 1]))) {
            buffer[buffer_index++] = chunk[i];
            in_word = 1;
        } 
        // A separator indicates the end of a word
        else {
            if (in_word) {
                buffer[buffer_index] = '\0';// Null-terminate the word
                add_word(buffer);           // Add the word to the count
                buffer_index = 0;           // Reset buffer for next word
                in_word = 0;
            }
        }
    }

    // Process the last word if the chunk ends with a valid character
    if (in_word) {
        buffer[buffer_index] = '\0';
        add_word(buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write(2, "Usage: ./words <file_or_directory>...\n", 38);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat path_stat;
        if (stat(argv[i], &path_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            // If the argument is a directory, process it recursively
            process_directory(argv[i]);
        } else if (S_ISREG(path_stat.st_mode)) {
            // If the argument is a regular file, process it directly
            int fd = open(argv[i], O_RDONLY);
            if (fd == -1) {
                perror("open");
                continue;
            }
            process_file(argv[i]);
            close(fd);
        } else {
            write(2, "Skipping unsupported path\n", 26);
        }
    }

    // Print the sorted word counts after processing all files and directories
    print_sorted_word_counts();

    return 0;
}
