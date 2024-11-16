#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define MYSH_TOK_DELIM " \t\r\n"
#define MYSH_TOK_BUFFER_SIZE 64

int mysh_cd(char **args);
int mysh_help(char **args);
int mysh_exit(char **args);

int mysh_builtin_nums();
void handle_redirection(char **args);

char *builtin_cmd[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[])(char **) = {
    &mysh_cd,
    &mysh_help,
    &mysh_exit
};

int mysh_cd(char **args) {
    if (args[1] == NULL) {
        perror("Mysh error at cd: missing argument");
    } else {
        if (chdir(args[1]) != 0) {
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

int mysh_builtin_nums() {
    return sizeof(builtin_cmd) / sizeof(builtin_cmd[0]);
}

char *mysh_read_line() {
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
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
            args[i] = NULL;  // Remove redirection token and file name
        } else if (strcmp(args[i], "<") == 0) {
            int in_fd = open(args[i + 1], O_RDONLY);
            if (in_fd == -1) {
                perror("mysh: cannot open input file");
                return;
            }
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
            args[i] = NULL;  // Remove redirection token and file name
        }
        i++;
    }
}

int mysh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {  // Child process
        handle_redirection(args);

        if (execvp(args[0], args) == -1) {
            perror("mysh: error at execvp");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {  // Error forking
        perror("mysh: error at fork");
    } else {  // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int mysh_execute(char **args) {
    if (args[0] == NULL) return 1;

    for (int i = 0; i < mysh_builtin_nums(); i++) {
        if (strcmp(args[0], builtin_cmd[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return mysh_launch(args);
}

void mysh_loop() {
    char *line;
    char **args;
    int status;

    do {
        char path[100];
        getcwd(path, sizeof(path));
        printf("[mysh %s]$ ", path);

        line = mysh_read_line();
        args = mysh_split_line(line);
        status = mysh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char *argv[]) {
    mysh_loop();
    return EXIT_SUCCESS;
}
