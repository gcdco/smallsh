/*  Name: George Duensing
    Email: duensing@oregonstate.edu
    Course: cs344 Operating Systems
    Homework 3: smallsh
    In this assignment you will write smallsh your own shell in C. 
    smallsh will implement a subset of features of well-known shells, 
    such as bash. Your program will:
        -Provide a prompt for running commands
        -Handle blank lines and comments, which are lines beginning with the # character
        -Provide expansion for the variable $$
        -Execute 3 commands exit, cd, and status via code built into the shell
        -Execute other commands by creating new processes using a function from the exec family of functions
        -Support input and output redirection
        -Support running commands in foreground and background processes
        -Implement custom handlers for 2 signals, SIGINT and SIGTSTP
*/
// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>  // Directories
#include <fcntl.h>   // Files
#include "smallsh.h"
/*   Compile the program as follows:7
*       gcc --std=gnu99 -o smallsh main.c
*/

int main(int argc, char* argv[])
{   // ref. modules
    // signal handlers
    struct sigaction SIGINT_action = {0};               // Initialize SIGINT_action struct to be empty
    SIGINT_action.sa_handler = SIG_IGN;                 // Ignore SIGINT in shell    
    sigfillset(&SIGINT_action.sa_mask);                 // Block all catchable signals while handle_SIGINT is running
    SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);            // Install our signal handler
    
    struct sigaction SIGTSTP_action = {0};              // Initialize SIGTSTP_action struct to be empty
    SIGTSTP_action.sa_handler = handle_SIGTSTP;         // Register handle_SIGTSTP as the signal handler
    sigfillset(&SIGTSTP_action.sa_mask);                // Block all catchable signals while handle_SIGTSTP is running
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);          // Install our signal handler
    
    while(1)
    {
        // Get user command and validate
        struct input* command = NULL;
        if((command = getUserInput()) != NULL)          // Get the input
        {
            runcommand(command);                        // Execute the command from user
            destroyCommand(command);                    // Free memory
        }

    }
    return EXIT_SUCCESS;
}