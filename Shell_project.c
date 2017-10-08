/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c
#include "string.h" 

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */
#define AZUL "\x1b[1;34m"
#define ROJO "\x1b[31;1;1m"
#define VERDE "\x1b[1;32m"
#define GRIS "\x1b[1;37m"

job *lista; //Creamos un puntero a una lista a partir de job_control.h 
job *tarea; //creamos un proceso

void listado(){ //listamos los jobs
	if(empty_list(lista) == 1){ //empty_list devuelve 1 si la lista está vacia
		printf("%s No hay ninguna tarea en segundo plano \n", ROJO); //si la lista esta vacia aparecera este mensaje
	} else {
		print_job_list(lista);	//pinta la lista
	}
}
void suspender(){
	block_SIGCHLD();	
	kill(tarea->pgid,SIGSTOP); //le mandamos el pid de la tarea y la seña para suspender dicha tarea
	unblock_SIGCHLD();
}

//****Metodo para los procesos zombie****
void manejador(){
	int status,info;
	enum status status_res;
	
	
	if(empty_list(lista)){
 		printf("%sLa lista está vacia\n", ROJO);
	} else {	
		job *aux = lista->next;
		
		
		while(aux!=NULL){ 
											       									  
			if(waitpid(aux->pgid,&status,WUNTRACED|WNOHANG > 0)){
				status_res=analyze_status(status,&info);
				if(status_res == SIGNALED || status_res == EXITED ){ //SIGNALED, que si termina por ejemplo con un kill
						job *aux2 = aux->next;
						printf ("%sTarea eliminada, con pid %d \n", VERDE, aux->pgid); //por ejemplo para sleep 5 &						
						block_SIGCHLD();
						delete_job(lista,aux);
						free_job(aux);
						aux=aux2;
						unblock_SIGCHLD();
						
				} else if (status_res == SUSPENDED){ //si el estado de la tarea es suspendido (stopped)
					block_SIGCHLD();
					aux->state = STOPPED;
					printf("%sHa sido bloqueado el proceso con pid : %d\n",ROJO, aux->pgid);
					aux = aux->next;
					unblock_SIGCHLD();
				}		
						
			} else {
				aux = aux->next;				
			}
			
		}	
	}
}

void ayuda(){
	printf("%sLos comandos incorporados en el Shell son: \n \n", AZUL);
	printf("%sjobs-> Devuelve el listado de las tareas en segundo plano o que estan suspendidas \n \n", AZUL);
	printf("%scd X-> Permite cambiar de directorio, el segundo argumento nos indica la direccion, si queremos volver atras hay que escribir \"cd ..\" \n \n", AZUL);
	printf("%sfg X-> Pone una tarea que está en segundo plano o suspendida en primer plano.\n Si  \"fg\" lo pasamos sin argumento pone en primer plano el primero en la lista  \n \n", AZUL);
	printf("%sbg X-> Pone una tarea que está suspendida, a ejecucion en segundo plano.\n El argumento es el numero dentro de la lista, si no le pasamos argumento coge el primero de la lista \n \n", AZUL);
}

void primer_plano(char *arg){	//arg es el identificativo el lista de jobs: 1, 2... 		
	int posicion;	//indica la posicion del proceso en la lista de jobs
	job *ac = NULL, *n = NULL;
	int st, info;
	enum status st_res;

	if(arg == NULL){
		posicion = 1;		
	} else {
		posicion = atoi(arg);	//el arg sera en tipo char: 1, 2, 3 ... el identificativo en la lista de jobs
	}


	if(posicion <= list_size(lista)){
		block_SIGCHLD();		
		ac = get_item_bypos(lista,posicion);	// me devuelve el proceso de la lista, dada la lista y la posicion				
		set_terminal(ac->pgid);
		if(ac->state == STOPPED){		//cambiamos el estado background a foreground
			killpg(ac->pgid,SIGCONT);
		}
		ac->state = FOREGROUND;
		waitpid(ac->pgid,&st,WUNTRACED);	//y esperamos a que termine
		st_res = analyze_status(st,&info);
		printf("%s pid:%d, comando:%s, %s, info:%d \n",state_strings[ac->state],ac->pgid,ac->command,status_strings[st_res],info);

		if(st_res == SUSPENDED){	
			ac->state = STOPPED;
		} else {
			delete_job(lista, ac);
		}
		
		set_terminal(getpid());//devolvemos el terminal al terminal
		unblock_SIGCHLD();
		
	} else {
		printf("%sNo existe ese argumento en la lista \n", ROJO);
	}

}
void segundo_plano(char *arg){	
	//listado();
	int posicion; //indica la posicion del proceso en la lista de jobs
	job *ac;

	if(arg == NULL){
		posicion = 1;	
	} else {
		posicion = atoi(arg);
	}

	if(posicion <= list_size(lista)){
		ac=get_item_bypos(lista,posicion);
			
		if(ac->state == STOPPED){	// Si está suspendido, lo pongo en background y le mando la señal para que siga						
			block_SIGCHLD();			
			ac->state = BACKGROUND;			
			unblock_SIGCHLD();			
			printf("%sPoniendo en background... pid:%d\n",VERDE,ac->pgid);			
			killpg(ac->pgid,SIGCONT);//le damos la señal para que continue ejecutandose
			
			
		} else if (ac->state == BACKGROUND) {
			printf("%sEsta tarea ya esta en segundo plano \n", ROJO);
		}
		
	} else{
		printf("%sNo existe ese argumento en la lista \n", ROJO);
	}

}


// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void){

	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	
	//********VARIABLES INCIALES	
	int i = 0;	//Contador que utilizaremos para primer inicio para que salga el ayuda de comandos
	lista = new_list("lista"); //new_list definido en control.h	

	signal(SIGCHLD, manejador); //SIGCHLD, cuando el hijo termina envia una señal al padre, por defecto se ignoraria y el proceso quedaria zombie pero nosotros hacemos que eso no ocurra	
	signal(SIGTSTP,suspender);//---------------------señal de suspender, implementada, o usar SIG_DFL en lugar de suspender				
	ignore_terminal_signals();	

	while (1){  /* Program terminates normally inside get_command() after ^D is typed*/   		

		if(i == 0){
			i=1;
			printf("%sPara saber los comandos implementados escriba \"ayuda\" \n", GRIS);
			
		}

		printf("%sadrian@Shell $~: ", GRIS);
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command, va al final y comprueba si hay return, como no lo hay regresa al principio del bucle



		//Implementacion de comandos internos
		if(strcmp(args[0], "jobs") == 0){		
			listado();
			continue;
		}else if(strcmp(args[0], "cd") == 0){
			if(chdir(args[1]) == -1){
				printf("%sError. El directorio \"%s\" no existe\n",ROJO, args[1]);			
			}
			continue;
		}else if(strcmp(args[0], "fg") == 0){			
			primer_plano(args[1]);			
			continue;
		}else if(strcmp(args[0], "bg") == 0){			
			segundo_plano(args[1]);
			continue;
		}else if(strcmp(args[0], "ayuda") == 0){
			ayuda();
			continue;		
		}
		//******************************	


		pid_fork = fork();
		
		if (pid_fork == 0){ //hijo
			restore_terminal_signals();
			pid_fork = getpid();	//obtenemos el pid 
			new_process_group(pid_fork); //creamos un nuevo grupo de procesos, grupos para diferenciar los comandos o aplicaciones externas, cada comando en su grupo de procesos 				//background == 0 es que es foreground
			if(background == 0 ) set_terminal(pid_fork); //sede al proceso el terminal 			
			execvp(args[0], args);	//ejecuta el comando, si hay error ejecuta el print de abajo	
			printf("%sError. El comando \"%s\" no existe \n",ROJO,args[0]);			
			exit(127); //Señal con la que nostros queremos que termine
			
		} else { //padre
			new_process_group(pid_fork);
			if(background == 0){ //quiere decir que el comando no va seguido de &
				set_terminal(pid_fork);	//Asignamos el terminal			
				pid_wait = waitpid(pid_fork, &status, WUNTRACED);//WUNTRACED, no esperar si el hijo está parado
 				status_res = analyze_status(status, &info); // Analizamos el estado de terminación				
								
				if(status_res == SUSPENDED){ //controla que si lo paramos (por ejemplo nano), se añada a la lista con estado STOPPED
					tarea = new_job(pid_fork, args[0], STOPPED);
					block_SIGCHLD();								
					add_job(lista,tarea);				
					unblock_SIGCHLD();				
				}

                		printf("%sForeground pid: %d, command: %s, %s, info: %d \n",VERDE, pid_wait, args[0],status_strings[status_res], info); //Mostramos el mensaje de estado del proceso 	
				set_terminal(getpid());	//el shell recupera el terminal
			
			}else{ //que el comando va seguido de un &, por lo tanto se está ejecutando en BG ----BACKGROUND
				tarea = new_job(pid_fork, args[0], BACKGROUND); //new_job es una funcion de job_control.h
				
				block_SIGCHLD();				
				add_job(lista, tarea);
				unblock_SIGCHLD();

				printf("%sBackground running... pid: %d. Ejecutando el comando '%s' en background\n",VERDE, pid_fork, args[0]);
				continue;		
			}
					
		}

	} // end while
}
