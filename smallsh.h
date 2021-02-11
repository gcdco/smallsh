/*  Name:       George Duensing
    Email:      duensing@oregonstate.edu
    Course:     cs344 Operating Systems
    Homework 3: smallsh
*/
#ifndef SMALLSH_H
#define SMALLSH_H

#define INPUT_OPERATOR "<"
#define OUTPUT_OPERATOR ">"
#define BG_PROCESS "&"
#define COMMENT '#'
#define DOLLAR '$'

int BG_PROCESSES[200];       // hold background processes

struct input;

int validateBuffer(const char*);
struct input* getUserInput();
int countVariables(char*);
char* expandVariable(char*);
struct input* initCommandStruct(char*); // return succeed or fail?
void destroyCommand(struct input*);
void Execute(struct input*, int);
void runcommand(struct input*);
void outputRedirect(struct input*);
void inputRedirect(struct input*);
void exit_shell();
void cd(struct input*);
void status(int);
void checkBgProcesses(int);
void handle_SIGINT(int);
void handle_SIGTSTP(int);

#endif