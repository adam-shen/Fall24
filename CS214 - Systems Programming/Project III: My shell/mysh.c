#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h> 

#define MYSH_TOK_DELIM " \t\r\n"
#define MYSH_TOK_BUFFER_SIZE 64

int mysh_cd(char **args);
int mysh_help(char **args);
int mysh_exit(char **args);
int mysh_which(char **args);
int mysh_pwd();

int mysh_builtin_nums();
void handle_redirection(char **args);
void handle_pipeline(char **args);
char **expand_wildcards(char **args);

char *builtin_cmd[] = {
    "cd",
    "help",
    "exit",
    "which",
    "pwd"
};

int (*builtin_func[])(char **) = {
    &mysh_cd,
    &mysh_help,
    &mysh_exit,
    &mysh_which,
    &mysh_pwd
};

int mysh_cd(char **args) {
    if (args[1] == NULL) {
        perror("Mysh error at cd: missing argument");
    } else {
        // Join all arguments after 'cd' into a single path string
        char path[1024] = "";
        for (int i = 1; args[i] != NULL; i++) {
            strcat(path, args[i]);
            if (args[i + 1] != NULL) {
                strcat(path, " ");
            }
        }
        if (chdir(path) != 0) {
            perror("Mysh error at chdir");
        }
    }
    return 1;
}

int mysh_help(char **args) {
    puts("This is Mysh, a simple shell");
    puts("Built-in commands:");
    for (int i = 0; i < mysh_builtin_nums(); i++) {
        printf("  %s\n", builtin_cmd[i]);
    }
    return 1;
}

int mysh_exit(char **args) {
    return 0;
}

int mysh_which(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mysh: which: missing argument\n");
        return 1;
    }

     // Check if the command is a built-in command.
    for (int i = 0; i < mysh_builtin_nums(); i++) {
        if (strcmp(args[1], builtin_cmd[i]) == 0) {
            fprintf(stderr, "mysh: which: %s: there is a shell built-in command\n", args[1]);
            return 1;
        }
    }

    char *path[] = {"/usr/local/bin", "/usr/bin", "/bin"};
    for (int i = 0; i < 3; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path[i], args[1]);
        if (access(full_path, X_OK) == 0) {
            printf("%s\n", full_path);
            return 1;
        }
    }
    fprintf(stderr, "mysh: which: command not found\n");
    return 1;
}
int mysh_pwd(){
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        printf("%s\n", buffer);
    } else {
        perror("getcwd() error");
    }
    return 1;
}

int mysh_builtin_nums() {
    return sizeof(builtin_cmd) / sizeof(builtin_cmd[0]);
}

#define READ_BUFFER_SIZE 1024

char *mysh_read_line_read(FILE *input) {
    int fd = fileno(input);  // Get the file descriptor from FILE*
    if (fd == -1) {
        perror("mysh: invalid file descriptor");
        return NULL;
    }

    size_t buffer_size = READ_BUFFER_SIZE;
    size_t position = 0;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char c;
        ssize_t bytes_read = read(fd, &c, 1);
        if (bytes_read == -1) {
            perror("mysh: read error");
            free(buffer);
            return NULL;
        } else if (bytes_read == 0) {
            // EOF
            if (position == 0) {
                free(buffer);
                return NULL;
            }
            break;
        }

        // If we encounter a newline, stop reading
        if (c == '\n') {
            break;
        }

        buffer[position++] = c;

        // If buffer is full, resize it
        if (position >= buffer_size) {
            buffer_size += READ_BUFFER_SIZE;
            char *new_buffer = realloc(buffer, buffer_size);
            if (!new_buffer) {
                fprintf(stderr, "mysh: allocation error\n");
                free(buffer);
                exit(EXIT_FAILURE);
            }
            buffer = new_buffer;
        }
    }

    // Null-terminate the string
    buffer[position] = '\0';

    return buffer;
}


char **mysh_split_line(char *line) {
    int buffer_size = MYSH_TOK_BUFFER_SIZE, position = 0;
    char **tokens = malloc(buffer_size * sizeof(char *));
    char *token;

    if (!tokens) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, MYSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position++] = token;

        if (position >= buffer_size) {
            buffer_size += MYSH_TOK_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "mysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, MYSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void handle_redirection(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            int out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (out_fd == -1) {
                perror("mysh: cannot open output file");
                return;
            }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            int in_fd = open(args[i + 1], O_RDONLY);
            if (in_fd == -1) {
                perror("mysh: cannot open input file");
                return;
            }
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
            args[i] = NULL;
        }
        i++;
    }
}

void handle_pipeline(char **args) {
    int pipe_index = -1;

    // Find the pipe operator in the arguments
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index == -1) {
        // No pipeline, let the caller handle it
        return;
    }

    args[pipe_index] = NULL; // Split the arguments at the pipe

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("mysh: error creating pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // First child process
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
        close(pipefd[0]);              // Close unused read end of the pipe
        close(pipefd[1]);              // Close write end after redirection
        execvp(args[0], args);         // Execute the first command
        perror("mysh: error at execvp"); // Exec failed
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Second child process
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
        close(pipefd[1]);              // Close unused write end of the pipe
        close(pipefd[0]);              // Close read end after redirection
        execvp(args[pipe_index + 1], &args[pipe_index + 1]); // Execute the second command
        perror("mysh: error at execvp"); // Exec failed
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(pipefd[0]); // Close both ends of the pipe
    close(pipefd[1]);

    // Wait for both child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}



char **expand_wildcards(char **args) {
    char **new_args = malloc(MYSH_TOK_BUFFER_SIZE * sizeof(char *));
    if (!new_args) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    int position = 0;
    for (int i = 0; args[i] != NULL; i++) {
        if (strchr(args[i], '*') != NULL) {
            char *pattern = args[i];
            char *dir_path = ".";
            char *last_slash = strrchr(pattern, '/');

            if (last_slash) {
                *last_slash = '\0';
                dir_path = pattern;
                pattern = last_slash + 1;
            }

            DIR *dir = opendir(dir_path);
            if (!dir) {
                perror("mysh: cannot open directory");
                new_args[position++] = args[i];
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (fnmatch(pattern, entry->d_name, 0) == 0) {
                    char full_path[1024];
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                    new_args[position++] = strdup(full_path);
                }
            }

            closedir(dir);
            if (position == 0) {
                new_args[position++] = args[i];
            }
        } else {
            new_args[position++] = args[i];
        }

        if (position >= MYSH_TOK_BUFFER_SIZE) {
            new_args = realloc(new_args, (position + MYSH_TOK_BUFFER_SIZE) * sizeof(char *));
            if (!new_args) {
                fprintf(stderr, "mysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    new_args[position] = NULL;
    return new_args;
}

int mysh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    char *allowed_paths[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
    char full_path[1024];
    int found = 0;

    // Iterate through allowed directories to find the executable
    for (int i = 0; allowed_paths[i] != NULL; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", allowed_paths[i], args[0]);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "mysh: %s: command not found\n", args[0]);
        return 1;
    }

    pid = fork();
    if (pid == 0) {
        // Child process
        handle_redirection(args);
        if (execv(full_path, args) == -1) {
            perror("mysh: execv error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Fork failed
        perror("mysh: fork error");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}


int mysh_execute(char **args) {
    if (args[0] == NULL) return 1; // Empty command

    // Check if the command includes a pipeline
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            handle_pipeline(args);
            return 1; // Pipeline handled, no further execution needed
        }
    }

    // Check for built-in commands
    for (int i = 0; i < mysh_builtin_nums(); i++) {
        if (strcmp(args[0], builtin_cmd[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    // Execute other commands
    return mysh_launch(args);
}


void mysh_loop(FILE *input) {
    char *line;
    char **args;
    int status;

    if (isatty(fileno(input))) {
        printf("Welcome to my shell!\n");
    }

    do {
        
        printf("mysh> ");
        fflush(stdout);

        line = mysh_read_line_read(input);  // Use the new read-based function
        if (!line) break;
        args = mysh_split_line(line);
        
        if (!args) {
            free(line);
            continue;
        }
        args = expand_wildcards(args);
        if (!args) {
            free(line);
            continue;
        }
        status = mysh_execute(args);

        free(line);
        free(args);
        
    } while (status);

    if (isatty(fileno(input))) {
        printf("Exiting my shell.\n");
    }
}


int main(int argc, char *argv[]) {
    if (argc == 2) {
        int batch_fd = open(argv[1], O_RDONLY);
        if (batch_fd == -1) {
            perror("mysh: cannot open batch file");
            return EXIT_FAILURE;
        }
        FILE *batch_file = fdopen(batch_fd, "r");
        if (!batch_file) {
            perror("mysh: fdopen failed");
            close(batch_fd); 
            return EXIT_FAILURE;
        }
        mysh_loop(batch_file);
        fclose(batch_file);
    } else {
        mysh_loop(stdin);
    }
    return EXIT_SUCCESS;
}