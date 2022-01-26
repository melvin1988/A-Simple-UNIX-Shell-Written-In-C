/*
 * File:	token.h
 * Author:	Melvin Sim
 * Date:	25 Mar 2021
 */

#define MAX_NUM_TOKENS 1000

void initialiseToken(char *token[]);
int tokeniseWhiteSpace(char *input, char *token[]);
void printTokens(int n_tokens, char *token[]);
