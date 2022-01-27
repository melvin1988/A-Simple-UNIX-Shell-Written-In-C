# A-Simple-Unix-Shell-Written-In-C
Design and implement a simple UNIX shell program using the grammar specified in the later part of this section. Please allow for at least 100 commands in a command line and at least 1000 arguments in each command.

In addition to the above, the following are required:

1. Reconfigurable shell prompt (default %)
The shell must have a shell built-in command prompt for changing the current prompt. For example, type the following command
% prompt john$
should change the shell prompt to john$, i.e., the second token of the command.

2. The shell built-in command pwd
This command prints the current directory (also known as working directory) of the shell process.

3. Directory walk
This command is similar to that provided by the Bash built-in command cd. In particular, typing the command without a path should set the current directory of the shell to the home directory of the user.

4. Wildcard characters
If a token contains wildcard characters * or ? the token is treated as a filename. The wildcard characters in such a token indicate to the shell that the filename must be expanded. For example the command
% ls *.c
may be expanded to ls ex1.c ex2.c ex3.c if there are three matching files ex1.c ex2.c ex3.c in the current directory.
You may implement this feature using the C function glob.

5. Standard input and output redirections > and <
For example:
% ls -lt > foo
would redirect the standard output of process ls -lt to file foo. Similarly in the following command,
% cat < foo
the standard input of process cat is redirected to file foo.

6. Shell pipeline |
For example:
% ls -lt | more
the standard output of process ls -lt is connected to the standard input of process more via a pipe.

7. Background job execution
For example:
% xterm &
The commannd line starts command xterm in the background (i.e., the shell will not wait for the process to terminate and you can type in the next command immediately). The following command line
% sleep 20 & ps -l
starts command sleep 20 and immediately execute command ps -l without waiting for command sleep 20 to finish first.

8. Sequential job execution
For example the command line
% sleep 20 ; ps -l
starts command sleep 20 first, and wait for it to finish, then execute command ps -l.

9. The shell environment
The shell should inherit its environment from its parent process.

10. The shell built-in command exit
Use the built-in command exit to terminate the shell program.
The behaviour of the above commands (except prompt) should be as close to those of the Bash shell as possible. In addition, your shell should not be terminated by CTRL-C, CTRL-\, or CTRL-Z.

Finally you must not use any existing shell program to implement your shell (for example by calling a shell through the function system). That would defeat the purpose of this project.

In the above, commands such as ls, cat, grep, sleep, ps and xterm are used as examples to illustrate the use of your shell program. However your shell must be able to handle any command or executable program. Note the commands prompt, pwd, cd and exit should be implemented as shell builtins, not as external commands.

The syntax and behaviour of the built-in commands pwd, cd and exit should be similar to the corresponding commands under Bash shell.

A major part of this shell is a command line parser. Please read the this note for suggestions on implementing the parser.

Definition of Command Line Syntax

The following is the formal definition of the command line syntax for the shell, defined in Extended BNF:

< command line > ::= < job >
| < job > '&'
| < job > '&' < command line >
| < job > ';'
| < job > ';' < command line >
< job > ::= < command >
| < job > '|' < command >
< command > ::= < simple command >
| < simple command > '<' < filename >
| < simple command > '>' < filename >
< simple command > ::= < pathname >
| < simple command > < token >

An informal definition plus additional explanations of the syntax is given below:

1. A command line consists of one or several jobs separated by the special character "&" and/or ";". The last job may be followed by the character "&" or ";". If a job is followed by the character "&", then it should be executed in the background.

2. A job consists of one or more commands separated by pipeline characters "|";

3. A command is either a simple command or a simple command followed by an input redirection (< filename) or an output redirection (> filename);

4. A simple command consists of a single pathname followed by zero or more tokens;

5. The following five characters are the special characters: &, ;, |, < , > ;

6. The white space characters are defined to be the space character and the tab character;

7. A token is either a special character or a string that does not contain space characters or special characters. In this project we do not consider quoted strings. Therefore if single quote or double quote characters appear in a string, they are treated just like any other non-special characters without its usually special meaning;

8. Tokens must be separated by one or more white spaces;

9. A pathname is either a file name, or an absolute pathname, or a relative pathname. Examples of pathnames are grep, /usr/bin/grep, bin/grep and ./grep;

10. A command line must end with a newline character.
