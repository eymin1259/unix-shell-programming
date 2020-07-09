#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

#define BACKGROUND 1
static const char *prompt = "myshell★ > ";
char* cmdvector[MAX_CMD_ARG];
char* cmd2vector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];	
int where;		// background실행 or foreground실행
pid_t pid;

void fatal(char *str);	//오류출력	 
int makeargv(char *s, const char *delimiters, char** list, int MAX_LIST); //문자열 커맨드를 vector형태로변환 
void z_handler();	// 좀비프로세스 핸들러 
void userIn();		// 유저로부터 커맨드 입력 
void runcommand();	// 커맨드 실행 

struct sigaction act1;
struct sigaction act2;

int main(int argc, char**argv){
	//SIGINT 핸들러정의 
	sigemptyset(&act1.sa_mask);
	act1.sa_flags = SA_RESTART;	
	act1.sa_handler = SIG_IGN;
	sigaction(SIGINT,&act1, NULL);

	//SIGQUIT 핸들러정의 
	sigemptyset(&act2.sa_mask);
	act2.sa_flags = SA_RESTART;	
	act2.sa_handler = SIG_IGN;
	sigaction(SIGQUIT,&act2, NULL);

	signal(SIGCHLD,z_handler); 	//좀비프로세스 핸들러등록

	  while (1) {
		where = -1;		// background실행 or foreground실행 
		userIn();		//커맨드 입력 
		runcommand();		//커맨드 실행 
  	}
 	return 0;
}

void fatal(char *str){
	perror(str);
	exit(1);
}

int makeargv(char *s, const char *delimiters, char** list, int MAX_LIST){	
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);	// Skip delimiters 
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;
	
  numtokens = 1;
  
  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
	break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;
}

void z_handler(){	// zombie proc handler
	pid_t child;
	int state;
	child = waitpid(-1, &state, WNOHANG);
}

void userIn(){		//get user input
	fputs(prompt, stdout);
	fgets(cmdline, BUFSIZ, stdin);
	cmdline[strlen(cmdline) -1] = '\0';

	 //background parsing : background 실행시 where을 background로 설정 
	for(int i=0; i< strlen(cmdline); i++){
	    if(cmdline[i] == '&'){
		cmdline[i] = ' ';
		where = BACKGROUND;
	    }
	}
}



void runcommand(){

	//pipe parsing		
	char* cmd2 = NULL;
	int cmdlen = strlen(cmdline);
	for(int i=0; i<cmdlen; i++){
		if(cmdline[i] == '|'){	
			cmd2 = strtok(&cmdline[i+1], "\0");
			cmd2[strlen(cmd2)] = '\0';
			cmdline[i] = '\0';
		}
	}

	//redirection parsing
	char* arg1 = NULL;
	char* arg2 = NULL;
	int fd;
	int redirection1 = 0;
	int redirection2 = 0;
	for(int i=0; i<cmdlen; i++){
		if(cmdline[i] == '<'){	
			arg1 = strtok(&cmdline[i+1], " \t");
			cmdline[i] = '\0';
			redirection1 = 1;
		}
		if( cmdline[i] == '>'){
			arg2 = strtok(&cmdline[i+1], " \t");
			cmdline[i] = '\0';
			redirection2 = 1;
		}
	}

	//cmdvector 생성 
	int numtoken = makeargv(cmdline, " \t", cmdvector, MAX_CMD_ARG);
	if(numtoken == 0) return;
	if(numtoken < 0) fatal("main()");

	// exit 명령
	if(!strcmp(cmdvector[0], "exit")) exit(0);

	switch(pid=fork()){
	case 0:	//child process

		//redirection
		if(redirection1 == 1){ // <
			if((fd=open(arg1,O_RDONLY | O_CREAT, 0644))<0)
				fatal("file open error");
			dup2(fd, 0); //stdin
			close(fd);
		}
		if(redirection2 == 1){	// >
			if((fd=open(arg2,O_WRONLY | O_CREAT|O_TRUNC, 0644))<0)
				fatal("file open error");
			dup2(fd, 1); //stdout
			close(fd);
		}
		

		// SIGINT와 SIGQUIT핸들러를 SIG_DFL로 다시 설정 
		act1.sa_handler = SIG_DFL;
		sigaction(SIGINT,&act1, NULL);
		act2.sa_handler = SIG_DFL;
		sigaction(SIGQUIT,&act2, NULL);

		//pipe
		if(cmd2){
			int pipefd[2];
			pipe(pipefd);	//파이프 생성 
			pid_t pid2 = fork();
			switch(pid2){
			case -1: fatal("main()");
			case 0 : // 손자프로세스
			{
				//redirection parsing
				char* arg1 = NULL;
				char* arg2 = NULL;
				int fd;
				int redirection1 = 0;
				int redirection2 = 0;
				for(int i=0; i<cmdlen; i++){
					if(cmdline[i] == '<'){	
						arg1 = strtok(&cmd2[i+1], " \t");
						cmd2[i] = '\0';
						redirection1 = 1;
					}
					else if( cmdline[i] == '>'){
						arg2 = strtok(&cmd2[i+1], " \t");
						cmd2[i] = '\0';
						redirection2 = 1;
					}
				}
				//redirection
				if(redirection1 == 1){ // <
					if((fd=open(arg1,O_RDONLY | O_CREAT, 0644))<0)
						fatal("file open error");
					dup2(fd, 0); //stdin
					close(fd);
				}
				if(redirection2 == 1){	// >
					if((fd=open(arg2,O_WRONLY | O_CREAT|O_TRUNC, 0644))<0)
						fatal("file open error");
					dup2(fd, 1); //stdout
					close(fd);
				}
				
				//cmd2vector 생성	 
				if(makeargv(cmd2, " \t", cmd2vector, MAX_CMD_ARG) <= 0) 
					fatal("main()");
				//파이프 연결 : 손자프로세스의 출력을 자식프로세스의 입력으로 전달 
				dup2(pipefd[1], 1);
				close(pipefd[1]);
				close(pipefd[0]);	
				execvp(cmd2vector[0], cmd2vector);
				fatal("main()");
			}
			default: //자식프로세스 
				//파이프 연결 : 자식프로세스는 손자프로세스의 출력을 입력으로 받음
				waitpid(pid2, NULL, 0);
				dup2(pipefd[0], 0);
				close(pipefd[1]);
				close(pipefd[0]);
			}
		}
				
		if(strcmp(cmdvector[0], "cd")){		//cd 명령이 아니면 실행
		    execvp(cmdvector[0], cmdvector);	
		    fatal("main()");
		}
		exit(0);			//cd 명령일때 자식프로세스는 그냥 종료 
	case -1: fatal("main()");
	default: //parent process : myshell
		if(!strcmp(cmdvector[0], "cd"))	//cd 명령이면 실행
		    if(chdir(cmdvector[1])==-1)
		        fatal("main()");
		if(where == BACKGROUND)	//background 실행시 wait하지 않는다 
		    return;
	        waitpid(pid, NULL, 0);
	}
}
