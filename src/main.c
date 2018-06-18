#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include "sfish.h"
#include "debug.h"
#include <signal.h>
#include <fcntl.h>




char *oldPWD = "NULL";
//signal(SIGCHLD, sigchld_handler);
volatile sig_atomic_t pid;
int main(int argc, char *argv[], char* envp[]) {
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, siginit_handler);
    signal(SIGTSTP, sigstp_handler);
    //signal(SIGTSTP, sigstp_handler);
    //char jobs*[1000];
    //int numjobs = 0;
    char* input;
    int print = 0;
    int color = -1;
    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
        else{
            print = 1;
        }
    }
    while(1) {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask,SIGCHLD);
        sigaddset(&mask,SIGINT);
        sigaddset(&mask,SIGTSTP);
        sigprocmask(SIG_BLOCK,&mask,NULL);
        char *input_file;
        char *output_file;
        int OR = 0;
        int IR = 0;
        //int saved_stdin = dup(STDIN_FILENO);
        //int saved_stdout = dup(STDOUT_FILENO);
        char *prompt = getPrompt(color);
        if (print ==1){
            input = readline(NULL);
        }
        else{
            input = readline(prompt);
        }
        //input = readline(prompt);
        if (input == NULL || isAllSpace(input) == 0)
            continue;
        cleanInput(&input);
        cleanInput(&input);
        int charCount = countChars(input,' ') + 2;
        int inputCount = countChars(input,'<');
        int outputCount = countChars(input,'>');
        int pipeCount = countChars(input,'|');
        if (outputCount > 1){
            printf(SYNTAX_ERROR,"Too many >");
            continue;
        }
        if (inputCount > 1){
            printf(SYNTAX_ERROR,"Too many <");
            continue;
        }

        char *temp = input;
        char *inputArray[charCount];
        int goesUpTo = get_args(input, inputArray);
        int valid = checkSyntax(inputArray,goesUpTo);

        if (valid == -1){
            continue;
        }
        int result = getRedirection(inputArray, &goesUpTo, &input_file, &output_file, &IR, &OR);
        //struct job_list[2];
        if (result == -1){
            continue;
        }
        if (pipeCount > 0){
            execute_pipes(inputArray,goesUpTo,pipeCount,temp);
        }
        else{
        // If EOF is read (aka ^D) readline returns NULL
        if (strcmp(inputArray[0],"exit") == 0){
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(inputArray[0],"cd") == 0){
            char *operation = inputArray[1];
            cdFunction(operation);
        }
        else if (strcmp(inputArray[0],"pwd") == 0){
            pwdFunction(inputArray);
        }
        else if (strcmp(inputArray[0],"color") == 0){
            if (inputArray[1] == NULL){
                printf(BUILTIN_ERROR,"No color");
            }
            else{
                color = colorFunction(inputArray[1]);
            }
        }
        else if (strcmp(inputArray[0],"jobs") == 0){

        }
        else if (strcmp(inputArray[0],"fg") == 0){
        }
        else if (strcmp(inputArray[0],"kill") == 0){
        }
        else{
            checkExec(inputArray,charCount,input_file,output_file,IR,OR);
        }
        rl_free(input);
    }
    }
}
int colorFunction(char *color){
    if (strcmp(color,"RED") == 0){
        return 0;
    }
    else if (strcmp(color,"GRN") == 0){
        return 1;
    }
    else if (strcmp(color,"YEL") == 0){
        return 2;
    }
    else if (strcmp(color,"BLU") == 0){
        return 3;
    }
    else if (strcmp(color,"MAG") == 0){
        return 4;
    }
    else if (strcmp(color,"CYN") == 0){
        return 5;
    }
    else if (strcmp(color,"WHT") == 0){
        return  6;
    }
    else if (strcmp(color,"BWN") == 0){
        return 7;
    }
    else{
        printf(BUILTIN_ERROR,"NOT AN OPTION");
    }
    return -1;
}
void execute_pipes(char *inputArray[],int num_args, int num_pipes, char *input){
    signal(SIGCHLD, sigchld_handler);
    //sigset_t prev, mask;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGCHLD);
    sigaddset(&mask,SIGTSTP);
    sigaddset(&mask,SIGINT);
    sigprocmask(SIG_BLOCK,&mask,NULL);
    int num_commands = num_pipes + 1;
    int pipe1[2]; // pos. 0 output, pos. 1 input of the pipe
    int pipe2[2];

    char *charsArray[num_commands][num_args];
    for (int i = 0; i < num_commands; i++) {
        for (int j = 0; j < num_args; j++) {
            charsArray[i][j] = NULL;
        }
    }

    int cmdCount = 0;
    int argIndex = 0;
    for (int i = 0;i < num_args; i++){
        char *inputArg = inputArray[i];
        if (strcmp(inputArg,"|") == 0 || inputArg == NULL){
            charsArray[cmdCount][argIndex] = NULL;
            cmdCount++;
            argIndex = 0;
        }
        else if (inputArg != NULL){
            charsArray[cmdCount][argIndex] = inputArg;
            argIndex++;
        }
    }

    for (int i = 0; i < num_commands ;i++){
        char* inputs[num_args];
        for (int z = 0; z < num_args; z++) {
            char *insert = charsArray[i][z];
            if (insert == NULL){
                inputs[z] = NULL;
                break;
            }
            else{
                inputs[z] = insert;
            }

        }

        if (i % 2 != 0){
            pipe(pipe1);
        }
        else{
            pipe(pipe2);
        }
        char *firstArg = inputs[0];
        char *temp = firstArg;
        if (countChars(firstArg,'/') == 0)
            firstArg = concat("/bin/",firstArg);
            struct stat fileStat;
            int statID = stat(firstArg, &fileStat);
            if (statID == 0){
                int pid = fork();
                if (pid == 0){
                    //Child
                    sigemptyset(&mask);//No Signals
                    sigaddset(&mask, SIGCHLD); //Add SIGNAL TO SET
                    sigaddset(&mask,SIGINT);
                    sigaddset(&mask,SIGTSTP);
                    sigprocmask(SIG_UNBLOCK, &mask, NULL);//UNBLOCK SIGCHLD

                    if (i == 0){
                        dup2(pipe2[1],STDOUT_FILENO);
                    }

                    else if (i == num_commands -1){
                        if (num_args % 2 != 0){
                            dup2(pipe1[0],STDIN_FILENO);
                        }
                        else{
                            dup2(pipe2[0],STDIN_FILENO);
                        }
                    }

                    else{
                        if (i % 2 != 0){
                            dup2(pipe2[0],STDIN_FILENO);
                            dup2(pipe1[1],STDOUT_FILENO);
                        }
                        else{
                            dup2(pipe1[0],STDIN_FILENO);
                            dup2(pipe2[1],STDOUT_FILENO);
                        }
                    }

                    if(execv(firstArg, inputs) < 0){
                        printf(EXEC_ERROR,firstArg);
                        exit(EXIT_FAILURE);
                    }
                }
                else if(pid < 0){
                    if (i != num_commands - 1){
                        if (i % 2 != 0){
                            close(pipe1[1]); // for odd i
                        }
                        else{
                            close(pipe2[1]); // for even i
                        }
                    }
                    printf(EXEC_ERROR,"Can't fork");
                    return;
                }
                else{
                    //Parent
                    signal(SIGCHLD, sigchld_handler);
                    //pid = 0;
                    //while (!pid){
                        //sigsuspend(&prev);
                    //}
                    if (i == 0){
                        close(pipe2[1]);
                    }
                    else if (i == num_commands -1){
                        if (num_commands % 2 != 0){
                            close(pipe1[0]);
                        }
                        else{
                            close(pipe2[0]);
                        }
                    }
                    else{
                        if (i % 2 != 0){
                            close(pipe2[0]);
                            close(pipe1[1]);
                        }
                        else{
                            close(pipe1[0]);
                            close(pipe2[1]);
                        }
                    }
                    waitpid(pid,NULL,0);
                    sigemptyset(&mask);
                    sigaddset(&mask,SIGCHLD);
                    sigaddset(&mask,SIGINT);
                    sigaddset(&mask,SIGTSTP);
                    sigprocmask(SIG_BLOCK,&mask,NULL);
            }
        }
        else{
            printf(EXEC_NOT_FOUND,temp);
            return;
        }
    }
}

void helpFunction(){
    printf("help: requires 0 arguments\n");
    printf("cd: [no argument] or [.] or [..] or [-] or [PATH]\n");
    printf("exit: requires 0 arguments\n");
    printf("pwd: requires 0 arguments\n");
}

void pwdFunction(){
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL){
            printf("%s\n", cwd);
        }
        else{
            printf(BUILTIN_ERROR,"pwd");
        }
}



void cdFunction(char *input){
    if (input == NULL){
        char* loc = malloc(sizeof(char)*1000);
        size_t size = sizeof(char)*1000;
        loc = getcwd(loc,size);
        oldPWD = loc;
        char *home = getenv("HOME");
        chdir(home);
        //free(loc);
    }
    else if (strcmp(input,"cd") == 0){
        char* loc = malloc(sizeof(char)*1000);
        size_t size = sizeof(char)*1000;
        loc = getcwd(loc,size);
        oldPWD = loc;
        char *home = getenv("HOME");
        chdir(home);
        //free(loc);
    }
    else if (strcmp(input,".") == 0){
        char* loc = malloc(sizeof(char)*1000);
            size_t size = sizeof(char)*1000;
            loc = getcwd(loc,size);
            oldPWD = loc;
            //free(loc);
    }
    else if (strcmp(input,"-") == 0){
        if (strcmp(oldPWD,"NULL") == 0){
            printf(BUILTIN_ERROR,"cd");
        }
        else{
            char* loc = malloc(sizeof(char)*1000);
            size_t size = sizeof(char)*1000;
            loc = getcwd(loc,size);
            chdir(oldPWD);
            oldPWD = loc;
        }
    }
    else if (strcmp(input,"..") == 0){
        char* loc = malloc(sizeof(char)*1000);
        size_t size = sizeof(char)*1000;
        loc = getcwd(loc,size);
        oldPWD = loc;
        chdir(input);
    }
    else{
        char* loc = malloc(sizeof(char)*1000);
        size_t size = sizeof(char)*1000;
        loc = getcwd(loc,size);
        int outcome = chdir(input);
        if (outcome == -1){
            printf(BUILTIN_ERROR,"cd");
        }
        else{
            oldPWD = loc;
        }
        //free(loc);
    }
}


char* getPrompt(int color){
    //if(!isatty(STDIN_FILENO)) {
        //return NULL;
    //}
    char* loc = malloc(sizeof(char)*1000); // sizeof(char) could be omitted
    size_t size = sizeof(char)*1000;
    loc = getcwd(loc,size);
    if (strstr(loc,"/home/student") != NULL){
        str_cut(loc,0,13);
        loc = concat("~",loc);
    }
    if (color == -1){
        return concat(loc," :: brilin >> \033[0m");
    }
    else{
        loc = concat(loc," :: brilin >> \033[0m");
        switch(color) {
            case 0  :
                return concat(KRED,loc);
                break;
            case 1  :
                return concat(KGRN,loc);
                break;
            case 2  :
                return concat(KYEL,loc);
                break;
            case 3  :
                return concat(KBLU,loc);
                break;
            case 4  :
                return concat(KMAG,loc);
                break;
            case 5  :
                return concat(KCYN,loc);
                break;
            case 6  :
                return concat(KWHT,loc);
                break;
            case 7  :
                return concat(KBWN,loc);
                break;
            default:
                return concat(KNRM,loc);
                break; /* optional */
        }
    }
}

char* concat(const char *s1, const char *s2){
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}



int str_cut(char *str, int begin, int len){
    int l = strlen(str);

    if (len < 0) len = l - begin;
    if (begin + len > l) len = l - begin;
    memmove(str + begin, str + begin + len, l - len + 1);

    return len;
}

int countChars( char* s, char c ){
    return *s == '\0'? 0
              : countChars( s + 1, c ) + (*s == c);
}

void checkExec(char*args[], int size,char *infile,char *outfile,int IR,int OR){
    sigset_t prev,mask;
    signal(SIGCHLD, sigchld_handler);
    sigemptyset(&mask);
    sigaddset(&mask,SIGCHLD);
    sigaddset(&mask,SIGINT);
    sigaddset(&mask,SIGTSTP);
    sigprocmask(SIG_BLOCK,&mask,NULL);
    char *firstArg = args[0];
    char *temp = firstArg;
    if (strcmp(firstArg,"help") == 0){
        int pID = fork();
        if (pID == -1){
            printf(EXEC_ERROR,"Fork");
        }
        else if (pID == 0){
            signal(SIGCHLD, sigchld_handler);
            sigemptyset(&mask);
            sigaddset(&mask, SIGCHLD);
            sigaddset(&mask,SIGINT);
            sigaddset(&mask,SIGTSTP);
            sigprocmask(SIG_UNBLOCK,&mask,NULL);
            if (IR == 1){
                int input = open(infile, O_RDONLY);
                dup2(input, STDIN_FILENO);
                close(input);
            }
            if(OR == 1){
                int output = open(outfile, O_CREAT | O_WRONLY, 0666);
                dup2(output, STDOUT_FILENO);
                close(output);
            }
            helpFunction();
            exit(EXIT_SUCCESS);
        }
        else{
            pid = 0;
            while (!pid){
                sigsuspend(&prev);
            }
            signal(SIGCHLD, sigchld_handler);
            sigemptyset(&mask);
            sigaddset(&mask, SIGCHLD);
            sigaddset(&mask,SIGINT);
            sigaddset(&mask,SIGTSTP);
            sigprocmask(SIG_BLOCK,&mask,NULL);
        }
    }
    else if (strcmp(firstArg,"pwd") == 0){
        int pID = fork();
        if (pID == -1){
            printf(EXEC_ERROR,"Fork");
        }
        else if (pID == 0){
            signal(SIGCHLD, sigchld_handler);
            sigemptyset(&mask);
            sigaddset(&mask, SIGCHLD);
            sigaddset(&mask,SIGINT);
            sigaddset(&mask,SIGTSTP);
            sigprocmask(SIG_UNBLOCK,&mask,NULL);
            if (IR == 1){
                int input = open(infile, O_RDONLY);
                dup2(input, STDIN_FILENO);
                close(input);
            }
            if(OR == 1){
                int output = open(outfile, O_CREAT | O_WRONLY, 0666);
                dup2(output, STDOUT_FILENO);
                close(output);
            }
            pwdFunction();
            exit(EXIT_SUCCESS);
        }
        else{
            pid = 0;
            while (!pid){
                sigsuspend(&prev);
            }
            signal(SIGCHLD, sigchld_handler);
            sigemptyset(&mask);
            sigaddset(&mask, SIGCHLD);
            sigaddset(&mask,SIGINT);
            sigaddset(&mask,SIGTSTP);
            sigprocmask(SIG_BLOCK,&mask,NULL);
        }
    }
    else{
        char *error = firstArg;
        if (countChars(firstArg,'/') == 0){
            firstArg = concat("/bin/",firstArg);
            args[0] = firstArg;
        }
        struct stat fileStat;
        int statID = stat(firstArg, &fileStat);
        if (statID == 0){
            int pID = fork();
            if (pID == -1){
                printf(EXEC_ERROR,"Fork");
            }
            else if (pID == 0){
                signal(SIGCHLD, sigchld_handler);
                sigemptyset(&mask);
                sigaddset(&mask, SIGCHLD);
                sigaddset(&mask,SIGINT);
                sigaddset(&mask,SIGTSTP);
                sigprocmask(SIG_UNBLOCK,&mask,NULL);
                if (IR == 1){
                    int input = open(infile, O_RDONLY);
                    dup2(input, STDIN_FILENO);
                    close(input);
                }
                if(OR == 1){
                    int output = open(outfile, O_CREAT | O_WRONLY, 0666);
                    dup2(output, STDOUT_FILENO);
                    close(output);
                }
                execv(firstArg, args);
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                printf(EXEC_ERROR,error);
                exit(EXIT_FAILURE);
            }
            else{
                pid = 0;
                while (!pid){
                    sigsuspend(&prev);
                }
                sigemptyset(&mask);
                sigaddset(&mask, SIGCHLD);
                sigaddset(&mask,SIGINT);
                sigaddset(&mask,SIGTSTP);
                sigprocmask(SIG_BLOCK,&mask,NULL);
            }
        }
        else{
            printf(EXEC_NOT_FOUND,temp);
        }
    }
}


void sigchld_handler(int s){
    pid = waitpid(-1, NULL, 0);
}

void siginit_handler(int s){
    kill(pid,SIGINT);
}

void sigstp_handler(int s){
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    kill(getpid(), SIGTSTP);
    waitpid(-1,NULL,WNOHANG);
}

int isAllSpace(char *input){
    int length = (int)strlen(input);
    for (int i = 0; i < length; i++){
       char let = input[i];
       if (let != ' ')
            return -1;

    }
    return 0;
}

int getRedirection(char* args[], int *num_args, char** input_file, char** output_file, int* in, int* out){

    for(int i = 0; i < *num_args; i++){
        if(strcmp(args[i], "<") == 0){
            *input_file = args[i+1];
            if (input_file == NULL){
                printf(SYNTAX_ERROR,"<");
                return -1;
            }
            int j;
            for(j = i; j < *num_args; j++){
                args[j] = args[j+2];
            }
            *num_args = *num_args-2;
            *in = 1;
            i = 0;
        }
        if (strcmp(args[i], ">") == 0){
            *output_file = args[i+1];
            if (output_file == NULL){
                printf(SYNTAX_ERROR,"<");
                return -1;
            }
            int j;
            for(j = i; j < *num_args; j++){
                if(j < *num_args){
                args[j] = args[j+2];
                }
            }
            *num_args = *num_args-2;
            *out = 1;
            i = 0;
        }
    }
    return 1;
}

int get_args(char* cmdline, char* args[]){
        char *arg = strtok (cmdline, " ");
        int i = 0;
        while (arg != NULL){
            if (strcmp(arg,"") != 0){
                args[i] = arg;
                i++;
                arg = strtok(NULL," ");
            }
        }
        args[i] = NULL;
        return i;
}

void cleanInput(char ** input){
    int j;
    int i = 0;
    char *str = *input;
    int len = strlen(str);
    while (i < len){
        if(str[i]==' ' && (str[i+1]==' ' || str[i-1]==' ')){
            for(j=i;j<len;j++)
                str[j]=str[j+1];
             len--;
        }
        else{
            i++;
        }
    }
    i = 0;
    len = strlen(str);
    while (i < len){
        int checker = 0;
        if(str[i]=='<'){
            if (str[i + 1] != ' '){
                insert_substring(str," ", i + 2);
                len++;
                i++;
                checker = 1;
            }
            if (str[i - 1] != ' '){
                if (checker == 1)
                    insert_substring(str," ", i);
                else
                    insert_substring(str," ", i+1);
                len++;
            }
            i++;
        }
        else if(str[i] == '>'){
            if (str[i + 1] != ' '){
                insert_substring(str," ", i + 2);
                len++;
                i++;
                checker = 1;
            }
            if (str[i - 1] != ' '){
                if (checker == 1)
                    insert_substring(str," ", i);
                else
                    insert_substring(str," ", i+1);
                len++;
            }
            i++;
        }
        else if (str[i] == '|'){
            if (str[i + 1] != ' '){
                insert_substring(str," ", i + 2);
                len++;
                i++;
                checker = 1;
            }
            if (str[i - 1] != ' '){
                if (checker == 1)
                    insert_substring(str," ", i);
                else
                    insert_substring(str," ", i+1);
                len++;
            }
            i++;
        }
        else{
            i++;
        }
    }
}

char *substring(char *string, int position, int length){
   char *pointer;
   int c;
   pointer = malloc(length+1);
   if( pointer == NULL )
       exit(EXIT_FAILURE);
   for( c = 0 ; c < length ; c++ )
      *(pointer+c) = *((string+position-1)+c);
   *(pointer+c) = '\0';
   return pointer;
}

void insert_substring(char *a, char *b, int position){
   char *f, *e;
   int length;
   length = strlen(a);
   f = substring(a, 1, position - 1 );
   e = substring(a, position, length-position+1);
   strcpy(a, "");
   strcat(a, f);
   free(f);
   strcat(a, b);
   strcat(a, e);
   free(e);
}

int checkSyntax(char *input[],int size){
    char *firstArg = input[0];
    if (strcmp(firstArg,"<") == 0){
        printf(SYNTAX_ERROR,"<");
        return -1;
    }
    else if(strcmp(firstArg,">")== 0){
        printf(SYNTAX_ERROR,">");
        return -1;
    }
    else if(strcmp(firstArg,"|")== 0){
        printf(SYNTAX_ERROR,"|");
        return -1;

    }
    char *lastArg = input[size-1];
    if (strcmp(lastArg,"<")== 0){
        printf(SYNTAX_ERROR,"<");
        return -1;
    }
    else if(strcmp(lastArg,">")== 0){
        printf(SYNTAX_ERROR,">");
        return -1;
    }
    else if(strcmp(lastArg,"|")== 0){
        printf(SYNTAX_ERROR,"|");
        return -1;
    }
    for (int i = 1; i < size-1 ; i++){
        char *str = input[i];
        if ((strcmp(str,"cd") == 0||strcmp(str,"exit") == 0||strcmp(str,"kill") == 0||strcmp(str,"fg") == 0||strcmp(str,"jobs") == 0)){
            printf(SYNTAX_ERROR,str);
            return -1;
        }
    }
    return 1;
}
