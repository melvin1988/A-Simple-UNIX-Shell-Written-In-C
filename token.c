/*
 * File:	token.c
 * Author:	Melvin Sim
 * Date:	25 Mar 2021
 */

#include <stdio.h>
#include <string.h>
#include "token.h"

//Initialises char *token[]
void initialiseToken(char *token[]) {
  for(int i = 0; i < MAX_NUM_TOKENS; i++) {
    token[i] = NULL;
  }
}

//Splits a string by whitespace " " and "\t"
int tokeniseWhiteSpace(char *input, char *token[]) {
  char *tok = strtok(input, " ,\t");
  int n_tokens = 0;
  while(tok != NULL) {
    token[n_tokens] = tok;
    tok = strtok(NULL, " ,\t");
    n_tokens++;
  }

  if(n_tokens > MAX_NUM_TOKENS) {
    return -1;
  }

  return n_tokens;
}

//Prints out each token in char *token[]
void printTokens(int n_tokens, char *token[]) {
  for(int i = 0; i < n_tokens; i++) {
    printf("%2d: %s\n", i + 1, token[i]);
  }
}
