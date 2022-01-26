/*
 * File:        main.c
 * Author:      Melvin Sim
 * Date:        25 Mar 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "command.h"
#include "myshell.h"

#define STR_SIZE 1024

int main() {
  //Declaration of variables
  char *command_token[MAX_NUM_TOKENS];
  Command command[MAX_NUM_COMMANDS];
  int n_commands;
  char input[STR_SIZE];
  char *prompt = "%";
  char new_prompt[STR_SIZE];
  int n_pipes;

  blockSignal(); //block SIGINT, SIGQUIT, SIGTSTP

  //Start of program
  while(strcmp(input, "exit") != 0) {
    getInput(input, prompt);
    n_commands = parseCommand(input, command_token, command);
    for(int i = 0; i < n_commands; i++) {
      if(builtInCommand(i, command)) {
        prompt = processPrompt(prompt, new_prompt, i, command);
        processPWD(i, command);
        processCD(i, command);
      }
      else if((n_pipes = processPipeAndStdin(command_token, i, command)) > 0) {
        i = i + n_pipes;
      }
      else if((n_pipes = processPipeAndStdout(command_token, i, command)) > 0) {
        i = i + n_pipes;
      }
      else if((n_pipes = processPipe(i, command)) > 0) {
        i = i + n_pipes;
      }
      else {
        processStdin(command_token, i, command);
        processStdout(command_token, i, command);
        executeCommand(i, command);
      }
      catchSigChld(); //claim zombie processes
    }
  }

  return 0;
}
