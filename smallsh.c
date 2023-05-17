#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

//global "boolean" to keep track of whether & is registered (foreground only mode)
volatile sig_atomic_t foregroundOnlyBool = 0;

//struct for containing redirection information of each command
struct redirect
{
    int inBool;
    int outBool;
    char inFile[128];
    char outFile[128];
};

//function prototypes
void prompt(int* exitStatus, int* signalStatus, int* statusBool, pid_t* bgProcesses);
void insertPID(char* string);
void argsCreate(char* input, char* args[], int* numArgs);
void changeDir(char* args[]);
struct redirect checkRedirect(char* args[], int numArgs);
int checkBG(char* args[], int numArgs);

/**********************************************************************************
    ** Description: This is the "main" function of the program as it is called
    each time the user is prompted. It checks for background processes that have
    terminated, prompts the user for input, and then forks and executes the user's
    command if needed.
    ** Parameters: status booleans and background process ID array
**********************************************************************************/
void prompt(int* exitStatus, int* signalStatus, int* statusBool, pid_t* bgProcesses)
{
    //check for background processes termination
    for(int i = 0; i < 20; i++)
    {
        //don't call on waitpid on initialized values
        if(bgProcesses[i] == -1) {continue;}

        int bgExitMethod;

        //if current bg PID has terminated
        if(waitpid(bgProcesses[i], &bgExitMethod, WNOHANG) != 0)//non-waiting (returns 0 if PID still running)
        {
            //check exit status
            if(WIFSIGNALED(bgExitMethod) != 0)//non zero if terminated by signal
            {
                *statusBool = 1;
                *signalStatus = WTERMSIG(bgExitMethod);
                fflush(stdout);
                printf("background PID %d is done. terminated by signal %d\n", bgProcesses[i], *signalStatus);
            }
            else if(WIFEXITED(bgExitMethod) != 0)//non zero of terminated normally
            {
                *statusBool = 0;
                *exitStatus = WEXITSTATUS(bgExitMethod);
                fflush(stdout);
                printf("background PID %d is done. exit value: %d\n", bgProcesses[i], *exitStatus);                
            } 
            bgProcesses[i] = -1; //reset bgProcesses array
        }
    }

    char* line = NULL; //for user input in getline
    size_t buffer = 0; 
    char userInput[2048]; //max size of user input
    memset(userInput, '\0', sizeof(userInput));
    char* args[512]; //max of 512 arguments per line
    memset(args, 0, sizeof(args));
    int numArgs = -1; //number of arguments
    int bgBool = 0; //background process boolean
    
    //prompt for and obtain user input
    printf(": ");
    getline(&line, &buffer, stdin);
    
    //if a comment or empty line, reprompt
    if(line[0] == '#' || line[0] == '\n')
    {
        return;
    }

    //copy user input to max sized string of null terminators
    strcpy(userInput, line);
    free(line);
    line == NULL;
    //get rid of newline character from user's input
    userInput[strlen(userInput) - 1] = '\0';

    //if $$ is in user input, find and replace $$'s with PID
    insertPID(userInput);

    //create argument array from user input
    argsCreate(userInput, args, &numArgs);

    //if last element in array is & and foregroundOnlyBool is active
    if(args[numArgs - 1] != NULL && strcmp(args[numArgs - 1], "&") == 0 && foregroundOnlyBool == 1)
    {
        //remove & from args array
        args[numArgs - 1] = NULL;
        numArgs--;
    }

    //if user wants to exit smallsh
    if(strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }
    //if user wants to change directory
    else if(strcmp(args[0], "cd") == 0)
    {
        changeDir(args);
    }
    //status
    else if(strcmp(args[0], "status") == 0)
    {
        fflush(stdout);
        if(*statusBool == 0)//previous process terminated normally
        {
            printf("exit value %d\n", *exitStatus);
        }
        else if(*statusBool == 1)//previous process signaled
        {
            printf("terminated by signal %d\n", *signalStatus);
        }
    }

    //else, fork and execute command
    else
    {
        //check for background symbol at end
        int bgBool = checkBG(args, numArgs);
        //background present
        if(bgBool == 1){
            //decrement numArgs so we don't check null index in checkRedirect()
            numArgs--;
        }

        //fork a child process
        pid_t spawnPid = -5;
        spawnPid = fork();

        switch(spawnPid)
        {
            //fork failed
            case -1:
            {
                perror("Hull Breach!\n");
                exit(1);
            }

            //in child process
            case 0:
            {

                //handle SIGINT normally (terminate) in foreground child processes
                if (bgBool == 0)
                {
                    struct sigaction default_action = {0};
                    default_action.sa_handler = SIG_DFL;
                    default_action.sa_flags = 0;
                    sigaction(SIGINT, &default_action, NULL);
                }
                //ignore SIGTSTP in the child process
                struct sigaction childIgnore = {0};
                childIgnore.sa_handler = SIG_IGN;
                childIgnore.sa_flags = 0;
                sigaction(SIGTSTP, &childIgnore, NULL);

                //obtain any redirection, store in struct
                struct redirect redirStatus = checkRedirect(args, numArgs);
                
                //if redirecting input
                if(redirStatus.inBool)
                {
                    //open source file
                    int sourceFD = open(redirStatus.inFile, O_RDONLY);
                    if(sourceFD == -1)
                    {//cannot open file
                        perror("Input File"); 
                        exit(1); 
                    }

                    //redirect stdin to this source file descriptor
                    int result = dup2(sourceFD, 0);
                    if(result == -1)
                    {//dup failed
                        perror("source dup2()"); 
                        exit(2); 
                    }
                }
                //if input wasn't redirected, and background process, redirect to dev/null
                else if(bgBool == 1)
                {
                    //redirect stdin to /dev/null
                    int sourceFD = open("/dev/null", O_RDONLY);
                    if(sourceFD == -1)
                    {//cannot open file
                        perror("Input File"); 
                        exit(1); 
                    }
  
                    //redirect stdin to this source file descriptor
                    int result = dup2(sourceFD, 0);
                    if(result == -1)
                    { 
                        perror("source dup2()"); 
                        exit(2); 
                    }
                }

                //if redirecting output
                if(redirStatus.outBool)
                {
                    //open destination file
                    int destinationFD = open(redirStatus.outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if(destinationFD == -1)
                    {
                        perror("Output File"); 
                        exit(1); 
                    }
                                   
                    //redirect stdout to this destination file
                    int result = dup2(destinationFD, 1);
                    if(result == -1)
                    {//dup failed
                        perror("target dup2()"); 
                        exit(2); 
                    }
                }
                //if output wasn't redirected, and background process, redirect to dev/null
                else if(bgBool == 1)
                {
                    //redirect stdout to /dev/null
                    int destinationFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if(destinationFD == -1)
                    { 
                        perror("Output File"); 
                        exit(1); 
                    }
                    // printf("destinationFD == %d\n", destinationFD); // Written to terminal
                
                    //redirect stdout to this destination file
                    int result = dup2(destinationFD, 1);
                    if(result == -1)
                    { 
                        perror("target dup2()"); 
                        exit(2); 
                    }
                }

                //execute command
                if(execvp(args[0], args) == -1)
                {
                    //if execvp returns -1, command command was not found
                    fflush(stdout);
                    printf("%s: command not found\n", args[0]);
                    exit(1);
                }
                exit(1);
            }

            //in parent process
            default:
            {

                //if we just spawned a background process
                if(bgBool == 1)
                {
                    fflush(stdout);
                    printf("background PID started: %d\n", spawnPid);

                    //add child background PID to array
                    for(int i = 0; i < 20; i++)
                    {
                        if(bgProcesses[i] == -1)
                        {
                            bgProcesses[i] = spawnPid;
                            break;
                        }
                    }
                    //don't wait for child to die here
                    return;
                }
             
                //wait for child process to finish
                int childExitMethod;
                waitpid(spawnPid, &childExitMethod, 0);

                //check exit status
                if(WIFSIGNALED(childExitMethod) != 0) //non zero if terminated by signal
                {
                    *statusBool = 1;
                    *signalStatus = WTERMSIG(childExitMethod);
                    fflush(stdout);
                    printf("terminated by signal %d\n", *signalStatus);
                }
                else if(WIFEXITED(childExitMethod) != 0) //non zero of terminated normally
                {
                    *statusBool = 0;
                    *exitStatus = WEXITSTATUS(childExitMethod);
                    fflush(stdout);
                    // printf("exit status %d\n", *exitStatus);
                }                
            }
        }
    }
}

/**********************************************************************************
    ** Description: takes a string and replaces all instances of "$$" with the process
    ID of the small shell
    ** Parameters: string to insert PID into
**********************************************************************************/
void insertPID(char* string)
{

    //recursive base case if string has no consecutive dollar signs
    if(strstr(string, "$$") == NULL)
    {
        return;
    }

    for(int i = 0; i < strlen(string); i++)
    {
        //if "$$" is found and not beyond end of string
        if((string[i] == '$') && (string[i + 1] == '$') && (i + 1 < strlen(string)))
        {
            //duplicate string, and replace "$$" with "%d"
            char* dupe = strdup(string);
            dupe[i] = '%';
            dupe[i + 1] = 'd';

            //format the pid into the %d, and then copy back into string
            sprintf(string, dupe, getpid());
            free(dupe);
        }
    }

    //recursive call to check for more $$ in string
    insertPID(string);
}

/**********************************************************************************
    ** Description: takes the string that the user input on the command line, and
    tokenizes into an array of arguments delimited by spaces
    ** Parameters: string input from user, array of arguments to be filled & updated,
    and number of arguments to be updated
**********************************************************************************/
void argsCreate(char* input, char* args[], int* numArgs)
{
    char* saveptr;
    char* token;

    //get first token
    token = strtok_r(input, " ", &saveptr);

    int j = 0;
    //walk through other tokens
    while(token != NULL)
    {
        args[j] = token;
        j++;
        token = strtok_r(NULL, " ", &saveptr);
    }

    //update number of arguments
    *numArgs = j;
}

/**********************************************************************************
    ** Description: changes the current working directory to the directory specified,
    and if no directory is specified, changes to the HOME directory
    ** Parameters: arguments array
**********************************************************************************/
void changeDir(char* args[])
{
    //obtain home and current directories
    char* homeDir = getenv("HOME");
    char currDir[2048];
    memset(currDir, '\0', sizeof(currDir));
    getcwd(currDir, sizeof(currDir));

    //no second argument was provided. navigate to HOME directory
    if(args[1] == NULL)
    {
        //move to home directory
        chdir(homeDir);        
    }

    //path to navigate to was provided
    else
    {
        //move to directory, print corresponding error if chdir fails
        if(chdir(args[1]) == -1) 
        {
            perror("chdir");
        }
    }
}

/**********************************************************************************
    ** Description: takes the array of arguments and checks for redirection symbols.
    if found, fills a struct with the status of redirection and the filenames and 
    returns said struct.
    ** Parameters: argument array and number of arguments
**********************************************************************************/
struct redirect checkRedirect(char* args[], int numArgs)
{
    //initialize struct to be returned
    struct redirect redirStatus;
    redirStatus.inBool = 0, redirStatus.outBool = 0;
    memset(redirStatus.inFile, '\0', sizeof(redirStatus.inFile));
    memset(redirStatus.outFile, '\0', sizeof(redirStatus.outFile));

    //loop through args array, looking for redirection
    for(int i = 0; i < numArgs; i++)
    {
        //if input redirection "<"
        if(strcmp(args[i], "<") == 0)
        {
            //trigger inBool and remove symbol as an argument
            redirStatus.inBool = 1;
            args[i] = NULL;

            //check if there is a filename after that symbol
            if(args[i + 1] != NULL)
            {
                //copy filename into struct and remove filename as an argument
                strcpy(redirStatus.inFile, args[i + 1]);
                args[i + 1] = NULL;
                i++; //to prevent seg fault on next iteration
            }
            else
            {
                fflush(stdout);
                printf("No filename provided after redirection symbol.\n");
            }
        }

        //if output redirection ">"
        else if(strcmp(args[i], ">") == 0)
        {
            //trigger outBool and remove symbol as an argument
            redirStatus.outBool = 1;
            args[i] = NULL;

            //check if there is a filename after that symbol
            if(args[i + 1] != NULL)
            {
                //copy filename into struct and remove filename as an argument
                strcpy(redirStatus.outFile, args[i + 1]);
                args[i + 1] = NULL;
                i++; //to prevent seg fault on next iteration
            }
            else
            {
                fflush(stdout);
                printf("No filename provided after redirection symbol");
            }
        }
    }

    return redirStatus;
}

/**********************************************************************************
    ** Description: checks if & is the last argument in the array and returns 1 or
    0 to set a boolean where it was called
    ** Parameters: argument array and number of args
**********************************************************************************/
int checkBG(char* args[], int numArgs)
{
    //check if & is last argument
    if(args[numArgs - 1] != NULL && strcmp(args[numArgs - 1], "&") == 0)
    {
        //remove & from args array
        args[numArgs - 1] = NULL;
        return 1;
    }
    else
    {
        return 0;
    }
}

/**********************************************************************************
    ** Description: signal handler to toggle foreground-only mode on and off when
    SIGTSTP is received
**********************************************************************************/
void toggleForeground(int signum)
{
    //if foreground mode is off, turn it on
    if(foregroundOnlyBool == 0)
    {
        foregroundOnlyBool = 1;
        char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 52);
    }
    //if foreground mode is on, turn it off
    else
    {
        foregroundOnlyBool = 0;
        char* message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 32);
    }
}

/**********************************************************************************
    ** Description: main function that has certain certain variables, arrays, and
    signal handlers initialized
**********************************************************************************/
int main()
{
    //initialize variables and array
    int exitStatus = -1; //for holding the exit status of processes
    int signalStatus = -1; //for holding the termitating signal of processes
    int statusBool = -1; //for indicating last processes' terminating status
    pid_t bgProcesses[20];
    memset(bgProcesses, -1, sizeof(pid_t) * 20);

    //ignore SIGINT in shell
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignore_action, NULL);

    //custom SIGTSTP handler in shell
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = toggleForeground;
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    //continuously prompt user
    while(1)
    {
        fflush(stdout);
        prompt(&exitStatus, &signalStatus, &statusBool, bgProcesses);
    }
}