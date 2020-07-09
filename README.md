myshell specification

- unix shell command를 실행하는 myshell 구현
1. cd 구현
2. exit 구현
3. 백그라운드 실행 구현
4. SIGCHLD로 자식 프로세스 wait() 시 프로세스가 온전하게 수행되도록 구현
5. ^C, ^＼사용시쉘이종료되지않도록구현
6. redirection 구현
7. pipe 구현 (파이프는 하나만 동작하도록 구현)

- pseudocode
variable where 
Ignore SIGCHLD
Define sigaction act1 ignoring SIGQUIT
Define sigaction act2 ignoring SIGINT
LOOP forever
	Print(“prompt> “)
	Get commandline
	Parse commandline in order to know if < or > is included or not: redirection parsing
	IF ‘&’ is included in commandline THEN where <-background
	Make command vector
	IF command is ‘exit’ THEN terminate myshell
	Make child process
	SWITCH
		case child proc : 
			IF ‘<’ is included in command THEN duplicate fd of RDONLY opened file to STDIN
			IF ‘>’ is included in command THEN duplicate fd of WRONLY opened file to STDOUT
			Set act1.sa_handler to SIG_DFL
			Set act2.sa_handler to SIG_DFL
			IF command is not ‘cd’ THEN execute commandline
			exit
		case parent proc :
			IF command is ‘cd’ THEN execute commandline
			IF where is background THEN restart LOOPwait 
			child proc