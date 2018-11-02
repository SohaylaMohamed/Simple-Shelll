#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
//Maximum size of array
size_t  memorySize = 128;
//Flag to indicate whether the process is background or not
int bgFlag = 0;
//Log file
FILE *lf;

/**
 * Writes  entries into the opened Log file
 * @param pid  Id of the process
 * @param flag state of the process/shell
 */
void writeLogFile(int pid, int flag) {
    time_t now = time(0);
    char *dt = ctime(&now);
    dt[strlen(dt) - 1] = NULL;
    if(flag == 0) {
        fprintf(lf, "{%s} Child process was terminated.\n", dt);
    } else if (flag == 1) {
        fprintf(lf, "{%s} Child process with ID %d is running.\n", dt, pid);
   } else if (flag == 2) {
       fprintf(lf, "{%s} shell is running.\n", dt);
    } else if (flag == 3) {
        fprintf(lf, "{%s} shell is terminated, EXIT_SUCCESS.\n", dt);

    } else if(flag == 4) {
        fprintf(lf, "{%s} Error Occured, EXIT_FAILURE.\n", dt);
    } else if( flag == 5) {
        fprintf(lf, "{%s} Child process with ID %d is terminated.\n",dt, pid);
    }
}
/**
 * Handles signals sent by a zombie child to the parent process
 * @param signal the sent signal
 */
void signalHandler(int signal)
{
    if (signal==SIGCHLD) {
        pid_t pid;
        while (waitpid(-1, NULL, WNOHANG) > 0 ) {
            //Process is terminated
            writeLogFile((int) SIGCHLD, 0);
        }
    }


}

/**
 * Execute the commands entered in the shell
 * @param args  arguments that will be sent to execvp() function
 * @return 1 to continue the loop of the shell
 */

int shellExecute(char **args)
{
    pid_t pid;
    int status;
    //check for signals from child
    signal(SIGCHLD,signalHandler);
    //fork a new process
    pid = fork();
    //if pid==0 then it's the child process
    if (pid == 0) {
        setpgrp(); //synchronization
        if (execvp(args[0], args) == -1) {
            perror("THis Error Occured: " );
        }
        //Error occured, exit failure
        writeLogFile((int) pid, 4);

        exit(EXIT_FAILURE);

    } else if (pid < 0) { //if pid <0 then an error in forking has occured
          perror("This Error Occured: ");
          //Error occured, exit failure
        writeLogFile((int) pid, 4);

        exit(EXIT_FAILURE);
    } else {
        setpgid(pid, pid); //synchronization
        if(!bgFlag) { //if background flag is not set, parent waits
            //Process is running
            writeLogFile(pid, 1);
            while(!waitpid(pid, &status, WNOHANG));
            //Process is terminated
            writeLogFile((int) pid, 5);
        } else { //parent doesn't wait
            //Process is running
            writeLogFile(pid, 1);
        }

    }
    return 1;
}

/**
 * Parses the line entered in the shell to get the arguments
 * splits the entered line around the space delimiter
 * @param line entered line
 * @param args to store the arguments
 */

void parseLine(char* line, char ** args) {
    memset(args, NULL, sizeof args);

    args[0] = strtok(line, " ");
    bgFlag = 0;
    int i = 0;

    while(args[i] != NULL) {

        args[++i] = strtok(NULL, " \n");
    }
    //empty command
    if(args[0] == NULL) {
        return;
    }
    //check if background if yes set the flag
    if(strcmp(args[i-1],"&") == 0){
        bgFlag = 1;
        args[i-1] = NULL;
    }
    args[i] = NULL;

}
/**
 * Contains the loop of the shell
 * read a command line from the user
 * checks if the command is empty then it continues without executing
 * checks if the command is exit then it terminates the shell
 */
void readCommand() {
    //open the log file
    lf = fopen("logFile.txt","w");
    //shell is running
    writeLogFile(-1, 2);
    char* line = static_cast<char *>(malloc(memorySize));
    char** arguments =  static_cast<char **>(malloc(memorySize * sizeof(char*)));;
    int status = 1;

    while(status) {
        cout << "Shell>";
        cin.getline(line,memorySize);
        parseLine(line, arguments);
        int i =0;
        //if command is empty continue the loop
        if ( arguments[0] == NULL) {
            continue;
        }
        //if command is exit terminate the loop
        if(strcmp(arguments[0],"exit") == 0 ) {
            break;
        }
        //if command is cd
        if(strcmp(arguments[0],"cd") == 0 ) {
            //check if no arguments
            if (arguments[1] == NULL) {
                chdir(getenv("HOME")); //go to directory Home
            } else if (chdir(arguments[1]) == -1) { //go to new directory
                    perror("This Error has Occured: ");
                }

        } else {
            //execute the command
            status = shellExecute(arguments);
        }

    }

}


int main() {
    readCommand();
    //shell is terminated
    writeLogFile(-1, 3);
    //close the log file
    fclose(lf);
    exit(EXIT_SUCCESS);
    return 0;
}


