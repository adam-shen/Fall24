// mysh.c
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#define MAX_TOKENS 128
#define BUFFER_SIZE 1024

// Directories to search for executables
const char *path_dirs[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};

// Function declarations
void print_prompt();
void execute_command(char *command_line, int interactive);
void parse_command(char *command_line, char ***argv1, char ***argv2, char **infile, char **outfile, int *is_pipe);
int is_builtin(char *cmd);
void run_builtin(char **args);
char *find_executable(char *cmd);
void expand_wildcards(char *token, char ***args, int *argc);
void handle_wildcard(char *pattern, char ***args, int *argc);

int main(int argc, char *argv[]) {
    int interactive = isatty(STDIN_FILENO);
    FILE *input = stdin;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int read_offset = 0;

    // Check for too many arguments
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [scriptfile]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Open script file if provided
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    if (interactive) {
        printf("Welcome to my shell!\n");
        print_prompt();
        fflush(stdout);
    }

    while (1) {
        char command_line[BUFFER_SIZE];
        int cmd_len = 0;

        // Read input one character at a time using read()
        while (1) {
            bytes_read = read(fileno(input), buffer + read_offset, 1);
            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            } else if (bytes_read == 0) {
                // EOF
                if (cmd_len > 0) {
                    buffer[read_offset] = '\0';
                    strcpy(command_line, buffer);
                    execute_command(command_line, interactive);
                }
                goto exit_shell;
            } else {
                if (buffer[read_offset] == '\n') {
                    buffer[read_offset] = '\0';
                    strcpy(command_line, buffer);
                    read_offset = 0;
                    execute_command(command_line, interactive);
                    break;
                } else {
                    read_offset++;
                    cmd_len++;
                    if (cmd_len >= BUFFER_SIZE - 1) {
                        fprintf(stderr, "Command too long\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        if (interactive) {
            print_prompt();
            fflush(stdout);
        }
    }

exit_shell:
    if (interactive) {
        printf("Exiting my shell.\n");
    }

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}

void print_prompt() {
    printf("mysh> ");
}

void execute_command(char *command_line, int interactive) {
    char **argv1 = NULL;
    char **argv2 = NULL;
    char *infile = NULL;
    char *outfile = NULL;
    int is_pipe = 0;
    int status;
    pid_t pid1, pid2;

    parse_command(command_line, &argv1, &argv2, &infile, &outfile, &is_pipe);

    if (argv1 == NULL) {
        return;
    }

    if (is_builtin(argv1[0])) {
        run_builtin(argv1);
    } else {
        if (is_pipe) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                goto cleanup;
            }

            pid1 = fork();
            if (pid1 == -1) {
                perror("fork");
                goto cleanup;
            } else if (pid1 == 0) {
                // First child (left side of the pipe)
                if (infile) {
                    int fd_in = open(infile, O_RDONLY);
                    if (fd_in == -1) {
                        perror("open");
                        _exit(EXIT_FAILURE);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);

                char *exec_path = find_executable(argv1[0]);
                if (exec_path) {
                    execv(exec_path, argv1);
                    perror("execv");
                    free(exec_path);
                } else {
                    fprintf(stderr, "%s: command not found\n", argv1[0]);
                }
                _exit(EXIT_FAILURE);
            }

            pid2 = fork();
            if (pid2 == -1) {
                perror("fork");
                goto cleanup;
            } else if (pid2 == 0) {
                // Second child (right side of the pipe)
                if (outfile) {
                    int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    if (fd_out == -1) {
                        perror("open");
                        _exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);

                char *exec_path = find_executable(argv2[0]);
                if (exec_path) {
                    execv(exec_path, argv2);
                    perror("execv");
                    free(exec_path);
                } else {
                    fprintf(stderr, "%s: command not found\n", argv2[0]);
                }
                _exit(EXIT_FAILURE);
            }

            // Parent process
            close(pipefd[0]);
            close(pipefd[1]);

            int status1, status2;
            waitpid(pid1, &status1, 0);
            waitpid(pid2, &status2, 0);

            // Check exit status of the last command
            if (interactive) {
                if (WIFEXITED(status2)) {
                    int exit_code = WEXITSTATUS(status2);
                    if (exit_code != 0) {
                        printf("mysh: Command failed with code %d\n", exit_code);
                    }
                } else if (WIFSIGNALED(status2)) {
                    int term_sig = WTERMSIG(status2);
                    printf("mysh: Terminated by signal: %s\n", strsignal(term_sig));
                }
            }

        } else {
            // Single command execution
            pid1 = fork();
            if (pid1 == -1) {
                perror("fork");
                goto cleanup;
            } else if (pid1 == 0) {
                // Child process
                if (infile) {
                    int fd_in = open(infile, O_RDONLY);
                    if (fd_in == -1) {
                        perror("open");
                        _exit(EXIT_FAILURE);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }
                if (outfile) {
                    int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    if (fd_out == -1) {
                        perror("open");
                        _exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                char *exec_path = find_executable(argv1[0]);
                if (exec_path) {
                    execv(exec_path, argv1);
                    perror("execv");
                    free(exec_path);
                } else {
                    fprintf(stderr, "%s: command not found\n", argv1[0]);
                }
                _exit(EXIT_FAILURE);
            }

            // Parent process
            waitpid(pid1, &status, 0);

            if (interactive) {
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    if (exit_code != 0) {
                        printf("mysh: Command failed with code %d\n", exit_code);
                    }
                } else if (WIFSIGNALED(status)) {
                    int term_sig = WTERMSIG(status);
                    printf("mysh: Terminated by signal: %s\n", strsignal(term_sig));
                }
            }
        }
    }

cleanup:
    // Free allocated memory
    if (argv1) {
        for (int i = 0; argv1[i]; i++) {
            free(argv1[i]);
        }
        free(argv1);
    }
    if (argv2) {
        for (int i = 0; argv2[i]; i++) {
            free(argv2[i]);
        }
        free(argv2);
    }
    if (infile) {
        free(infile);
    }
    if (outfile) {
        free(outfile);
    }
}

void parse_command(char *command_line, char ***argv1, char ***argv2, char **infile, char **outfile, int *is_pipe) {
    char *tokens[MAX_TOKENS];
    int ntokens = 0;
    char *token = strtok(command_line, " \t");

    while (token != NULL && ntokens < MAX_TOKENS - 1) {
        tokens[ntokens++] = token;
        token = strtok(NULL, " \t");
    }
    tokens[ntokens] = NULL;

    int i = 0;
    int argc1 = 0, argc2 = 0;
    char **args1 = malloc(sizeof(char *) * MAX_TOKENS);
    char **args2 = malloc(sizeof(char *) * MAX_TOKENS);
    int parsing_cmd1 = 1;

    *infile = NULL;
    *outfile = NULL;
    *is_pipe = 0;

    while (i < ntokens) {
        if (strcmp(tokens[i], "|") == 0) {
            *is_pipe = 1;
            parsing_cmd1 = 0;
            i++;
            continue;
        } else if (strcmp(tokens[i], "<") == 0) {
            i++;
            if (i < ntokens) {
                *infile = strdup(tokens[i++]);
            } else {
                fprintf(stderr, "mysh: Missing input file\n");
                free(args1);
                free(args2);
                *argv1 = NULL;
                return;
            }
        } else if (strcmp(tokens[i], ">") == 0) {
            i++;
            if (i < ntokens) {
                *outfile = strdup(tokens[i++]);
            } else {
                fprintf(stderr, "mysh: Missing output file\n");
                free(args1);
                free(args2);
                *argv1 = NULL;
                return;
            }
        } else {
            if (parsing_cmd1) {
                expand_wildcards(tokens[i], &args1, &argc1);
            } else {
                expand_wildcards(tokens[i], &args2, &argc2);
            }
            i++;
        }
    }

    args1[argc1] = NULL;
    args2[argc2] = NULL;

    if (argc1 == 0) {
        free(args1);
        free(args2);
        *argv1 = NULL;
        return;
    }

    *argv1 = args1;
    if (*is_pipe) {
        if (argc2 == 0) {
            fprintf(stderr, "mysh: Missing command after pipe\n");
            free(args1);
            free(args2);
            *argv1 = NULL;
            return;
        }
        *argv2 = args2;
    } else {
        free(args2);
        *argv2 = NULL;
    }
}

int is_builtin(char *cmd) {
    return (strcmp(cmd, "cd") == 0 ||
            strcmp(cmd, "pwd") == 0 ||
            strcmp(cmd, "which") == 0 ||
            strcmp(cmd, "exit") == 0);
}

void run_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] && !args[2]) {
            if (chdir(args[1]) == -1) {
                perror("cd");
            }
        } else {
            fprintf(stderr, "cd: Wrong number of arguments\n");
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd");
        }
    } else if (strcmp(args[0], "which") == 0) {
        if (args[1] && !args[2]) {
            if (is_builtin(args[1])) {
                // Do not print anything
            } else {
                char *exec_path = find_executable(args[1]);
                if (exec_path) {
                    printf("%s\n", exec_path);
                    free(exec_path);
                }
            }
        } else {
            // Do not print anything
        }
    } else if (strcmp(args[0], "exit") == 0) {
        int i = 1;
        while (args[i]) {
            printf("%s ", args[i]);
            i++;
        }
        if (i > 1) {
            printf("\n");
        }
        exit(EXIT_SUCCESS);
    }
}

char *find_executable(char *cmd) {
    if (strchr(cmd, '/')) {
        // Pathname
        if (access(cmd, X_OK) == 0) {
            return strdup(cmd);
        } else {
            return NULL;
        }
    } else {
        // Search in predefined directories
        for (int i = 0; path_dirs[i]; i++) {
            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path_dirs[i], cmd);
            if (access(fullpath, X_OK) == 0) {
                return strdup(fullpath);
            }
        }
    }
    return NULL;
}

void expand_wildcards(char *token, char ***args, int *argc) {
    if (strchr(token, '*')) {
        handle_wildcard(token, args, argc);
    } else {
        (*args)[(*argc)++] = strdup(token);
    }
}

void handle_wildcard(char *pattern, char ***args, int *argc) {
    char dir_path[PATH_MAX];
    char *slash = strrchr(pattern, '/');
    char *filename_pattern;
    if (slash) {
        // There is a directory path in pattern
        size_t dir_len = slash - pattern;
        strncpy(dir_path, pattern, dir_len);
        dir_path[dir_len] = '\0';
        filename_pattern = slash + 1;
    } else {
        // No directory path
        strcpy(dir_path, ".");
        filename_pattern = pattern;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        // No matches; add the pattern as-is
        (*args)[(*argc)++] = strdup(pattern);
        return;
    }

    struct dirent *entry;
    int matched = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.' && filename_pattern[0] != '.') {
            continue; // Skip hidden files unless pattern starts with '.'
        }

        // Simple pattern matching
        char *asterisk = strchr(filename_pattern, '*');
        int len_before = asterisk - filename_pattern;
        int len_after = strlen(filename_pattern) - len_before - 1;

        if (strncmp(entry->d_name, filename_pattern, len_before) == 0 &&
            strcmp(entry->d_name + strlen(entry->d_name) - len_after, asterisk + 1) == 0) {
            char fullpath[PATH_MAX];
            if (strcmp(dir_path, ".") == 0) {
                snprintf(fullpath, PATH_MAX, "%s", entry->d_name);
            } else {
                snprintf(fullpath, PATH_MAX, "%s/%s", dir_path, entry->d_name);
            }
            (*args)[(*argc)++] = strdup(fullpath);
            matched = 1;
        }
    }
    closedir(dir);
    if (!matched) {
        (*args)[(*argc)++] = strdup(pattern);
    }
}
