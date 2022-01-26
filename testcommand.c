/*
 * File:	test_command.c
 * Author:	Melvin Sim
 * Date:	25 Mar 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include "token.h"

#define MAX_NUM_TOKENS 1000
#define STR_SIZE 1024

int main() {
  char *inputLine = malloc(sizeof(char) * STR_SIZE);
  Command command[MAX_NUM_COMMANDS];
  char *token[MAX_NUM_TOKENS];
  int n_tokens;

  //Get user input, display user input
  printf("$ ");
  fgets(inputLine, STR_SIZE, stdin);
  printf("You entered: %s\n", inputLine);

  //Split user input, display each token
  n_tokens = tokeniseWhiteSpace(inputLine, token);
  if(n_tokens > 0) {
    printf("The %d tokens are:\n", n_tokens);
    printTokens(n_tokens, token);
  }
  else {
    printf("Error: token array too small\n");
  }

  //Initialise command
  initialiseCommand(command);

  //Display the number of commands
  printf("\nNumber of commands: %d\n", separateCommands(token, command));

  //Display each sequence of command
  printCommandSequence(token, command);

  //Check results
  printStructCommand(token, command);

  return 0;
}
