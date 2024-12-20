#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#define BUFFERSIZE 10240

int get_infile_fd(char *Infilename){ //this function gets the infile's fd and returns it

    int infile_fd = open(Infilename, O_RDONLY);
    if (infile_fd < 0){
        perror("Failure to open file");
        exit(1);
    }

    return infile_fd;


}
void read_infile_and_redirect_to_inpipe(int Infilefd, int InputPipefd){ //this function gets the infile's content and
    char buffer[BUFFERSIZE];                                            //redirects it to the input pipe
    ssize_t read_infile = read(Infilefd, buffer, BUFFERSIZE);
    while (read_infile > 0){
        int bytes_written = 0;
        while (bytes_written < read_infile){
            int write_to_input_pipe = write(InputPipefd, buffer + bytes_written, read_infile - bytes_written);
            if (write_to_input_pipe < 0){
                perror("Writing error occurred");
                exit(1);
            }
            else{
                bytes_written += write_to_input_pipe;
            }
        }
        read_infile = read(Infilefd, buffer, BUFFERSIZE);
    }
    if (read_infile < 0){
        perror("Reading error occurred");
        exit(1);
    }

    printf("Infile successfully transferred to pipe \n");
}
void show_processed_infile(int output_from_2nd_pipe){ //gets the output from exec and transforms the output to be printed based on endlines
    char buffer[BUFFERSIZE];
    ssize_t read_processed_infile = read(output_from_2nd_pipe, buffer, BUFFERSIZE);
    if (read_processed_infile < 0){
        perror("Reading error occurred");
        exit(1);
    }
    char* token = strtok(buffer, "\n");
    while (token != NULL){
        printf("Data received through pipe %s\n", token);
        token = strtok(NULL, "\n");
    }
}

void exec_from_pipe(int input_pipe_fd, int output_pipe_fd){ //using dup2 to use stdout/inand to refer to the pipes and exec to use cut
    dup2(input_pipe_fd, 0);
    dup2(output_pipe_fd, 1);

    char *cut_args[] = {"cut", "-c5-", NULL };
    execvp("cut", cut_args);
    perror("Failed to extract the intended data from the infile");
    exit(1);

}

int main(int argc, char *argv[])
{
    int input_fd1[2],output_fd2[2]; //initializing and declaring the pipes and their fds
    int input_pipe = pipe(input_fd1);
    int output_pipe = pipe(output_fd2);
    if (input_pipe < 0 || output_pipe < 0){
        perror("Failed to create pipes");
        exit(1);
    }
    pid_t p = fork(); //creates a child process
    char* infile_name = argv[1];


    if (p > 0){ //parent
        int infile_fd = get_infile_fd(infile_name);
        close(input_fd1[0]); //closing the pipe ends that are not needed
        read_infile_and_redirect_to_inpipe(infile_fd, input_fd1[1]);

        wait((int *)0);
        close(output_fd2[1]);
        show_processed_infile(output_fd2[0]);
    }
    else if(p == 0){ //child
        close(input_fd1[1]);
        close(output_fd2[0]);
        exec_from_pipe(input_fd1[0], output_fd2[1]);


    }
    else{ //fork error
        perror("Error creating process");
        close(infile_fd);
        exit(1);
    }
    close(infile_fd);

    return 0;
}
