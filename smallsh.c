/*  Name: 		George Duensing
    Email: 		duensing@oregonstate.edu
    Course: 	cs344 Operating Systems
    Homework 3: smallsh
*/

#include "smallsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>	
#include <sys/wait.h> // for waitpid
#include <sys/types.h> // for kill
#include <unistd.h> // getpid
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <assert.h>

int BG_PROCESS_ALLOWED = 1; // Allows a background process

struct input {			// Holds cli input
	char *argc[513];
	int argCount;
	char *input_file;
	char *output_file;
	int bg_process; 	// Background process: 1 true, 0 false
};

/*	Expand the variable "$$" in the buffer with the shell's process id.
*	param: 				buffer: string to expand
*	precondition: 		char* string
*	postcondition:		$$ in string will be expanded to pid of shell	
*	return: char* string to expanded
*/
char* expandVariable(char* buffer)
{
	char* dest = NULL; // target after variable replacement
	if(buffer != NULL) {
		int buffLen = strlen(buffer);
		// assume pid about 6 char max + '\n' and '\0'
		dest = malloc(sizeof(char) * (buffLen + 8));
		for(int i = 0; i < buffLen; i++){
			if(buffer[i] == '$' && buffer[i+1] == '$') {
				buffer[i] = '%';
				buffer[i+1]  = 'd';
			}
			sprintf(dest, buffer, getpid()); // replace %d w/ pid
		}
	}
	return dest;
}

/*	Display the CLI prompt and get the user input. Validate input (not comment or blank line) and process input into the command struct.
*	params:				none
*	preconditions:		none
*	postcondition:		filled out command struct or null command struct on failure
*	return: pointer to struct *input command 
*/
struct input* getUserInput() 
{
	char *buffer = NULL;
	struct input* command = NULL;
	printf(": ");
	fflush(stdout);
	// cli to accept commands up to 2048 chars +1 for \n?
	size_t inputSize = 2049;
	getline(&buffer, &inputSize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;	// Remove the trailing '\n'
	if(buffer[0] != COMMENT	&& buffer[0] != '\0') {
		command = initCommandStruct(buffer);	// initialize input into command struct
	}
	free(buffer);
	return command;
}
	
/*	Parse CLI input and initialize input into command structure.
*	params:					string buffer to parse
*	preconditions:			string buffer to expand
*	postcondition:			user input is parsed into structure for use executing commands
*	return: 				pointer to struct *input command 
*/
struct input* initCommandStruct(char *buffer)
{
	char *token;	
	char *saveptr; // save place in line

	struct input *command = malloc(sizeof(struct input));
	command->bg_process = 0;					// Initialize to a foreground process
	command->argCount = 0;						// Initialize argument count
	command->output_file = NULL;
	command->input_file = NULL;
	// Get the first token and subsequently set buffer to NULL
	token = expandVariable(strtok_r(buffer, " ", &saveptr));						// Expand variable if needed or return original buffer
	command->argc[command->argCount] = malloc((strlen(token) + 1) * sizeof(char));	// command is always first token and always at position [0]
	assert(command->argc[command->argCount] != 0);
	strcpy(command->argc[command->argCount], token);
	command->argCount++;
	// Expand variable if needed or return original buffer
	while((token = expandVariable(strtok_r(NULL, " ", &saveptr))) != NULL)
	{
		if(strcmp(token, INPUT_OPERATOR) == 0) { 					// input redirection
			token = strtok_r(NULL, " ", &saveptr);
			command->input_file = malloc((strlen(token) + 1) * sizeof(char));
			assert(command->input_file != 0);
			strcpy(command->input_file, token); }
		else if(strcmp(token, OUTPUT_OPERATOR) == 0) { 				// output redirection
			token = strtok_r(NULL, " ", &saveptr);
			command->output_file = malloc((strlen(token) + 1) * sizeof(char));
			assert(command->output_file != 0);
			strcpy(command->output_file, token); }
		else if(strcmp(token, BG_PROCESS) == 0) { 					// background process
			command->bg_process = 1; }
		else { 	// regular old argument
			command->argc[command->argCount] = malloc((strlen(token) + 1) * sizeof(char));
			assert(command->argc[command->argCount] != 0);
			strcpy(command->argc[command->argCount], token);
			command->argCount++; }
	}
	// If no file was specified for i/o and it is meant to be a background process
	if(command->bg_process == 1 && command->input_file == NULL) { command->input_file = strdup("/dev/null"); }
	if(command->bg_process == 1 && command->output_file == NULL){ command->output_file = strdup("/dev/null"); }
	for(int i = command->argCount; i < 512; i++)	// Initialize rest of arguments to NULL to avoid bad behavior in execvp()
		command->argc[i] = NULL;
	return command;
}

/*	Built in command. Exit the shell.
*	params:				none
*	preconditions:		command struct points here
*	postcondition:		exit
*	return: 			none
*/
void exit_shell() { 
	for(int i = 0; i < 200; i++) { // Kill background processes
		if(BG_PROCESSES[i] != -5) { 
			kill(BG_PROCESSES[i],SIGTERM);
			BG_PROCESSES[i] = -5;
		} 
	} 
	exit(0);
}

/*	Change directory to user specified location, else change to home directory 
*	params:					command struct w/ optional directory path
*	preconditions:			command struct w/ valid input
*	postcondition:			directory is changed
*	return: 				none
*/
void cd(struct input* command)
{
	if(command->argCount == 1)
		chdir(getenv("HOME"));		// Get the home directory from environment variable
	else
 		chdir(command->argc[1]);	// User specified directory to switch to
}

/*	The status command prints out either the exit status or the terminating signal
*	of the last foreground process ran by your shell.
*	params:					status code from waitpid
*	preconditions:			foreground process ran and waitpid called
*	postcondition:			Exit status or signal printed
*	return: 				none
*/
void status(int status)
{
	if(WIFEXITED(status))	// If the process exited print code
		printf("exit status %d\n", WEXITSTATUS(status)); fflush(stdout);
	if(WIFSIGNALED(status))	// if process terminated from signal print the signal
		printf("terminated by signal %d\n", WTERMSIG(status)); fflush(stdout);
}

/*	Check on the background signals and print either exit status or signal termination status
*	params:					status code from waitpid
*	preconditions:			background process ran and waitpid called
*	postcondition:			print update on status of background command
*	return: 				none
*/
void checkBgProcesses(int status)
{
	int pid = -5;
	do {	// Wait for any child proccess to terminate until parent is returned
		pid = waitpid(-1, &status, WNOHANG);
		if(WIFEXITED(status) != 0 && pid > 0) { // Print the exit status if exited
			printf("\nbackground pid %d is done: exit value %d\n", pid, WEXITSTATUS(status));
			fflush(stdout);
		}
		else if(WIFSIGNALED(status) != 0 && pid > 0) { // Print the termination signal if terminated
			printf("background pid %d terminated by signal %d\n", WTERMSIG(status)); fflush(stdout);
		}
	} while(pid > 0);	// 0 = parent process
}

/*	Route the user command to appropriate destination. Either built in, or to be forked and executed with exec() family
*	params:				Parsed command structure
*	preconditions:		Parsed command structure
*	postcondition:		Either a built in command is run or a forked process is executed.
*	return: 			none
*/
void runcommand(struct input* command)
{
	// Hold the status of the foreground process
	static int processStatus;
	
	if (strcmp(command->argc[0], "cd") == 0)
		cd(command);
	else if (strcmp(command->argc[0], "exit") == 0)
		exit_shell();
	else if (strcmp(command->argc[0], "status") == 0)
		status(processStatus);
	else	// Execute non built-in command
		Execute(command, &processStatus);

	checkBgProcesses(processStatus);	// Check on and output background processes status before returning control to CLI
}

/*	Execute a user specified process. Handle signals SIGTSTP and SIGINT for background and foreground processes
*	params:				command: user input, command, arguments, redirection, background process
*						processStatus: holds waitpid status for foreground command
*	preconditions:		parsed command
*	postcondition:		user specified process is executed, or error handling is printed. Update process status code
*	return: 			none
*	ref: 4_2_execv_fork_ls.c
*/
void Execute(struct input *command, int processStatus)
{
	pid_t spawnPid = fork();		// Fork a new process
	switch(spawnPid){	
	case -1:						// Error
		perror("fork()\n");
		exit(1);
		break;
	case 0: // child process
		if(command->bg_process == 1){ 						// init signal handlers
			struct sigaction SIGINT_action = {0};
			SIGINT_action.sa_handler = SIG_IGN; 			// background child to ignore SIGINT
			sigfillset(&SIGINT_action.sa_mask);				// ignore other signals
			SIGINT_action.sa_flags = SA_RESTART;
			sigaction(SIGINT, &SIGINT_action, NULL);		// register
			struct sigaction SIGTSTP_action = {0};			// SIGTSTP
			SIGTSTP_action.sa_handler = SIG_IGN;			// background child to ignore SIGTSTP
			sigfillset(&SIGTSTP_action.sa_mask);
			SIGTSTP_action.sa_flags = SA_RESTART;
			sigaction(SIGTSTP, &SIGTSTP_action, NULL);		// register
		} else { // foreground signal handlers
			struct sigaction SIGINT_action = {0};			// SIGINT
			SIGINT_action.sa_handler = SIG_DFL;				// default handler stop command
			sigfillset(&SIGINT_action.sa_mask);				// ignore other signals
			SIGINT_action.sa_flags = SA_RESTART;
			sigaction(SIGINT, &SIGINT_action, NULL);
			struct sigaction SIGTSTP_action = {0};			// SIGTSTP
			SIGTSTP_action.sa_handler = SIG_IGN;			// foreground child to ignore SIGTSTP
			sigfillset(&SIGTSTP_action.sa_mask);			// ignore other signals
			SIGTSTP_action.sa_flags = SA_RESTART;
			sigaction(SIGTSTP, &SIGTSTP_action, NULL);		// register
		}
		// If redirection needed or it is a background process:
		if(command->input_file != NULL || command->bg_process == 1 && BG_PROCESS_ALLOWED != 0)
			inputRedirect(command);
		if(command->output_file != NULL || command->bg_process == 1 && BG_PROCESS_ALLOWED != 0)
			outputRedirect(command);
		execvp(command->argc[0], command->argc);			// Run the command in argc[0] and pass arguments
		perror(command->argc[0]); 							// returns here only on error
		exit(1);
		break;
	default: // parent
		if(command->bg_process == 1 && BG_PROCESS_ALLOWED != 0) {			// Print background pid
			printf("background pid is %d\n", spawnPid); fflush(stdout);
			spawnPid = waitpid(spawnPid, &processStatus, WNOHANG);			// Don't wait for the process to return
			for(int i = 0; i < 200; i++) { if(BG_PROCESSES[i] != -5) { BG_PROCESSES[i] = spawnPid;} } // keep track of background process and add to array
		}
		else {
			spawnPid = waitpid(spawnPid, &processStatus, 0);				// Wait for foreground process to return
			if(WIFSIGNALED(processStatus) > 0)  // parent process prints signal number if child is stopped in foreground mode
				printf("terminated by signal %d\n", WTERMSIG(processStatus)); fflush(stdout);
		}
		break;
	}
}

/*	Redirect stdout to the file specified in command. On error, set status.
*	params:				command: command->output_file is the file to redirect to
*	preconditions:		command struct to have file for redirection or redirect to /dev/null for background process if not specified 
*	postcondition:		output is redirected per user specified location
*	return: 			none
*	ref: 5_4_sortViaFiles.c
*/
void outputRedirect(struct input* command)
{
	// Open target file
	int targetFD = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0770);
	if (targetFD == -1) {	// Error
		perror(command->output_file); 
		exit(1); 
	}
	// Redirect stdout to target file
	int result = dup2(targetFD, 1);
	if (result == -1) { // if error
		perror(command->output_file); 
		exit(1); 
	}
}

/*	Redirect stdin to the file specified in command. On error, set status.
*	params:				command: command->input_file is the file to redirect to
*	preconditions:		command struct to have file for redirection or redirect to /dev/null for background process if not specified 
*	postcondition:		output is redirected per user specified location
*	return: 			none
*	ref: 5_4_sortViaFiles.c
*/
void inputRedirect(struct input* command) 
{
	// Open source file
	int sourceFD = open(command->input_file, O_RDONLY, 0770);
	if (sourceFD == -1) { // if error
		perror(command->input_file); 
		exit(1); 
	}
	  // Redirect stdin to source file
	int result = dup2(sourceFD, 0);
	if (result == -1) { 
		perror(command->input_file); 
		exit(2); 
	}
}

/*	Signal handler for SIGTSTP. First signal disable background processes. Second signal allow background processes.
*	params:				signal number
*	preconditions:		global variable controls background allowance
*	postcondition:		background process is disabled or enabled
*	return: 			none
*/
void handle_SIGTSTP(int signo){
	if(BG_PROCESS_ALLOWED == 1){	// If background process is allowed
		char* message = "\nEntering foreground-only mode (& is now ignored)\n";
		// We are using write rather than printf
		write(STDOUT_FILENO, message, 51); 	// Use reentrant function to print 
		BG_PROCESS_ALLOWED = 0;				// Disable background processes
		fflush(stdout);	
	}
	else if(BG_PROCESS_ALLOWED == 0){	// If background process is disabled
		char* message = "\nExiting foreground-only mode\n";
		// We are using write rather than printf
		write(STDOUT_FILENO, message, 31); 	// Use reentrant function to print
		BG_PROCESS_ALLOWED = 1;				// Allow background processes
		fflush(stdout);
	}
}

/*	Free allocated memory in the command structure
*	params:				memory allocated for command structure 
*	preconditions:		non-null command structure
*	postcondition:		free memory allocated for command
*	return: 			none
*/
void destroyCommand(struct input* command)
{
	if (command != NULL)
	{
		if (command->output_file != NULL) {
			free(command->output_file);
			command->output_file = NULL;
		}
		if (command->input_file != NULL) {
			free(command->input_file);
			command->input_file = NULL;
		}
		if(command->argCount > 0)
		{
			for(int i = 0; i < command->argCount; i++) {
				if(command->argc[i] != NULL){
					free(command->argc[i]);
					command->argc[i] = NULL;
				}
			}
		}
		free(command);
		command = NULL;
	}
}