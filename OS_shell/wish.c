#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <fcntl.h>

// For debugging.
void print_array_from_to(char *arr[], int from, int to)
{
    for (int j = from; j < to; j++)
    {
        printf("%s ", arr[j]);
    }
}

int execute(char line[])
{
    strtok(line, "\n");
    int token_count = 0;
    // Count tokens in the command to determine list size for later
    for (int i = 0; line[i] != '\0'; i++)
    {
        if ((line[i] != ' ' && line[i] != '\t') && (line[i + 1] == ' ' || line[i + 1] == '\t' || line[i + 1] == '\0'))
        {
            token_count++;
        }
    }
    char *tokens[token_count];
    char *token;
    int i = 0;
    int redirection_counter = 0;
    int input_index = 0;
    int output_index = 0;
    // Tokenize the input line, and check for I/O redirections. This technically doesn't care when the redirection comes.
    token = strtok(line, " \t");
    while (token != NULL)
    {
        tokens[i] = token;
        // Checks that < or > is the only character in the token.
        if (*token == '>' && *(token + 1) == '\0')
        {
            output_index = i;
            redirection_counter++;
        }
        else if (*token == '<' && *(token + 1) == '\0')
        {
            input_index = i;
            redirection_counter++;
        }
        token = strtok(NULL, " \t");
        i++;
    }

    int parameter_count = token_count - redirection_counter * 2 - 1;
    if (redirection_counter > 2)
    {
        printf("Error: Too many I/O Redirections. \n");
        exit(EXIT_FAILURE);
    }

    // Split then tokens for readability
    char *command = tokens[0];
    char *parameters[parameter_count + 2];
    parameters[0] = tokens[0];
    for (int i = 0; i < parameter_count; i++)
    {
        parameters[i + 1] = tokens[i + 1];
    }
    parameters[parameter_count + 1] = NULL;

    // Uncomment the following to print task a)
    // printf("Command: %s ", command);
    // printf("| Parameters: ");
    // print_array_from_to(parameters, 0, parameter_count);
    // printf("| Redirections: ");
    // print_array_from_to(tokens, token_count - redirection_counter * 2, token_count);
    // printf("\n");

    // d)
    // This if statement has to be above fork and execute, because internal commands should have precedence over external.
    // Takes in a parameter that is an absolute path starting with a “/”
    if (!strcmp(command, "cd"))
    {
        int result;
        // parameters[1] is first parameter after command
        char *string = parameters[1];
        result = chdir(string);
        // if chdir fails result becomes -1
        if (result == -1)
        {
            printf("Error: Cannot find directory, try again.\n");
        }
    }
    else if (!strcmp(command, "exit"))
    {
        printf("Exiting...\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        pid_t process = fork();
        if (process == 0)
        {
            int input_fd;
            int output_fd;
            // Considered adding a check to see if the user wants to read and write to the same file.
            // After seeing that the command "cat < test.txt > text.txt" just truncated the file and didn't 'rewrite' the content when we called it in a normal unix shell, 
            // we decided that ultimately wasn't necessary. Could have done so by comparing the input and output path, and opening with O_RDWR as well as O_TMPFILE.
            if (input_index != 0)
            {
                // Open file to read
                char *input_path = tokens[input_index + 1];
                input_fd = open(input_path, O_RDONLY, 0);
                if (input_fd == -1)
                {
                    perror("Error: Could not open the given file. \n");
                    exit(EXIT_FAILURE);
                }
                dup2(input_fd, 0);
            }
            if (output_index != 0)
            {
                // Open file to write
                char *output_path = tokens[output_index + 1];
                // O_WRONLY - write to file, O_TRUNC - overwrite content, O_CREAT - create if missing, S_IRWXU - give user all access permissions
                output_fd = open(output_path, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU);
                dup2(output_fd, 1);
            }
            int ex = execv(command, parameters);
            if (ex == -1)
            {
                printf("Error: Could not perform command \"%s\".\n", command);

            }
        }
        else
        {
            wait(NULL);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    //Current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("Error: can not get current working directory.\n");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t linesize = 0;

    // e)
    // If argc is greater than one, another file is being passed with the function when called.
    if (argc > 1)
    {
        FILE *stream;
        stream = fopen(argv[1], "r");
        ssize_t next;
        if (stream == NULL)
        {
            printf("Error: shell script can not be read. \n");
            exit(EXIT_FAILURE);
        }
        while ((next = getline(&line, &len, stream)) != -1)
        {
            execute(line);
        }
        free(line);
        fclose(stream);
        return 0;
    }
    else
    {
        while (1)
        {
            getcwd(cwd, sizeof(cwd));
            printf("Shell: %s>$ ", cwd);
            linesize = getline(&line, &len, stdin);
            if (line[0] == '\n')
            {
                continue;
            }
            execute(line);
        }
    }
    return 0;
}

/* Presumptions:
    - I/O redirections have exactly one filename associated with each redirection.
    - Only one of each input and output redirections are present.
    - Parameters are called with the filename associated with the file being executed first, then other options.
*/
