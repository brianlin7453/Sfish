#ifndef SFISH_H
#define SFISH_H

/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"
#define BUILTIN_ERROR  "sfish builtin error: %s\n"
#define SYNTAX_ERROR   "sfish syntax error: %s\n"
#define EXEC_ERROR     "sfish exec error: %s\n"
void helpFunction();
void cdFunction(char *input);
void pwdFunction();
char* concat(const char *s1, const char *s2);
char* getPrompt();
int str_cut(char *str, int begin, int len);
int countChars(char* s, char c );
void checkExec(char* args[],int size,char *infile,char *outfile,int IR,int OR);
void sigchld_handler(int s);
char** getParams(char* argv[],int size);
void getInput(char* t1);
char* getOutput(char* t2);
int isAllSpace(char* input);
int getRedirection(char* args[], int *num_args, char** input_file, char** output_file, int* in, int* out);
int get_args(char* cmdline, char* args[]);
void cleanInput(char ** input);
char *substring(char *string, int position, int length);
void insert_substring(char *a, char *b, int position);
int checkSyntax(char *input[],int size);
void execute_pipes(char *job_list[],int num_args, int num_pipes, char *input);
void siginit_handler(int s);
int colorFunction(char *color);
void sigstp_handler(int s);

#endif

