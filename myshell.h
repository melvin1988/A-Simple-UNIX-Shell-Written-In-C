/*
 * File:        myshell.h
 * Author:      Melvin Sim
 * Date:        25 Mar 2021
 */

//Main functions
void blockSignal();
void getInput(char input[], char *prompt);
int parseCommand(char input[], char *token[], Command command[]);
char *processPrompt(char *prompt, char new_prompt[], int index, Command command[]);
void processPWD(int index, Command command[]);
void processCD(int index, Command command[]);
int processPipe(int index, Command command[]);
void processStdin(char *command_token[], int index, Command command[]);
void processStdout(char *command_token[], int index, Command command[]);
int processPipeAndStdin(char *command_token[], int index, Command command[]);
int processPipeAndStdout(char *command_token[], int index, Command command[]);
void executeCommand(int index, Command command[]);
void catchSigChld();

//Helper functions
int builtInCommand(int index, Command command[]);
void getFileNameToken(char *filename_token[]);
int isFileName(char *input, char *filename[]);
int isDirectory(char *filename);
int isWildCard(int index, Command command[]);
int isWildCardForStdinStdout(int index, Command command[]);
int numOfWildCardFiles(char *input);
void expandWildCard(char *input, char *token[]);
void getArgvForExecuteCommand(int index, Command command[], char *argv[]);
void getArgvForStdinStdout(int index, Command command[], char *argv[]);
void getArgvForWildCard(int index, Command command[], char *argv[]);
void getArgvForWildCardStdinStdout(int index, Command command[], char *argv[]);
void claimChildren();
