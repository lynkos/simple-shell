/*
* Name: Kiran Brahmatewari
* PID: 5467937
*
* I affirm that I wrote this program myself without any help
* from any other people or sources from the internet
*
* This program is an extension of myshell.c with pipelines
* + I/O redirection functionality. It can do the following:
*
* 1. > Redirect standard output from a command to a file; if the
* file already exists, it will be erased and overwritten without warning
*
* 2. >> Append standard output from a command to a file if
* the file exists, else create a new one
*
* 3. < Redirect standard input from a file to a command
*
* 4. | Pass the standard output of one command to another
* for further processing
*/
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_ARGS 20
#define BUFSIZ 1024
#define READ 0
#define WRITE 1

/* Function prototypes */
int get_nargs(char *cmdline, char *args[]);
void execute(char *cmdline);

int get_nargs(char *cmdline, char *args[]) {
  int i = 0;

  /* if no args */
  if((args[0] = strtok(cmdline, "\n\t ")) == NULL) 
    return 0; 

  while((args[++i] = strtok(NULL, "\n\t ")) != NULL) {
    if(i >= MAX_ARGS) {
      printf("too many args\n");
      exit(1);
    }
  }

  return i;
}

void execute(char *cmdline) {
  /* array of args */
  char *argv[MAX_ARGS];
  int nargs = get_nargs(cmdline, argv);
  
  /* exit if no args */
  if(nargs <= 0 || cmdline == NULL) {
    perror("not enough args");
    exit(1);
  }

  /* check if user wants to exit/quit */
  if(!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit"))
    exit(0);
  
  /* check if async call */
  bool async = false;
  if(!strcmp(argv[nargs - 1], "&")) {
    async = true;
    argv[--nargs] = 0;
  }
  
  /* vars for piping */
  unsigned int count = 0, pipes = 0;
  int left_pipe[2], right_pipe[2];
  /* array of arg locations in argv */
  int arg_locations[MAX_ARGS] = { 0 };
  /* vars for redirection */
  char *in_path, *out_path, *app_path;
  bool redirect_input = false, redirect_output = false, append_stdout = false;
  /* manage redirection */
  while(argv[count] != 0) {
    if(!strcmp(argv[count], "|")) { /* pipe */
      argv[count] = 0;
      arg_locations[pipes + 1] = count + 1;
      ++pipes;
    }

    else if(!strcmp(argv[count], "<")) { /* redirect input */
      in_path = strdup(argv[count + 1]);
      argv[count] = 0;
      redirect_input = true;
    }

    else if(!strcmp(argv[count], ">")) { /* redirect output */
      out_path = strdup(argv[count + 1]);
      argv[count] = 0;
      redirect_output = true;
    }

    else if(!strcmp(argv[count], ">>")) { /* append output */
      app_path = strdup(argv[count + 1]);
      argv[count] = 0;
      append_stdout = true;
    }
    
    else
      arg_locations[count] = count;

    ++count;
  }

  /* command(s) execution */
  pid_t pid = 0;
  for(unsigned int index = 0; index <= pipes; ++index) {
    if(pipes > 0 && index != pipes) /* if command(s) contain(s) pipe(s) */
      pipe(right_pipe);

    switch(pid = fork()) {
      case -1: /* error occurred */
        perror("fork failed");
        exit(1);

      case 0: /* child process */
        /* input redirection */
        if(index == 0 && redirect_input == true) {
          int input_file_desc = open(in_path, O_RDONLY, 0400);
          if(input_file_desc == -1) {
            perror("file failed to open");
            exit(1);
          }
          close(READ);
          dup(input_file_desc);
          close(input_file_desc);
        }

        /* output redirection */
        if(index == pipes && redirect_output == true) {
          int output_file_desc = creat(out_path, 0700);
          if(output_file_desc < 0) {
            perror("file failed to open");
            exit(1);
          }
          close(WRITE);
          dup(output_file_desc);
          close(output_file_desc);
        }

        /* append output */
        if(index == pipes && append_stdout == true) {
          int append_file_desc = open(app_path, O_WRONLY | O_CREAT | O_APPEND, 0700);
          if(append_file_desc < 0) {
            perror("file failed to open");
            exit(1);
          }
          close(WRITE);
          dup(append_file_desc);
          close(append_file_desc);
        }

        /* manage pipes */
        if(pipes > 0) {
          if(index == 0) { /* first child process */
            close(WRITE);
            dup(right_pipe[WRITE]);
            close(right_pipe[WRITE]);
            close(right_pipe[READ]);
          }

          else if(index < pipes) { /* in-between child process */
            close(READ);
            dup(left_pipe[READ]);
            close(left_pipe[READ]);
            close(left_pipe[WRITE]);
            close(WRITE);
            dup(right_pipe[WRITE]);
            close(right_pipe[READ]);
            close(right_pipe[WRITE]);
          }

          else { /* final child process */
            close(READ);
            dup(left_pipe[READ]);
            close(left_pipe[READ]);
            close(left_pipe[WRITE]);
          }
        }

        execvp(argv[arg_locations[index]], &argv[arg_locations[index]]);
        /* return only when exec fails */
        perror("exec failed");
        exit(-1);

      default: /* parent process */
        if(index > 0) {
          close(left_pipe[READ]);
          close(left_pipe[WRITE]);
        }
        
        left_pipe[READ] = right_pipe[READ];
        left_pipe[WRITE] = right_pipe[WRITE];

        /* parent waits for child process to complete */
        if(!async) waitpid(pid, NULL, 0);
        else printf("this is an async call\n");
    }
  }
}

int main(int argc, char *argv[]) {
  char *parser, cmdline[BUFSIZ];

  while(!feof(stdin)) {
    /* begin parsing at beginning of cmdline */
    parser = cmdline;
    printf("COP4338$ ");

    /* read in cmdline and exit if fails */
    if(fgets(parser, BUFSIZ, stdin) == NULL) {
      perror("fgets failed");
      exit(1);
    }

    /* remove newline char */
    if(cmdline[strlen(cmdline) - 1] == '\n')
      cmdline[strlen(cmdline) - 1] = '\0';
    
    execute(parser);
  }

  return 0;
}