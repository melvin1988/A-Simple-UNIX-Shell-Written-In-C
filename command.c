/*
 * File:	command.c
 * Author:	Melvin Sim
 * Date:	25 Mar 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"

//Return 1 if the token is a command separator
//Return 0 otherwise
int separator(char *token) {
  int i = 0;
  char *commandSeparators[] = {pipeSep, conSep, seqSep, NULL};

  while(commandSeparators[i] != NULL) {
    if(strcmp(commandSeparators[i], token) == 0) {
      return 1;
    }
    i++;
  }

  return 0;
}

//Fill one command structure with the details
void fillCommandStructure(Command *cp, int first, int last, char *sep) {
  cp->first = first;
  cp->last = last - 1;
  cp->sep = sep;
}

//Assigns redirection file name if "<" or ">" is found
void searchRedirection(char *token[], Command *cp) {
  for(int i = cp->first; i <= cp->last + 1; i++) {
    if(strcmp(token[i], "<") == 0) { //if "<" found
      cp->stdin_file = token[i + 1]; //next token is assigned to stdin_file
    }
    else if(strcmp(token[i], ">") == 0) { //if ">" found
      cp->stdout_file = token[i + 1]; //next token is assigned to stdout_file
    }
  }
}

//Builds the command line argument vector for execvp function
void buildCommandArgumentArray(char *token[], Command *cp) {
  int n = (cp->last - cp->first + 1) + 1; //number of tokens in the command                                            //the element in argv must be a NULL

  //Re-allocate memory for argument vector
  cp->argv = (char **) realloc(cp->argv, sizeof(char *) * n);
  if(cp->argv == NULL) {
    perror("realloc");
    exit(1);
  }

  //Build the argument vector
  int i;
  int k = 0;
  for(i = cp->first; i <= cp->last; i++) {
    if(strcmp(token[i], ">") == 0 || strcmp(token[i], "<") == 0) {
      i++; //skip off the std in/out redirection
    }
    else {
      cp->argv[k] = token[i];
      k++;
    }
  }
  cp->argv[k] = NULL;
}

//Returns the number of commands
int separateCommands(char *token[], Command command[]) {
  int i;
  int nTokens;

  //Find out the number of tokens
  i = 0;
  while(token[i] != NULL) {
    i++;
  }
  nTokens = i;

  //If empty command line
  if(nTokens == 0) {
    return 0;
  }

  //Check the first token
  if(separator(token[0])) {
    return -3;
  }

  //Check last token, add ";" if necessary
  if(!separator(token[nTokens-1])) {
    token[nTokens] = seqSep;
    nTokens++;
  }

  int first = 0; //points to the first tokens of a command
  int last; //points to the last tokens of a command
  char *sep; //command separator at the end of a command
  int c = 0;
  for(i = 0; i < nTokens; i++) {
    last = i;
    if(separator(token[i])) {
      sep = token[i];
      if(first == last) { //two consecutive separators
        return -2;
      }
      fillCommandStructure(&(command[c]), first, last, sep);
      c++;
      first = i + 1;
    }
  }

  //Check the last token of the last command
  if(strcmp(token[last], pipeSep) == 0) { //last token is pipe separator
    return -4;
  }

  //Calculate the number of commands
  int nCommands = c;

  //Handle standard in/out redirection and build command line argument vector
  for(i = 0; i < nCommands; i++) {
    searchRedirection(token, &(command[i]));
    buildCommandArgumentArray(token, &(command[i]));
  }

  return nCommands;
}

//Initialises command[]
void initialiseCommand(Command command[]) {
  for(int i = 0; i < MAX_NUM_COMMANDS; i++) {
    command[i].first = 0;
    command[i].last = 0;
    command[i].sep = NULL;
    command[i].argv = NULL;
    command[i].stdin_file = NULL;
    command[i].stdout_file = NULL;
  }
}

//Prints each sequence of command
void printCommandSequence(char *token[], Command command[]) {
  for(int i = 0; i < separateCommands(token, command); i++) {
    printf("command %d: ", i + 1);
    for(int j = command[i].first; j <= command[i].last + 1; j++) {
      //For the last sequence, do not print if separator is ';'
      if(i == separateCommands(token, command) - 1 && strcmp(token[j], ";") == 0) {
        printf(" ");
      }
      else {
        printf("%s ", token[j]);
      }
    }
    printf("\n");
  }
}

//Prints the entire struct command
void printStructCommand(char *token[], Command command[]) {
  for(int i = 0; i < separateCommands(token, command); i++) {
    printf("\ncommand[%d].first = %d\n", i, command[i].first);
    printf("command[%d].last = %d\n", i, command[i].last);
    printf("command[%d].sep = %s\n", i, command[i].sep);
    printf("command[%d].stdin_file = %s\n", i, command[i].stdin_file);
    printf("command[%d].stdout_file = %s\n", i, command[i].stdout_file);
    for(int j = 0; j <= command[i].last - command[i].first + 1; j++) {
      printf("command[%d].argv[%d] = %s\n", i, j, command[i].argv[j]);
    }
  }
}
