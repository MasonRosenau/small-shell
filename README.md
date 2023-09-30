<ins>Program Description</ins><br>
This program is a small, UNIX-like shell, completed for Operating Systems I (CS 344) at Oregon State University. Below are some of the capabilities.
1. Provide a prompt for running commands
2. Handle blank lines and comments, which are lines beginning with the # character
3. Provide expansion for the variable $$
4. Execute 3 commands exit, cd, and status via code built into the shell
5. Execute other commands by creating new processes using a function from the exec family of functions
6. Support input and output redirection
7. Support running commands in foreground and background processes
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP

<br><ins>Program Instructions</ins>
1. Compile using `gcc --std=gnu99 smallsh.c -o smallsh` which compiles the code using the GNU99 standard.
2. Run the program with `./smallsh`.

You can run `testscript.sh` to see the objectives of the program being completed in realtime by making it executable and running it like below, once smallsh is compiled.
- `chmod +x ./testscript.sh`
- `./testscript.sh`
