#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

//Adressing the questions for B
    //
    //What is the largest block size supported on your system?
    //
    // Trying the fcntl code provided by an instructor on piazza
    // // int s = fcntl(fd[0], 1032);
    // // printf("s is: %d", s);
    // returned a size of ~6 553 610 526. This however seemed very strange, as it didn't correspond with the different inputs I tried. Inputs this high 
    // gave 0 bandwith, or read(2) returned -1. This code snippet however: 
    // // struct statvfs fs_stat;
    // // statvfs(".", &fs_stat);
    // // printf("%lu\n", fs_stat.f_bsize);
    // gave 4096 as the file system's block size, which indeed was the size we got the highest bandwith for.
    // In addition, with input sizes higher than 4096, read(2) sometimes returned -1, especially for integers just over powers of two (4100, 8200). Higher
    // numbers (5100, 10000) failed less frequently.
    //
    //What is the highest bandwidth you can achieve and at which block size is this bandwidth achieved?
    //
    // // Size (bytes)  ~   Bandwidth 
    // 1                ~       1 715 700
    // 10               ~      16 980 500    
    // 100              ~     146 697 200
    // 1.000            ~   1 427 620 000
    // 2.048            ~   2 650 089 472
    // 4.096            ~   4 765 028 352   Highest
    // 8.000            ~   4 183 696 000
    // 8.192            ~   4 262 518 784
    // 10.000           ~   1 343 360 848
    // 50.000           ~   1 918 062 784
    //Highest bandwith was achieved at 4096 bytes, and was pretty stable up to 8192.
    //
    //Does the bandwidth change when you start several instances of your program at the same time?
    //
    //Running mulitiple instances caused a drop in bandwith, seemingly somewhere between 1/4 and 1/3 of the bandwith per instance, at block size 4096.
    
//Adressing the questions for D
    //
    //What is the largest block size supported on your system?
    //
    //Through running the system with a named pipe it was possible to enter 10MB+
    //From reading from readStatus for the highest value we achieved was alos here 8192 Bytes?? as the largest block size
    //
    //What is the highest bandwidth you can achieve and at which block size is this bandwidth achieved?
    //
    // // Size (bytes)  ~   Bandwidth 
    // 1                ~         957 848
    // 10               ~       9 507 220  
    // 100              ~      93 955 100  
    // 1.000            ~     882 260 000
    // 4.096            ~   3 231 948 800   HIGHEST
    // 8.192            ~   1 422 729 216 
    // 10.000           ~   2 125 570 192
    // 100.000          ~   1 128 902 400
    // 1.000.000        ~   1 789 716 800 
    // 10.000.000       ~   1 377 915 904
    
    
    //Highest bandwith was achieved at 4096.
    //
    //Does the bandwidth change when you start several instances of your program at the same time?
    //
    //Running mulitiple instances caused a significant drop in bandwith here as well, at the same rate as for unnamed pipe.


unsigned long int cumulativeBytesRead = 0;
unsigned long int bytesRead = 0;

char *makeData(size_t size)
{
    char *block = malloc(size);
    for (int i = 0; i < size; i++)
    {
        //Random filler data
        block[i] = 'x';
    }
    return block;
}

void sigHandler(int signum)
{
    if (signum == SIGALRM)
    {
        printf("Current pipeline bandwidth: %lu\n", bytesRead);
        cumulativeBytesRead += bytesRead;
        bytesRead = 0;
        alarm(1);
    }
    else
    {
        printf("Wrong signal in signal handler");
        exit(EXIT_FAILURE);
    }
}

void sigHandler2(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("Cumulative number of bytes: %lu\n", cumulativeBytesRead);
    }
}

int unnamedPipe(size_t size)
{
    int pid, fd[2];
    //fd[0] -read
    //fd[1] - write

    if (pipe(fd) == -1)
    {
        printf("Error: Couldn't open the pipe.\n");
        exit(EXIT_FAILURE);
        return -1;
    }

    pid = fork();
    if (pid == -1)
    {
        printf("Error: Couldn't fork process.\n ");
        exit(EXIT_FAILURE);
        return -1;
    }
    if (pid == 0)
    {
        signal(SIGUSR1, SIG_IGN);
        close(fd[0]);
        dup2(fd[0], 1);
        int writeStatus = 0;
        char *block = makeData(size);
        int y = 2;

        // printf("Random data from child: %s\n", block);

        while (writeStatus != -1)
        {
            writeStatus = write(fd[1], &block, size);
            if (writeStatus == -1)
            {
                printf("Error: Couldn't write to the pipe.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
        }

        if (close(fd[1]) == -1)
        {
            {
                printf("Error: Couldn't close write file descriptor.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
        }
        free(block);
    }
    else
    {
        signal(SIGALRM, sigHandler);
        signal(SIGUSR1, sigHandler2);
        alarm(1);
        printf("%d\n", getpid());
        close(fd[1]);
        dup2(fd[0], 0);
        int x;
        int readStatus = 0;
        char *blockParent = malloc(size);
        // int readStatus = read(fd[0], &blockParent, size);

        while (readStatus != -1)
        {
            //Read returns how many bytes are read if successful
            readStatus = read(fd[0], &blockParent, size);
            if (readStatus == -1)
            {
                printf("Error: Couldn't read from the pipe.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
            bytesRead += readStatus;
        }

        if (close(fd[0]) == -1)
        {
            {
                printf("Error: Couldn't close read file descriptor.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
        }
        free(blockParent);
        wait(NULL);
    }

    return 0;
}

int namedPipe(size_t size)
{
    int pid, fd;

    // FIFO file path
    char *myfifo = "/tmp/myfifo";
    //Have to remove the file already there, else will fileStatus return -1 and crash.
    remove(myfifo);

    // Creating the named file(FIFO)
    // mkfifo(<pathname>, <permission>)
    int fileStatus = mkfifo(myfifo, 0666);

    if (fileStatus == -1)
    {
        printf("Error: Couldn't create file.\n");
        exit(EXIT_FAILURE);
        return -1;
    }

    pid = fork();
    if (pid == -1)
    {
        printf("Error: Couldn't fork process.\n ");
        exit(EXIT_FAILURE);
        return -1;
    }
    if (pid == 0)
    {
        signal(SIGUSR1, SIG_IGN);
        fd = open(myfifo, O_WRONLY);
        int writeStatus = 0;
        char *block = makeData(size);
        int y = 2;
        // printf("Random data from child: %s\n", block);

        while (writeStatus != -1)
        {
            writeStatus = write(fd, block, size);
            if (writeStatus == -1)
            {
                printf("Error: Couldn't write to the pipe.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
        }

        if (close(fd) == -1)
        {
            {
                printf("Error: Couldn't close write file descriptor.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
        }
        free(block);
    }
    else
    {
        signal(SIGALRM, sigHandler);
        signal(SIGUSR1, sigHandler2);
        alarm(1);
        printf("%d\n", getpid());
        fd = open(myfifo, O_RDONLY);
        int x;
        int readStatus = 0;
        char *blockParent = malloc(size);
        // int readStatus = read(fd[0], &blockParent, size);

        while (readStatus != -1)
        {
            //Read returns how many bytes are read if successful
            readStatus = read(fd, &blockParent, size);
            if (readStatus == -1)
            {
                printf("Error: Couldn't read from the pipe.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
            bytesRead += readStatus;
        }

        if (close(fd) == -1)
        {
            {
                printf("Error: Couldn't close read file descriptor.\n");
                exit(EXIT_FAILURE);
                return -1;
            }
        }
        free(blockParent);
        wait(NULL);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    size_t size;
    printf("Block size (number of bytes): ");
    scanf("%lu", &size);
    //Alarm must come after scan, otherwise it will start before were able to enter blocksize

    unnamedPipe(size);
    // namedPipe(size);

    return 0;
}


