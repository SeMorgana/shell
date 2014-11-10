#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tokenizer.h"
#include <fcntl.h>
#include "linkedList.h"

int shell_pgid;

struct node* jobs_head;

sigset_t block_set;

/*
 * convert an a ascii string into an integer
 * args:
 *	 input: an ascii string
 *
 * return:
 * 	an integer; if coversion not valid, returns 1
 */
int a_to_i(char * input){
	int res = 0;
	int def = 1;
	int i = 0;
	while(input[i] != '\0'){
		if(input[i] < 48 || input[i]> 57 ){return def;}
		if(i>0){res = res*10;}
		res = res + input[i] - 48;
		i++;
	}
	return res;
}


/*
 * get the first token from a string
 * args:
 *		cmd: an string containing the program and its arguments
 *
 * return:
 * 		an string representing a program in the system
 */
char * get_path( char* cmd){
  TOKENIZER *tokenizer;
	tokenizer = init_tokenizer(cmd);
	char * tok = get_next_token( tokenizer );
	free_tokenizer(tokenizer);
	return tok;
}

/*
 * get an array of all the tokens except the first from a string
 * args:
 *	 	cmd: an string containing the program and its arguments
 *
 * return:
 * 		an array of strings representing all the arguments of a program
 */
char ** parse_cmd_args(char* cmd){
	TOKENIZER * tokenizer;
	char * tok;
	tokenizer = init_tokenizer(cmd);
	int num_args=0;
	while( (tok = get_next_token(tokenizer)) != NULL){
		num_args++;
		free(tok);
	}
	free_tokenizer(tokenizer);
	
	tokenizer = init_tokenizer(cmd);
	char ** arg = malloc( (num_args+1)*4 );
	int index = 0;
	while( (tok = get_next_token(tokenizer)) != NULL){
		arg[index] = tok;
		index++;
	}
	free_tokenizer( tokenizer );
	arg[num_args] = NULL;	
	return arg;
}


/*
 * check whether there is status change in a job. All the changes are 
 * done by sig_handler() function, if any. This function is called at 
 * the beginning of the while loop so that the update infomation of a 
 * job can be printed without interrupting the user. 
 * args:
 *	 	head: the address of the head of the list 
 *
 * return:
 *	 	0 if successful
 */
int check_gp_stats(struct node ** head){
	struct node * curr = *head;
	struct node * pre = *head;
	int index = 1;  // index of the current node within list
	
	// iterates through all node within list
	while (curr != NULL){
		if( curr->stat_change ){
			if( curr->status == 0 ){ //done
				printf("Finished: %s\n",curr->pg_name.value);
				
				if( curr == *head ){
					
					*head = deleteValue(*head, curr->value);
					pre = *head;
					curr = pre;
					
				} else {
					curr = curr->next;
					*head = deleteValue(*head, curr->value);
				}
			}else if( curr->status == 2){ //running
				printf("Running: %s\n",curr->pg_name.value);
				curr->stat_change = 0;
			}else if( curr->status == 1) {  //stopped
				printf("Stopped: %s\n",curr->pg_name.value);
				curr->stat_change = 0;
			}else if( curr->status == 3) {	//killed 
				printf("Killed: %s\n",curr->pg_name.value);
				if( curr == *head ){
					*head = deleteValue(*head, curr->value);
					pre = *head;
					curr = pre;
				} else {
					curr = curr->next;
					*head = deleteValue(*head, curr->value);
				}
			}
		}else{
			pre = curr;
			curr = curr->next;
		}	
		index++;
	}
	
	return 0;
}

/*
 * get the name of the job based on the user input
 * args:
 *		buffer: user's input
 *
 * return:
 * 		a struct containing a string representing the name of the job
 */
struct Name get_pg_name(char* buffer){
	struct Name pg_name;
	int index=0;
	while( buffer[index] != '\0' && buffer[index] != '&'  && buffer[index] != '\n'){
		pg_name.value[index] = buffer[index];
		index++;
	}
	pg_name.value[index] = '\0';
	return pg_name;
}

/*
 * a simple check whether the input is valid
 * args:
 *		buffer: user's input
 *		len_buffer: the length of the buffer
 * 
 * return:
 * 		1 if the input is valid; 0 otherwise
 */
int is_input_valid(char * buffer, int len_buffer){
	int i = 0;
	int num_pipe = 0;
	int num_in= 0;
	int num_out = 0;
	int num_err = 0;
	
	for(i=0;i< len_buffer;i++){

		if( buffer[i] == '>' ){
			if(i-1 < 0){ return 0; }
			if( buffer[i-1] == '2' ){
				num_err++;
			} else {
				num_out++;
			}
		}else if( buffer[i] == '<' ){
			num_in++;
		}else if( buffer[i] == '|' ){
			num_pipe++;
			if (num_pipe == 1){
				if (num_out) return 0;
				if (num_in > 1) return 0;
				if (num_err > 1) return 0; 
			}else{
				if (num_out) return 0;
				if (num_in) return 0;
				if (num_err > 1) return 0; 
			}
			num_out = 0;
			num_in = 0;
			num_err = 0;
		}else if(i == len_buffer-1){
			if( num_pipe ){
				if (num_out > 1) return 0;
				if (num_in) return 0;
				if (num_err > 1) return 0; 
			} else {
				if (num_out > 1) return 0;
				if (num_in > 1) return 0;
				if (num_err > 1) return 0;
			}
		}
	}
	return 1;
}

/*
 * signal handler for handling the SIGCHLD signal from the child process
 * args:
 *		signum: the number representing the signal
 *
 * return:
 * 		none
 */
void sig_handler(int signum){
	// block signal set
	sigprocmask(SIG_BLOCK, &block_set, NULL);
	
	// handle when child changes status
	if( signum == SIGCHLD ){
		// get the returned status from child and notify the changes
		int status;
		int ret = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED);
		if( ret == 0 || ret == -1){return;}
		struct node * ptr = getBypgid(jobs_head,ret);
		if( ptr == NULL ) return;
		
		// case exited
		if( WIFEXITED(status) ){
			ptr->stat_change = 1;
			ptr->status = 0;
		
		// case continued
		} else if (WIFCONTINUED(status)){
			ptr->stat_change = 1;
			ptr->status = 2;
			
		// case terminated
		} else if (WIFSIGNALED(status)){
			ptr->stat_change = 1;
			ptr->status = 3;
			
		// case stopped
		}else if( WIFSTOPPED(status) ){	
			tcsetpgrp(STDIN_FILENO,getpgid(shell_pgid));
			ptr->stat_change = 1;
			ptr->status = 1;			
		}
	}
	
	// unblock signal set
	sigprocmask(SIG_UNBLOCK, &block_set, NULL);
}

/*
 *	the start of the program 
 */
int main( int argc, char *argv[] )
{
	/********************************* initialize the program *********************************/
	// save the shell's pgid
	shell_pgid = getpgid(getpid());
	// set up signal set to block and trap other signals
	sigemptyset(&block_set);
	sigaddset(&block_set, SIGCHLD);
	signal(SIGCHLD, sig_handler);
	// set up buffer size
	int buf_size = 1024;
	// job lists
	jobs_head = NULL;
	// ignore the following signals
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	// start processing
	while(1){
		// check jobs status changes
		check_gp_stats(&jobs_head);
		// prompt
		write(STDOUT_FILENO,"kinda-sh> ",10);
		// read in input
		char buffer[1024]="";
		int input_len = read(STDIN_FILENO,buffer, buf_size);
		if(input_len == 0){break;}
		if(input_len == 1){continue;}
		int bl;
		int is_all_blank=1;
		for(bl=0; bl<input_len-1;bl++){
			if( buffer[bl] != ' ') { 
				is_all_blank = 0;
				break;
			}
		}
		if( is_all_blank ) continue;
		
		buffer[1023] = '\0';
		
		// check input validity
		if ( !is_input_valid(buffer,input_len) ){
				fprintf(stderr, "Invalid input!\n");
				continue;
		}
		TOKENIZER * tokenizer;
		char * tok;
		
		/*----------------------------------evaluate if it is bg fg jobs-------------------------------------*/
		int is_cmd_bg = 0;
		int is_cmd_fg = 0;
		int fg_index = 1;
		int bg_index = 1;
		int is_cmd_jobs=0;
		tokenizer = init_tokenizer(buffer);
		tok = get_next_token( tokenizer );
		
		// check whether the token is bg
		if( input_len>=3 && tok[0]=='b' && tok[1]=='g' && tok[2]=='\0' ){
			is_cmd_bg = 1;
			free( tok );
			tok = get_next_token( tokenizer );
			if(tok != NULL && tok[0]== '%' && tok[1] != '\0'){
				char * temp = &tok[1];
				bg_index = a_to_i(temp);
			}
			
		// check whether the token is fg
		} else if (input_len>=3 && tok[0]=='f' && tok[1]=='g' && tok[2]=='\0' ){
			is_cmd_fg = 1;
			free( tok );
			tok = get_next_token( tokenizer );
			if(tok != NULL && tok[0]== '%' && tok[1] != '\0'){
				char * temp = &tok[1];
				fg_index = a_to_i(temp);
			}
			
		// check whether the token is jobs
		} else if ( input_len>=5 && tok[0]=='j' && tok[1]=='o' && tok[2]=='b' && tok[3]=='s' && tok[4]=='\0'){
				is_cmd_jobs = 1;
		}
		
		// free token
		if( tok != NULL ){free(tok);}
		free_tokenizer( tokenizer );
		
		/*----------------------------------handle shell command-----------------------------------------*/
		// case foreground
		if(is_cmd_fg) { //bring the (fg_index)th job into foreground
			sigprocmask(SIG_BLOCK, &block_set, NULL);
			struct node * ptr = getByIndex(jobs_head, fg_index);		
			// check wether there is any background process
			if (ptr == NULL){
				write(STDERR_FILENO,"no such jobs\n ",13);
				continue;
			}
			
			// gives the process terminal control and continue it
			tcsetpgrp(STDIN_FILENO,ptr->value);
			ptr->status = 2;
			kill(-ptr->value,SIGCONT);
			
			// wait for the process to return
			int status;
			int ret = waitpid(-ptr->value,&status,WUNTRACED );
			tcsetpgrp(STDIN_FILENO,shell_pgid);
			ptr = getBypgid(jobs_head,ret);
			
			// check the return status
			if( WIFEXITED(status) ){	
				jobs_head = deleteValue(jobs_head, ret);
			} else if (WIFCONTINUED(status)){
				ptr->stat_change = 1;
				ptr->status = 2;
			} else if (WIFSIGNALED(status)){
				printf("%s\n", "ter");
				ptr->stat_change = 1;
				ptr->status = 3;
					
			}else if( WIFSTOPPED(status) ){	
				tcsetpgrp(STDIN_FILENO,getpgid(shell_pgid));
				ptr->stat_change = 1;
				ptr->status = 1;			
			}
			sigprocmask(SIG_UNBLOCK, &block_set, NULL);
			continue;
		}
		
		// case background
		if(is_cmd_bg) {// send countinue signal to the most recent job
			struct node * ptr = getByIndex(jobs_head, bg_index);

			// check wether there is any background process
			if (ptr == NULL){
				write(STDERR_FILENO,"no such jobs\n ",13);
				continue;
			}
			
			kill(-ptr->value,SIGCONT);
			continue;
		}

		// case jobs, prints out all the jobs
		if(is_cmd_jobs) {
			int index = 1;
			if( jobs_head == NULL ){
				continue;
			}
			struct node* curr = jobs_head;
			
			// prints out all the jobs
			while(curr !=NULL ){
				printf("[%d] %s (%s)\n",index, curr->pg_name.value, &(getStatus(curr).value[0]) );
				curr = curr->next;
				index++;
			}
			continue;
		}
		
		/*---------------------------------------other command-----------------------------------------*/
		//get the number of < > |
		tokenizer = init_tokenizer(buffer);
		//get the pg_name here
		struct Name pg_name = get_pg_name(buffer);
		int len_redi_pipe = 0;	// number of redirections or pipes
		int len_pipe = 0;		// number of pipes
		int len_bg = 0;			//background
		
		// iterates each token
		while( (tok = get_next_token( tokenizer )) != NULL ){
			if( tok[0] == '<' || tok[0]=='>' || tok[0]=='|' || tok[0] == '&'){
				len_redi_pipe++;
				if(tok[0] == '|'){len_pipe++;}
				if(tok[0] == '&'){len_bg++;}	
			}
			free(tok);		//free
		}
		len_redi_pipe -= len_bg;		//remove & symbol
		//free_tokenizer( tokenizer );
		//allocate memory based on the previous number
		char * cmds[len_redi_pipe+1];	//number of cmds in between < > |
		char delimiters[len_redi_pipe];	//1 less than cmds
		int index_cmd=1;
		cmds[0] = &buffer[0];
		int index_deli=0;
		int i=0;
		while(buffer[i] != '\0' ){
			if( buffer[i] == '&' ){ buffer[i] = ' ';}		//remove '&'
			if( buffer[i]=='<'||buffer[i]=='>'||buffer[i]=='|' ){
				if( buffer[i] == '>' && buffer[i-1] == '2'){
					delimiters[index_deli] = '2';
				} else {
					delimiters[index_deli] = buffer[i];
				}
					index_deli++;
					buffer[i]='\0';
					cmds[index_cmd]=&buffer[i+1];
					index_cmd++;
			}
			i++;
		}

		/*-------------------------redirection and pipe and back ground handling----------------------------*/
		if( len_bg > 1 ){
			fprintf(stderr, "Invalid number of & token!\n");
			continue;
		}	
		
		int saved_stderr = dup(2);
		int saved_stdout = dup(1);
		int saved_stdin = dup(0);
		int cmd_index = 0;

		int fd[len_pipe][2];
		int p;

		for(p=0;p<len_pipe;p++){
			pipe(fd[p]);
		}
		int pcount=0;
		
		char *filename[3];  // file name to open file descriptor with
		int file_out;    // output file descriptor
		int file_in;     // input file descriptor
		int file_err;
	
		int f_out_b = 0; // boolean if file out
		int f_in_b = 0;  // boolean if file in
		int f_err_b = 0; // boolean if file error
		
		int fcmdnum;

		// handle w/o pipe case
		if(len_pipe==0){
			int i;
			// look at each symbols and set up redirections and pipe numbers
			for(i=0;i<len_redi_pipe;i++){
				if( delimiters[i] == '>' ){
					filename[1] = get_path(cmds[i+1]);
					f_out_b++;
				} else if( delimiters[i] == '<' ){
					filename[0] = get_path(cmds[i+1]);
					f_in_b++;
				} else if( delimiters[i] == '2' ){
					filename[2] = get_path(cmds[i+1]);
					f_err_b++;
				}
			}
			
			// set up program path and arguments
			char * program = get_path(cmds[0]);
			char ** args = parse_cmd_args(cmds[0]);
				
			int pid = fork();
			
			if(!pid){//child process
				// reset default actions
				signal(SIGTTOU, SIG_DFL);
				signal(SIGTTIN, SIG_DFL);
				signal(SIGTERM, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				signal(SIGINT, SIG_DFL);

				setpgid(getpid() , getpid());
				// if its foreground process, gain terminal control
				if( !len_bg){
					tcsetpgrp(STDOUT_FILENO,getpgid(getpid()));
				}
				
				// check if redirection needed
				if (f_out_b){
					file_out = open(filename[1],O_WRONLY|O_CREAT,0644);
					dup2(file_out,STDOUT_FILENO);
				} 
				if (f_in_b){
					file_in = open(filename[0],O_RDONLY);
					dup2(file_in,STDIN_FILENO);
				}
				if(f_err_b){
					file_err = open(filename[2],O_WRONLY|O_CREAT,0644);
					dup2(file_err,STDERR_FILENO);
				}
			
				// call program
				if( execvp(program, args) == -1){
					perror("execvp failed: ");
					_exit(0);
				}
			}else{// parent process
				// block signal to update jobs linked list
				sigprocmask(SIG_BLOCK, &block_set, NULL);
				jobs_head = addVal(jobs_head, pid, pg_name);
				setpgid(pid ,pid);
				sigprocmask(SIG_UNBLOCK, &block_set, NULL);
				
				if( !len_bg ) { //foreground
					sigprocmask(SIG_BLOCK, &block_set, NULL);
					tcsetpgrp(STDIN_FILENO, getpgid(pid));
						
					int status;
					// wait for child
					if(waitpid(pid,&status,WUNTRACED) == pid ){
						// reset terminal control, stdin out, signal set actions, and file booleans
						tcsetpgrp(STDIN_FILENO,getpgid(shell_pgid));				
						sigprocmask(SIG_UNBLOCK, &block_set, NULL);
						dup2(saved_stdout, STDOUT_FILENO);
						dup2(saved_stdin, STDIN_FILENO);
						f_out_b=0;
						f_in_b=0;
						f_err_b=0;
						
						struct node * ptr = getBypgid(jobs_head,pid);
						// check the return status
						if( WIFEXITED(status) ){
							jobs_head = deleteValue(jobs_head , pid);	
						} else if (WIFCONTINUED(status)){
							ptr->stat_change = 1;
							ptr->status = 2;
						} else if (WIFSIGNALED(status)){
							printf("%s\n", "ter");
							ptr->stat_change = 1;
							ptr->status = 3;
								
						}else if( WIFSTOPPED(status) ){	
							tcsetpgrp(STDIN_FILENO,getpgid(shell_pgid));
							ptr->stat_change = 1;
							ptr->status = 1;			
						}
					}
				} else {//if it is background, then give terminal control to the shell
					printf("Running: %s", cmds[0]);
				}
				if( f_out_b ){free(filename[1]);}
				if( f_in_b ) {free(filename[0]);}
				if( f_err_b ){free(filename[2]);}
				//free(args);
				free(program);
			}	
			
		} else {// pipe number is not 0
			
			/********************************* multi staging piping *********************************/
			int pgid = 0;  // the shared pgid
			
			// iterates through each delimiters and set up redirections
			// and fork when encounter a pipeline
			for(i=0;i<len_redi_pipe;i++){
				if( (delimiters[i] == '2') ){
					f_err_b = 1;
					fcmdnum = i+1;
				} else if ((delimiters[i] == '>')){
					f_out_b = 1;
					fcmdnum = i + 1;
					
				} else if ((delimiters[i] == '<')){
					f_in_b = 1;
					fcmdnum = i + 1;
				
				// encounters a pipeline the first time
				}else if ((delimiters[i] == '|') && (cmd_index == 0)){ // first call
					sigprocmask(SIG_BLOCK, &block_set, NULL);
					char *program = get_path(cmds[cmd_index]);
					char **args = parse_cmd_args(cmds[cmd_index]);
				
					int pid = fork();
					
					if( !pid ){//child process
						// reset default actions
						signal(SIGTTOU, SIG_DFL);
						signal(SIGTTIN, SIG_DFL);
						signal(SIGTERM, SIG_DFL);
						signal(SIGTSTP, SIG_DFL);
						signal(SIGINT, SIG_DFL);
						sigprocmask(SIG_UNBLOCK, &block_set, NULL);
						
						setpgid(getpid() ,getpid());
						
						// set up pipe
						close(fd[0][0]);
						dup2(fd[0][1],1);
						
						// check if redirection needed
						if (f_out_b){
							filename[1] = get_path(cmds[fcmdnum]);
							file_out = open(filename[1],O_WRONLY|O_CREAT,0644);
							dup2(file_out,STDOUT_FILENO);
						} 
						 if (f_in_b){
							filename[0] = get_path(cmds[fcmdnum]);
							file_in = open(filename[0],O_RDONLY);
							dup2(file_in,STDIN_FILENO);
						}
						if ( f_err_b){
							filename[2] = get_path(cmds[fcmdnum]);
							file_err = open(filename[2],O_WRONLY|O_CREAT,0644);
							dup2(file_err, STDERR_FILENO);
						}
			
						// call program
						if( execvp(program, args) == -1){
							perror("execvp failed: ");
							_exit(0);
						}
					}else{// parent process
						pgid = pid;
						setpgid(pid, pgid);
						close(fd[0][1]);
						// reset file boolean
						f_out_b = 0;
						f_in_b = 0;	
						f_err_b = 0;
						
						if( f_out_b ){free(filename[1]);}
						if( f_in_b ) {free(filename[0]);}
						if( f_err_b ){free(filename[2]);}
						//free(args);
						free(program);
					}
					
					cmd_index = i + 1;
				
				// other calls between the first call and the last call
				}else if (delimiters[i] == '|'){
					char *program = get_path(cmds[cmd_index]);
					char **args = parse_cmd_args(cmds[cmd_index]);
					int pid = fork();
					
					if( !pid ){	//child process				
						// reset default actions
						signal(SIGTTOU, SIG_DFL);
						signal(SIGTTIN, SIG_DFL);
						signal(SIGTERM, SIG_DFL);
						signal(SIGTSTP, SIG_DFL);
						signal(SIGINT, SIG_DFL);
						sigprocmask(SIG_UNBLOCK, &block_set, NULL);
						
						setpgid(getpid(), pgid);
						
						// set up pipe
						dup2(fd[pcount][0],0);				
						dup2(fd[pcount+1][1],1);
						
						
						// check if redirection needed
						if (f_out_b){
							filename[1] = get_path(cmds[fcmdnum]);
							file_out = open(filename[1],O_WRONLY|O_CREAT,0644);
							dup2(file_out,STDOUT_FILENO);
						}
						 if (f_in_b){
							filename[0] = get_path(cmds[fcmdnum]);
							file_in = open(filename[0],O_RDONLY);
							dup2(file_in,STDIN_FILENO);
						}
						if ( f_err_b){
							filename[2] = get_path(cmds[fcmdnum]);
							file_err = open(filename[2],O_WRONLY|O_CREAT,0644);
							dup2(file_err, STDERR_FILENO);
						}	

						// call program
						if( execvp(program, args) == -1){
							perror("execvp failed: ");
							_exit(0);
						}
						
					}else{// parent process
						setpgid(pid, pgid);
						close(fd[pcount+1][1]);
						// reset file boolean
						f_out_b = 0;
						f_in_b = 0;
						f_err_b=0;	
						
						if( f_out_b ){free(filename[1]);}
						if( f_in_b ) {free(filename[0]);}
						if( f_err_b ){free(filename[2]);}
						//free(args);
						free(program);
					}	
					pcount++;
					cmd_index = i + 1;
				}
			}
			
			// last call
			char *program = get_path(cmds[cmd_index]);
			char **args = parse_cmd_args(cmds[cmd_index]);
			int pid = fork();
			
			if( !pid ){
				// reset default actions
				signal(SIGTTOU, SIG_DFL);
				signal(SIGTTIN, SIG_DFL);
				signal(SIGTERM, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				sigprocmask(SIG_UNBLOCK, &block_set, NULL);
				
				setpgid(getpid(), pgid);
				close(fd[pcount][1]);
				dup2(fd[pcount][0],0);

				// check if its foreground
				if ( !len_bg ){
					tcsetpgrp(STDOUT_FILENO,getpgid(getpid()));
				}

				// check if redirection needed
				if (f_out_b){
						filename[1] = get_path(cmds[fcmdnum]);
						file_out = open(filename[1],O_WRONLY|O_CREAT,0644);
						dup2(file_out,STDOUT_FILENO);
				}  
				if (f_in_b){
						filename[0] = get_path(cmds[fcmdnum]);
						file_in = open(filename[0],O_RDONLY);
						dup2(file_in,STDIN_FILENO);
				}
				if ( f_err_b){
					filename[2] = get_path(cmds[fcmdnum]);
					file_err = open(filename[2],O_WRONLY|O_CREAT,0644);
					dup2(file_err, STDERR_FILENO);
				}
			
				// call program
				if( execvp(program, args) == -1){
					perror("execvp failed: ");
					_exit(0);
				}
			}else{// parent process
				jobs_head = addVal(jobs_head, pgid, pg_name);
				setpgid(pid, pgid);
				if( !len_bg ){  // foreground
					tcsetpgrp(STDIN_FILENO, getpgid(pid));
					
					int status;
					// wait for child
					if(waitpid(pid,&status,WUNTRACED) == pid ){
						// reset terminal control, close pipe, and file booleans
						tcsetpgrp(STDIN_FILENO,getpgid(shell_pgid));
						close(fd[pcount][1]);
						f_out_b = 0;
						f_in_b = 0;
						f_err_b=0;

						struct node* ptr = getBypgid(jobs_head,pgid);
						// check the return status
						if( WIFEXITED(status) ){
							jobs_head = deleteValue(jobs_head, pgid);
						} else if (WIFCONTINUED(status)){
							ptr->stat_change = 1;
							ptr->status = 2;
						} else if (WIFSIGNALED(status)){
							ptr->stat_change = 1;
							ptr->status = 3;
								
						}else if( WIFSTOPPED(status) ){	
							tcsetpgrp(STDIN_FILENO,getpgid(shell_pgid));
							ptr->stat_change = 1;
							ptr->status = 1;			
						}
					}
				} else {//if it is background, then give terminal control to the shell
					printf("Running: %s\n", pg_name.value);
				}
			}
			if( f_out_b ){free(filename[1]);}
			if( f_in_b ) {free(filename[0]);}
			if( f_err_b ){free(filename[2]);}
			//free(args);
			free(program);
			
			// restore standard in out and unblock the signal set
			dup2(saved_stderr, 2);
			dup2(saved_stdout, 1);
			dup2(saved_stdin, 0);
			sigprocmask(SIG_UNBLOCK, &block_set, NULL);
		}
	}
	destructList(jobs_head);	
}
