This program is an extension of myshell.c with pipelines + I/O redirection functionality. It can do the following:

1. > Redirect standard output from a command to a file; if the file already exists, it will be erased and overwritten without warning
2. >> Append standard output from a command to a file if the file exists, else create a new one
3. < Redirect standard input from a file to a command
4. | Pass the standard output of one command to another for further processing
