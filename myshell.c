/*
 * File:        myshell.c
 * Author:      Melvin Sim
 * Date:        25 Mar 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include "token.h"
#include "command.h"
#include "myshell.h"

#define STR_SIZE 1024

//Blocks SIGINT, SIGQUIT, SIGTSTP
void blockSignal() {
  sigset_t sigs;

  if(sigemptyset(&sigs) == 0) {
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGQUIT);
    sigaddset(&sigs, SIGTSTP);
  }
  sigprocmask(SIG_SETMASK, &sigs, NULL);
}

//Gets the user input
void getInput(char input[], char *prompt) {
  int again = 1;
  char *line_pt; //pointer to the line buffer

  while(again) {
    again = 0;
    printf("%s ", prompt);
    line_pt = fgets(input, STR_SIZE, stdin);
    if(input[strlen(input)-1] == '\n') {
      input[strlen(input)-1] = '\0';
    }
    if(line_pt == NULL) {
      if(errno == EINTR) {
        again = 1; //signal interruption, read again
      }
    }
  }
}

//Parses the input and fills up command
int parseCommand(char input[], char *token[], Command command[]) {
  initialiseToken(token);
  initialiseCommand(command);
  tokeniseWhiteSpace(input, token);
  int n_commands = separateCommands(token, command);

  return n_commands;
}

//Processes the command if argv[0] is "prompt"
char *processPrompt(char *prompt, char new_prompt[], int index, Command command[]) {
  if(strcmp(command[index].argv[0], "prompt") == 0) {
    //Has 2 args
    if(command[index].last == command[index].first + 1) {
      strcpy(new_prompt, command[index].argv[1]);
      prompt = new_prompt;
    }
    //Has 1 arg only
    else if(command[index].last == command[index].first) {
      printf("bash: prompt: missing argument\n");
    }
    //Has >2 args
    else {
      printf("bash: prompt: too many arguments\n");
    }
  }

  return prompt;
}

//Processes the command if argv[0] is "pwd"
void processPWD(int index, Command command[]) {
  char direct[STR_SIZE];

  if(strcmp(command[index].argv[0], "pwd") == 0) {
    printf("%s\n", getcwd(direct, sizeof(direct)));
  }
}

//Processes the command if argv[0] is "cd"
void processCD(int index, Command command[]) {
  char dir[STR_SIZE];
  char *filename_token[MAX_NUM_TOKENS];

  initialiseToken(filename_token);
  getFileNameToken(filename_token); //used for constraint checking to access child dir

  if(strcmp(command[index].argv[0], "cd") == 0) {
    //Has 1 arg only
    if(command[index].first == command[index].last) {
      chdir(getenv("HOME"));
      initialiseToken(filename_token);
      getFileNameToken(filename_token); //after chdir(), update filename_token
    }
    //Has 2 args AND argv[1] == "/tmp"
    else if(command[index].last == command[index].first + 1 && strcmp(command[index].argv[1], "/tmp") == 0) {
      chdir("/tmp");
      initialiseToken(filename_token);
      getFileNameToken(filename_token);
      getcwd(dir, sizeof(dir));
    }
    //Has 2 args AND cwd == /home AND argv[1] == username
    else if(command[index].last == command[index].first + 1 && strcmp(getcwd(dir, sizeof(dir)), "/home") == 0 && strcmp(command[index].argv[1], getenv("USER")) == 0) {
      chdir(getenv("HOME"));
      initialiseToken(filename_token);
      getFileNameToken(filename_token);
      getcwd(dir, sizeof(dir));
    }
    //Has 2 args AND cwd != /home
    else if(command[index].last == command[index].first + 1 && strcmp(getcwd(dir, sizeof(dir)), "/home") != 0) {
      //argv[1] == ".."
      if(strcmp(command[index].argv[1], "..") == 0) {
        chdir(".."); //navigate to parent dir
        initialiseToken(filename_token);
        getFileNameToken(filename_token);
        getcwd(dir, sizeof(dir));
      }
      //argv[1] == child dir AND is a dir file
      else if(isFileName(command[index].argv[1], filename_token) && isDirectory(command[index].argv[1])) {
        getcwd(dir, sizeof(dir)); //store current path in dir
        strcat(dir, "/");
        strcat(dir, command[index].argv[1]); //full path name
        chdir(dir);
        initialiseToken(filename_token);
        getFileNameToken(filename_token);
        getcwd(dir, sizeof(dir));
      }
      //arg[1] == child path AND is not a dir file
      else if(isFileName(command[index].argv[1], filename_token) && !isDirectory(command[index].argv[1])) {
        printf("bash: cd: %s: Not a directory\n", command[index].argv[1]);
      }
      else {
        printf("bash: cd: %s: No such file or directory\n", command[index].argv[1]);
      }
    }
    else {
      printf("bash: cd: %s: No such file or directory\n", command[index].argv[1]);
    }
  }
}

//Processes the command if sep is "|"
int processPipe(int index, Command command[]) {
  int n_pipes = 0;
  pid_t pid;
  int flag = 0;

  if(strcmp(command[index].argv[0], "exit") != 0 && strcmp(command[index].sep, "|") == 0) {
    //Get the number of pipes
    int temp = index;
    while(strcmp(command[temp].sep, "|") == 0) {
      temp++;
    }
    n_pipes = temp - index;

    //Create pipes
    int p[n_pipes * 2];
    for(int i = 0; i < n_pipes * 2; i+=2) {
      if(pipe(p + i) == -1) {
        perror("pipe");
        exit(1);
      }
    }

    //Create first child process
    if((pid = fork()) == 0) {
      dup2(p[1], STDOUT_FILENO); //replace stdout with first pipe write
      for(int i = 0; i < n_pipes * 2; i++) {
        close(p[i]);
      }
      if(isWildCard(index, command) != -1) { //-1 means no wildcard, any int >= 0 means wildcard present
        int num_of_wildcard_tokens = numOfWildCardFiles(command[index].argv[isWildCard(index, command)]);
        char *argv[command[index].last - command[index].first + 1 + num_of_wildcard_tokens];
        getArgvForWildCard(index, command, argv);
        execvp(argv[0], argv);
      }
      else {
        char *argv[command[index].last - command[index].first + 2];
        getArgvForExecuteCommand(index, command, argv);
        execvp(argv[0], argv);
      }
      perror("execvp");
      exit(1);
    }
    //Create subsequent child processes
    for(int i = 0; i < n_pipes - 1; i++) {
      if((pid = fork()) == 0) {
        dup2(p[i * 2], STDIN_FILENO); //replace stdin with pipe read
        dup2(p[i * 2 + 3], STDOUT_FILENO); //replace stdout with pipe write
        for(int j = 0; j < n_pipes * 2; j++) {
          close(p[j]);
        }
        char *argv[command[index + i + 1].last - command[index + i + 1].first + 2];
        getArgvForExecuteCommand(index + i + 1, command, argv);
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
      }
    }
    //Create last child process
    if((pid = fork()) == 0) {
      dup2(p[n_pipes * 2 - 2], STDIN_FILENO); //replace stdin with last pipe read
      for(int i = 0; i < n_pipes * 2; i++) {
        close(p[i]);
      }
      char *argv[command[index + n_pipes].last - command[index + n_pipes].first + 2];
      getArgvForExecuteCommand(index + n_pipes, command, argv);
      execvp(argv[0], argv);
      perror("execvp");
      exit(1);
    }

    //Parent process
    for(int i = 0; i < n_pipes * 2; i++) {
      close(p[i]);
    }
    for(int i = 0; i < n_pipes + 1; i++) {
      wait(NULL);
    }
    flag = n_pipes;
  }

  return flag;
}

//Processes the command if stdin_file is not null
void processStdin(char *command_token[], int index, Command command[]) {
  int stdin_index;
  int stdin_fd;

  if(strcmp(command[index].argv[0], "exit") != 0 && command[index].stdin_file != NULL) {
    //Get stdin_index
    for(int i = command[index].first; i <= command[index].last; i++) {
      if(strcmp(command_token[i], "<") == 0) {
        stdin_index = i;
      }
    }
    //Number of args after "<" = 1
    if(command[index].last - stdin_index == 1) {
      stdin_fd = open(command[index].stdin_file, O_RDONLY);
      //Create child process
      pid_t pid;
      if((pid = fork()) < 0) {
        perror("fork");
        exit(1);
      }
      //Child process
      if(pid == 0) {
        dup2(stdin_fd, STDIN_FILENO); //replace stdin with file read
        close(stdin_fd);
        if(isWildCardForStdinStdout(index, command) != -1) {
          int num_of_wildcard_tokens = numOfWildCardFiles(command[index].argv[isWildCardForStdinStdout(index, command)]);
          char *argv[command[index].last - command[index].first - 1 + num_of_wildcard_tokens];
          getArgvForWildCardStdinStdout(index, command, argv);
          execvp(argv[0], argv);
        }
        else {
          char *argv[command[index].last - command[index].first];
          getArgvForStdinStdout(index, command, argv);
          execvp(argv[0], argv);
        }
        perror("execvp");
        exit(1);
      }
      //Parent process
      close(stdin_fd);
      wait(NULL);
    }
    //Missing arg after "<"
    else if(command[index].last - stdin_index == 0) {
      printf("Missing argument after <\n");
    }
    //Too many args after "<"
    else {
      printf("Too many arguments after <\n");
    }
  }
}

//Processes the command if stdout_file is not null
void processStdout(char *command_token[], int index, Command command[]) {
  int stdout_index;
  int stdout_fd;

  if(strcmp(command[index].argv[0], "exit") != 0 && command[index].stdout_file != NULL) {
    //Get stdout_index
    for(int i = command[index].first; i <= command[index].last; i++) {
      if(strcmp(command_token[i], ">") == 0) {
        stdout_index = i;
      }
    }
    //Number of args after ">" = 1
    if(command[index].last - stdout_index == 1) {
      stdout_fd = open(command[index].stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0664);
      //Create child process
      pid_t pid;
      if((pid = fork()) < 0) {
        perror("fork");
        exit(1);
      }
      //Child process
      if(pid == 0) {
        dup2(stdout_fd, STDOUT_FILENO); //replace stdout with file write
        close(stdout_fd);
        if(isWildCardForStdinStdout(index, command) != -1) {
          int num_of_wildcard_tokens = numOfWildCardFiles(command[index].argv[isWildCardForStdinStdout(index, command)]);
          char *argv[command[index].last - command[index].first - 1 + num_of_wildcard_tokens];
          getArgvForWildCardStdinStdout(index, command, argv);
          execvp(argv[0], argv);
        }
        else {
          char *argv[command[index].last - command[index].first];
          getArgvForStdinStdout(index, command, argv);
          execvp(argv[0], argv);
        }
        perror("execvp");
        exit(1);
      }
      //Parent process
      close(stdout_fd);
      wait(NULL);
    }
    //Missing arg after ">"
    else if(command[index].last - stdout_index == 0) {
      printf("Missing argument after >\n");
    }
    //Too many args after ">"
    else {
      printf("Too many arguments after >\n");
    }
  }
}

//Processes the command if "|" is present and stdin file is not null
int processPipeAndStdin(char *command_token[], int index, Command command[]) {
  int n_pipes = 0;
  int stdin_index;
  int stdin_fd;
  int flag = 0;

  //Must have at least a "|" present
  if(strcmp(command[index].argv[0], "exit") != 0 && strcmp(command[index].sep, "|") == 0) {
    //Get the number of pipes
    int temp = index;
    while(strcmp(command[temp].sep, "|") == 0) {
      temp++;
    }
    n_pipes = temp - index;
  }

  //Check that stdin file is not null
  if(command[index + n_pipes].stdin_file != NULL) {
    //Get stdin_index
    for(int i = command[index + n_pipes].first; i <= command[index + n_pipes].last; i++) {
      if(strcmp(command_token[i], "<") == 0) {
        stdin_index = i;
      }
    }
    //Number of args after "<" = 1
    if(command[index + n_pipes].last - stdin_index == 1) {
      stdin_fd = open(command[index + n_pipes].stdin_file, O_RDONLY);
      //Create child process
      pid_t pid;
      if((pid = fork()) < 0) {
        perror("fork");
        exit(1);
      }
      //Child process
      if(pid == 0) {
        dup2(stdin_fd, STDIN_FILENO);
        close(stdin_fd);
        processPipe(index, command);
        exit(0);
      }
      //Parent process
      close(stdin_fd);
      wait(NULL);
    }
    //Missing arg after "<"
    else if(command[index + n_pipes].last - stdin_index == 0) {
      printf("Missing argument after <\n");
    }
    //Too many args after "<"
    else {
      printf("Too many arguments after <\n");
    }
    flag = n_pipes;
  }

  return flag;
}

//Processes the command if "|" is present and stdout file is not null
int processPipeAndStdout(char *command_token[], int index, Command command[]) {
  int n_pipes;
  int stdout_index;
  int stdout_fd;
  int flag = 0;

  //Must have at least a "|" present
  if(strcmp(command[index].argv[0], "exit") != 0 && strcmp(command[index].sep, "|") == 0) {
    //Get the number of pipes
    int temp = index;
    while(strcmp(command[temp].sep, "|") == 0) {
      temp++;
    }
    n_pipes = temp - index;
  }

  //Check that stdout file is not null
  if(command[index + n_pipes].stdout_file != NULL) {
    //Get stdout_index
    for(int i = command[index + n_pipes].first; i <= command[index + n_pipes].last; i++) {
      if(strcmp(command_token[i], ">") == 0) {
        stdout_index = i;
      }
    }
    //Number of args after ">" = 1
    if(command[index + n_pipes].last - stdout_index == 1) {
      stdout_fd = open(command[index + n_pipes].stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0664);
      //Create child process
      pid_t pid;
      if((pid = fork()) < 0) {
        perror("fork");
        exit(1);
      }
      //Child process
      if(pid == 0) {
        dup2(stdout_fd, STDOUT_FILENO);
        close(stdout_fd);
        processPipe(index, command);
        exit(0);
      }
      //Parent process
      close(stdout_fd);
      wait(NULL);
    }
    //Missing arg after ">"
    else if(command[index + n_pipes].last - stdout_index == 0) {
      printf("Missing argument after >\n");
    }
    //Too many args after ">"
    else {
      printf("Too many arguments after >\n");
    }
    flag = n_pipes;
  }

  return flag;
}

//Executes all Unix commands
void executeCommand(int index, Command command[]) {
  if(strcmp(command[index].argv[0], "exit") != 0) {
    pid_t pid;
    if((pid = fork()) < 0) {
      perror("fork");
      exit(1);
    }
    //Child process
    if(pid == 0) {
      if(isWildCard(index, command) != -1) { //-1 means no wildcard, any int >= 0 means wildcard present
        int num_of_wildcard_tokens = numOfWildCardFiles(command[index].argv[isWildCard(index, command)]);
        char *argv[command[index].last - command[index].first + 1 + num_of_wildcard_tokens];
        getArgvForWildCard(index, command, argv);
        execvp(argv[0], argv);
      }
      else {
        char *argv[command[index].last - command[index].first + 2];
        getArgvForExecuteCommand(index, command, argv);
        execvp(argv[0], argv);
      }
      perror("execvp");
      exit(1);
    }
    //Parent process
    if(strcmp(command[index].sep, "&") != 0) {
      wait(NULL);
    }
  }
}

//Catch SIGCHLD to remove zombies from the system
void catchSigChld() {
  struct sigaction act;

  act.sa_handler = claimChildren;
  sigemptyset(&act.sa_mask); //not to block other signals
  act.sa_flags = SA_NOCLDSTOP; //not to catch stopped children
  sigaction(SIGCHLD,(struct sigaction *)&act,(struct sigaction *)0);
}

int builtInCommand(int index, Command command[]) {
  int flag = 0;

  if(strcmp(command[index].argv[0], "exit") != 0 && (strcmp(command[index].argv[0], "prompt") == 0 || strcmp(command[index].argv[0], "pwd") == 0 || strcmp(command[index].argv[0], "cd") == 0)) {
    flag = 1;
  }

  return flag;
}

//Gets the file names from the current working directory
void getFileNameToken(char *filename_token[]) {
  struct dirent *de;
  DIR *dp = opendir(".");

  //Read dir and store file names in filename[]
  int num_of_files = 0;
  while((de = readdir(dp)) != NULL) {
    filename_token[num_of_files] = de->d_name;
    num_of_files++;
  }
  closedir(dp);
}

//Returns 0 if name does not exist or 1 if name exists
int isFileName(char *input, char *filename[]) {
  int exist = 0;
  int i = 0;

  while(filename[i] != NULL) {
    if(strcmp(filename[i], input) == 0) {
      exist = 1;
      break;
    }
    i++;
  }

  return exist;
}

//Returns 0 if not dir file or 1 if dir file
int isDirectory(char *filename) {
  struct stat buf;

  if(lstat(filename, &buf) != 0) {
    return 0;
  }

  return S_ISDIR(buf.st_mode);
}

//Returns -1 if the sequence of command does not contain a wildcard or the index of command->argv if it contains a wildcard
int isWildCard(int index, Command command[]) {
  int flag = -1;

  for(int i = 0; i <= command[index].last - command[index].first; i++) {
    if(strchr(command[index].argv[i], '*') != NULL || strchr(command[index].argv[i], '?') != NULL) {
      flag = i;
    }
  }

  return flag;
}

//Returns -1 if the sequence of command does not contain a wildcard or the index of command->argv if it contains a wildcard for stdin/stdout
int isWildCardForStdinStdout(int index, Command command[]) {
  int flag = -1;

  for(int i = 0; i <= command[index].last - command[index].first - 2; i++) {
    if(strchr(command[index].argv[i], '*') != NULL || strchr(command[index].argv[i], '?') != NULL) {
      flag = i;
    }
  }

  return flag;
}

//Returns the number of wildcard files
int numOfWildCardFiles(char *input) {
  glob_t glob_buf;

  glob(input, 0, NULL, &glob_buf);
  globfree(&glob_buf);

  return glob_buf.gl_pathc;
}

//Stores the wildcard file names in char *token[]
void expandWildCard(char *input, char *token[]) {
  glob_t glob_buf;

  glob(input, 0, NULL, &glob_buf);
  for(int i = 0; i < glob_buf.gl_pathc; i++) {
    token[i] = glob_buf.gl_pathv[i];
  }
}

//Gets char *argv[] for processStdinStdout()
void getArgvForStdinStdout(int index, Command command[], char *argv[]) {
  for(int i = 0; i <= command[index].last - command[index].first - 2; i++) {
    argv[i] = command[index].argv[i];
  }
  argv[command[index].last - command[index].first - 1] = NULL; //assign NULL to the last element
}

//Gets char *argv[] for executeCommand()
void getArgvForExecuteCommand(int index, Command command[], char *argv[]) {
  for(int i = 0; i <= command[index].last - command[index].first; i++) {
    argv[i] = command[index].argv[i];
  }
  argv[command[index].last - command[index].first + 1] = NULL; //assign NULL to the last element
}

//Gets char *argv[] if wildcard is present
void getArgvForWildCard(int index, Command command[], char *argv[]) {
  int wildcard_index;
  char *wildcard_token[MAX_NUM_TOKENS];
  int num_of_wildcard_tokens;

  for(int i = 0; i <= command[index].last - command[index].first; i++) {
      //Get wildcard_index
      if(strchr(command[index].argv[i], '*') != NULL || strchr(command[index].argv[i], '?') != NULL) {
        wildcard_index = i;
        initialiseToken(wildcard_token);
        num_of_wildcard_tokens = numOfWildCardFiles(command[index].argv[i]);
        expandWildCard(command[index].argv[i], wildcard_token); //store all expanded filenames in wildcard_token[]
      }
    }

  //Fill char *argv[] before wildcard_index
  for(int j = 0; j < wildcard_index; j++) {
    argv[j] = command[index].argv[j];
  }

  //Fill char *argv[] from wildcard_index
  for(int k = 0; k < num_of_wildcard_tokens; k++) {
    argv[wildcard_index + k] = wildcard_token[k];
  }
  argv[command[index].last - command[index].first + num_of_wildcard_tokens] = NULL; //assign NULL to the last element
}

//Gets char *argv[] if wildcard is present
void getArgvForWildCardStdinStdout(int index, Command command[], char *argv[]) {
  int wildcard_index;
  char *wildcard_token[MAX_NUM_TOKENS];
  int num_of_wildcard_tokens;

  for(int i = 0; i <= command[index].last - command[index].first - 2; i++) {
      //Get wildcard_index
      if(strchr(command[index].argv[i], '*') != NULL || strchr(command[index].argv[i], '?') != NULL) {
        wildcard_index = i;
        initialiseToken(wildcard_token);
        num_of_wildcard_tokens = numOfWildCardFiles(command[index].argv[i]);
        expandWildCard(command[index].argv[i], wildcard_token); //store all expanded filenames in wildcard_token[]
      }
    }

  //Fill char *argv[] before wildcard_index
  for(int j = 0; j < wildcard_index; j++) {
    argv[j] = command[index].argv[j];
    argv[j] = command[index].argv[j];
  }

  //Fill char *argv[] from wildcard_index
  for(int k = 0; k < num_of_wildcard_tokens; k++) {
    argv[wildcard_index + k] = wildcard_token[k];
  }
  argv[command[index].last - command[index].first - 2 + num_of_wildcard_tokens] = NULL; //assign NULL to the last element
}

//Claims zombie processes
void claimChildren() {
  pid_t pid = 1;

  while(pid > 0) {
    pid = waitpid(0, (int *)0, WNOHANG);
  }
}
