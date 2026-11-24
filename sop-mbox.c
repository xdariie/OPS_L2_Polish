#define _XOPEN_SOURCE 500
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#define MAX_SMBOX_MSGS 25
#define BATCH_BYTES 64
#define MAX_FIFOS 4

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

static void usage(char* name)
{
    fprintf(stderr, "USAGE: %s mailbox_file [fifo ...]\n", name);
    exit(EXIT_FAILURE);
}

static void msleep(int ms)
{
    struct timespec time = {.tv_sec = 0, .tv_nsec = 1000000 * ms};
    nanosleep(&time, NULL);
}

struct smbox_file{
    void *mmap_addr;
    size_t size;
};

void dump_file(struct smbox_file *file)
{
    if (file->mmap_addr == NULL || file->size == 0)
    {
        fprintf(stderr, "File is not mapped or empty\n");
        return;
    }
    fflush(stdout);
    write(STDOUT_FILENO, file->mmap_addr, file->size);
}

void mmap_file(char *filename, struct smbox_file *file)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        ERR("open");

    struct stat st;
    if (fstat(fd, &st) == -1)
        ERR("fstat");
    
    if(st.st_size == 0){
        fprintf(stderr,"File is empty\n");
        exit(EXIT_FAILURE);
    }


    file->size = st.st_size;
    file->mmap_addr = mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file->mmap_addr == MAP_FAILED)
        ERR("mmap");

    close(fd);
}

int main(int argc, char *argv[]){
    struct smbox_file file = {0};
    if(argc<2) usage(argv[0]);
    mmap_file(argv[1], &file);
    int flags = fcntl(STDIN_FILENO, F_GETFL);

    if (flags == -1)
    {
        ERR("fcntl");
        return EXIT_FAILURE;
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        ERR("fcntl");
    }
    char buffer[BATCH_BYTES];
    int selected = 0;
    int running = 1;

    while (running)
    {
        ssize_t n = read(STDIN_FILENO, buffer, BATCH_BYTES);

        if (n > 0)
        {
            for (ssize_t i = 0; i < n && running; i++)
            {
                switch (buffer[i])
                {
                    case 'w':
                        if (selected > 0)
                            selected--;

                        printf("Selected message: %d\n",
                               selected);
                        break;

                    case 's':
                        selected++;

                        printf("Selected message: %d\n",
                               selected);
                        break;

                    case 'd':
                        printf("Dumping the file...\n");
                        dump_file(&file);
                        break;

                    case 'h':
                        printf("Printing headers...\n");
                        break;

                    case 'm':
                        printf("Printing message %d\n",
                               selected);
                        break;

                    case 'q':
                        running = 0;
                        break;

                    case '\n':
                        break;

                    default:
                        printf("Invalid input\n");
                        break;
                }
            }

            fflush(stdout);
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                msleep(100);
            }
            else if (errno != EINTR)
            {
                ERR("read");
                
            }
        }
    }

    fcntl(STDIN_FILENO, F_SETFL, flags);
    munmap(file.mmap_addr, file.size);

    return EXIT_SUCCESS;
}