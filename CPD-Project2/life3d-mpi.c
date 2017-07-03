#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "life3d.h"
#define BUFFER_SIZE 50000000

int main(int argc, char *argv[]){
   
   int x, y, z;
   int p;
   int id;
   int SIZE_CUBE;
   int iter;
   double elapsed_time;
   int BUFFER_MAX = 500000;
   
   
   char line[100];
   char * points;

   /* Initialize MPI  */
   
   MPI_Init(&argc, &argv); // Basicamente é para remover os argumentos de entrada relativos ao MPI.
   MPI_Barrier(MPI_COMM_WORLD); // Enquanto todos os processos nao entrarem na barreira, nenhum pode sair. Serve para sincronizar no inicio do programa.
   elapsed_time = -MPI_Wtime();
   
   /* Get the rank of the process and the size of the world (total number of processes) */
   
										//MPI_COMM_WORLD---> COMUNICADOR DEFAULT. ENGLOBA TODOS OS PROCESSOS!
   MPI_Comm_rank (MPI_COMM_WORLD, &id); //Vai buscar o rank do processo para a variavel id
   MPI_Comm_size (MPI_COMM_WORLD, &p); //Vai buscar o numero de processos que estao a ser utilizados para a variavel p

   
   /* Read the file (only process 0) with the initial cells distribution and try
    * to compute boundaries in order to guarantee a fair distribution of alive cells among processes 
    * (This may not be necessary since the procedure is deterministic and each process can do this autonomously)
    * (Consider using process 0 to send all the cells to the various processes)
    */
    
   int boundaries[2*p]; //Estas variaveis sao definidas fora do proc 0 porque todos os outros proc vao usa-las 
   int one_row = 0;
   int totcells;

   memset(&boundaries, -1, 2*p*sizeof(int));
      
   if (id == 0){ // SE FOR O PROCESSO 0 

        if (argv[1] == NULL) {
             printf("Error: file pointer is null.");
             exit(1);
        } 
   
        if (argv[2] == NULL) {
            printf("Error: Number of Iterations not specified.");
            exit(1);
        }
            
       FILE* file = fopen (argv[1], "r");
       fgets(line, sizeof(line), file);
       SIZE_CUBE = atoi(line);
       iter=atoi(argv[2]);
       
       int count[SIZE_CUBE];
       memset(&count, 0, SIZE_CUBE*sizeof(int)); 
       
       totcells = 0; // COUNT TOTAL NUMBER OF INITIAL LIVING CELLS
       
       while (fgets(line, sizeof(line), file)) {
            points = strtok(line, " ");
            x = atoi(points);
            points = strtok (NULL, " ");
            y= atoi(points);
            points = strtok (NULL, " ");
            z = atoi(points);
            count[x] = count[x] + 1; // Verificar quantas celulas existem por cada fatia
           // printf("x = %d , count[x] = %d\n",x,count[x]);
            totcells++;
       } 
      
       fclose (file); 
       
       
       int cell_proc = totcells/p; // NUMBER OF CELLS PER PROCESS (IDEALLY)
       
       /*
       for (int i = 0; i < SIZE_CUBE; i++){
            
            printf("%d : %d\n", i, count[i]);
            fflush(stdout);  
       }
       */
       //printf("-----\n %d \n", cell_proc);
       //fflush(stdout);

       
       int average = totcells/SIZE_CUBE; //Numero medio de celulas por cada slice. NUMBER OF CELLS PER SLICE
       boundaries[0] = 0;
       double perc;
       
       if (p > SIZE_CUBE){
            printf("Too many processes");
            exit(1);
       }
       else if (SIZE_CUBE > 2*p && average >= 2*p) //escolha do perc para fazer melhor a distribuição das celulas por processo
            perc = 0.90;
       else
            perc = 0.65;
       
       int partial_sum = 0;
       int partial_total = 0;
       int partials[p];
       int j = 1;
       int k = 0;
	   
	   
	   // DENTRO DOS ARRAY DOS BOUNDARIES ESTÁ A CAMADA OU AS CAMADAS QUE CADA PROCESSADOR VAO TRABALHAR!
	   //VAI DIVIDIR UM NUMERO JUSTO DE CELULAS POR PROCESSO EM QUE OS BOUNDARIES VAO SER OS LIMITES(LINHAS LIMITES)
	   
	   for (int i = 0; i < SIZE_CUBE; i++){ // vai ser analisado o numero de celulas vivas na camada i
            partial_sum += count[i]; // numero de celulas na camada i
           // printf("PARTIAL_SUM = %d COUNT[i] = %d\n", partial_sum, count[i]);
			

            if (j == 2*p-1 || i == SIZE_CUBE-1){
                boundaries[j] = SIZE_CUBE-1;
                break;
            }
            
            partial_total += count[i];
            //printf("PARTIAL_TOTAL = %d COUNT[i] = %d\n", partial_total, count[i]);
            if (partial_sum >= perc*cell_proc){ //Numero de celulas vivas na camada i for demasiado elevado, basicamente ele vai dividir essa camada em dois! se o numero de celulas na camada i for maior que o numero medio de celulas por processador
                boundaries[j] = i; 
                boundaries[j+1] = i+1;
                j += 2;
                partials[k] = partial_sum;
                k++;
                partial_sum = 0;
            } 
       }
       
       
       for(j=0;j<2*p;j++){
		   printf("BOUNDARIES[%d] = %d\n",j,boundaries[j]);
	   }
       
       /*
       int i;
       for(i=0;i<SIZE_CUBE;i++){

            if(j==2*p-1 || i==SIZE_CUBE-1){
              boundaries[j]=SIZE_CUBE-1;
              break;
            }
            partial_total+=count[i];
            
            if(partial_sum + count[i]>cell_proc && cell_proc-partial_sum){
              boundaries[j]=i;
              boundaries[j+1]=i+1;
              j+=2;
              partials[k]=partial_sum+count[i];
              k++;
              cell_proc=(totcells-partial_total)/(p-k);
              partial_sum=0;
            } else if(partial_sum + count[i] > cell_proc){
              boundaries[j]=i-1;
              boundaries[j+1]=i;
              j+=2;
              partials[k]=partial_sum;
              k++;
              cell_proc=(totcells-partial_total+count[i])/(p-k);
              partial_sum=count[i];
            } else partial_sum +=count[i];
       }
       */
       /* Check if boundaries have been computed correctly */
       
       /*
       for (int i = 0; i < 2*p; i++){
            if (boundaries[i] == -1){
                printf("Boundaries computation failed!");
                fflush(stdout);
                exit(3);
            }
       }
  
       partials[p-1] = totcells-partial_total;
       
       
       printf("-------------- BOUNDARIES ------------\n");
       
       for (int i = 0; i < 2*p; i++){
            
            printf("%d ", boundaries[i]);
            fflush(stdout);  
       }
       printf("\n");
       
       printf("--------------PARTIALS ---------------\n");
       
       for (int i = p-1; i >= 0; i--){
            
            printf("%d ", partials[i]);
            fflush(stdout);  
       }
       printf("\n");
       
       */
      
   }
    
   /* Broadcast boundary array to all nodes along with the size of the cube */ 
   
   MPI_Bcast(boundaries, 2*p, MPI_INT, 0, MPI_COMM_WORLD); //Broadcast o array dos boundaries com tamanho 2*p do root 0 para todos os outros processos
   MPI_Bcast(&SIZE_CUBE, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&iter, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&totcells, 1, MPI_INT, 0, MPI_COMM_WORLD);
   
   //ATE AQUI ELES RECEBEREM TODA A INFORMAÇÃO NECESSARIA PARA FAZEREM AS SUAS TABELAS


   /*Check if some processes only get one row */

  //APENAS PARA VERIFICAR SE ALGUM PROCESSO TEM APENAS UMA LINHA 
  for (int i = 0; i < 2*p; i = i+2){  // APARTIR DE AGORA ESTAO TODOS OS PROCESSADORES A TRABALHAR O MESMO CODIGO! 	 
          if (boundaries[i] == boundaries[i+1]){ // SE FIZERMOS UM PRINT NESTE LOOP TODOS OS PROCESSADORES VAO FAZER ESSE PRINT COM O MESMO RESULT!
            one_row = 1;
            break;
          }
  }

   //These are the boundaries of the current process ---> USE THESE VARIABLES IN THE MAIN ALGORITHM WHEN NEEDED
   int bound_low = boundaries[2*id]; //BASICAMENTE INDICA AS CAMADAS SUPERIORES E INFERIORES QUE O PROCESSO ESTA A TRABALHAR!
   int bound_high = boundaries[2*id+1]; //SE UM PROCESSO ESTA APENAS COM UMA CAMADA(PORQUE OBVIAMENTE ESTA É GRANDE) ENTAO O SEU BOUND_LOW E HIGH VAI SER O MESMO! 
  /* 
   printf("|||||||||||||||||||||||||||||||||||||\n");
   printf("PROCESS %d ------> BOUND_lOW = %d , BOUND_HIGH = %d\n",id,bound_low, bound_high);
   printf("|||||||||||||||||||||||||||||||||||||\n");
	exit(1);
   */
   /* x coordinates of the upper and lower ghost rows (rows sent from neighbours) */
   int ghost_up = mod(bound_high+1, SIZE_CUBE); //CAMADAS DOS PROCESSOS VIZINHOS!   
   int ghost_down = mod(bound_low-1, SIZE_CUBE); //VAO SER ENVIADAS PARA OS PRCESSOS VIZINHOS COM AS BOUND_LOW E HIGH!
   
   /*
   printf("|||||||||||||||||||||||||||||||||||||\n");
   printf("PROCESS %d ------> GHOST_UP = %d , GHOST_DOWN = %d\n",id,ghost_up, ghost_down);
   printf("|||||||||||||||||||||||||||||||||||||\n");
	exit(1);
   */
   /* Insert the cells in first_batch inside the right tables and compute the first generation */
  int INITIAL;
  if (totcells > 300000){
      INITIAL = 1000003;
  }else{
      INITIAL = 103001;
  }

  int SIZE_MAIN_TABLE = INITIAL/p;  //The main table has more entries //27741
  int SIZE_SIDE_TABLE = INITIAL/p;   //The tables containing only ghost rows have less entries on average so the table can be smaller

  cell * cur_main_table = malloc(SIZE_MAIN_TABLE*sizeof(cell)); //CRIAÇÃO DA TABELA PRINCIPAL PARA CADA PROCESSO
  memset(cur_main_table, -1, SIZE_MAIN_TABLE*sizeof(cell));

  cell * low_main_table = malloc(SIZE_SIDE_TABLE*sizeof(cell)); //??? TABELA PARA ARMAZENAR A BOUND_LOW PARA MANDAR PARA O GHOST_UP ??? OU TABELA RECEBIDA DA GHOST_UP ATUALIZADA COM AS CELULAS VIVAS
  memset(low_main_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * high_main_table = malloc(SIZE_SIDE_TABLE*sizeof(cell)); // ??? TABELA PARA ARMAZENARO BOUND_HIGH PARA MANDAR PARA O GHOST_DOWN
  memset(high_main_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * aux_main_table = malloc(SIZE_MAIN_TABLE*sizeof(cell)); // TABELA AUXILIAR PARA GUARDAR AS CELULAS VIVAS DA PROXIMA ITERAÇÃO
  memset(aux_main_table, -1, SIZE_MAIN_TABLE*sizeof(cell));

  cell * aux_low_table = malloc(SIZE_SIDE_TABLE*sizeof(cell)); // TABELA AUXILIAR PARA GUARDAR AS CELULAS VIVAS DA PROXIMA ITERAÇÃO DO BOUND_LOW PARA ENVIAR PARA O GHOST_UP
  memset(aux_low_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * aux_high_table = malloc(SIZE_SIDE_TABLE*sizeof(cell)); // TABELA AUXILIAR PARA GUARDAR AS CELULAS VIVAS DA PROXIMA ITERAÇÃO DO BOUND_HIGH PARA ENVIAR PARA O GHOST_DOWN
  memset(aux_high_table, -1, SIZE_SIDE_TABLE*sizeof(cell));
	
  cell * high_next_table = malloc(SIZE_SIDE_TABLE*sizeof(cell)); // 
  memset(high_next_table, -1, SIZE_SIDE_TABLE*sizeof(cell));

  cell * low_next_table = malloc(SIZE_SIDE_TABLE*sizeof(cell));
  memset(low_next_table, -1, SIZE_SIDE_TABLE*sizeof(cell));


  FILE *file = fopen (argv[1], "r");
  fgets(line, sizeof(line), file);



  if (one_row != 1){ // SE TODOS OS PROCESSOS TIVERAM MAIS QUE UMA CAMADA (MAIS QUE UMA LINHA DE X)
          while (fgets(line, sizeof(line), file)) {
           
              points = strtok(line, " ");
              x = atoi(points);
              points = strtok (NULL, " ");
              y= atoi(points);
              points = strtok (NULL, " ");
              z = atoi(points);
                        
              if (x >= boundaries[2*id] && x <= boundaries[2*id+1]) { // CADA PROCESSO VAI INSERIR AS SUAS CELULAS VIVAS NA TABELA PRINCIPAL!
                  insert(aux_main_table, x, y, z, SIZE_MAIN_TABLE); // INSERT É FEITO NORMALMENTE NA MAIN TABLE. TODAS AS CELULAS SAO LA COLOCADAS
              }    			  
			  
			  //ESTAS CONDIÇÕES PARA VERIFICAR QUAIS SAO AS LOWER E HIGH BOUND DE CADA PROCESSO E INSERIR AS CELULAS VIVAS NAS RESPECTIVAS TABELAS
				
              if (x == mod(boundaries[2*id]-1, SIZE_CUBE) || x == mod(boundaries[2*id]-2, SIZE_CUBE) || x == boundaries[2*id]) { 
				  //CRIAÇÃO DA TABELA LOW USANDO 2 GHOST ROWS
                  insert(low_main_table, x, y, z, SIZE_SIDE_TABLE); //INSERT NA LOW MAIN TABLE COM A COM A LOW BOUND DO ACIMA
              } 

              if (x==mod(boundaries[2*id+1]+1, SIZE_CUBE) || x==mod(boundaries[2*id+1]+2, SIZE_CUBE) || x == boundaries[2*id+1]){
				  //CRIAÇÃO DA TABELA HIGH USANDO 2 GHOST ROWS
                  insert(high_main_table, x, y, z, SIZE_SIDE_TABLE); // INSER NA HIGH TABLE COM A HIGH BOUND DO PROCESSA ABAIXO
              }

          }
          
    
  } else { // SE HOUVER ALGUM PROCESSO QUE CONTENHA APENAS UMA CAMADA. ISTO SIGNIFICA QUE O TAMANHO DO CUBO É PEQUENO!
		
          while (fgets(line, sizeof(line), file)) {
                        
              points = strtok(line, " ");
              x = atoi(points);
              points = strtok (NULL, " ");
              y= atoi(points);
              points = strtok (NULL, " ");
              z = atoi(points);
                        
              if ((x >= boundaries[2*id] && x <= boundaries[2*id+1]) || (x == mod(boundaries[2*id]-1, SIZE_CUBE)) || (x==mod(boundaries[2*id+1]+1, SIZE_CUBE))) { //IF DE SAVE! TEM AS CONDIÇÕES PARA SE O PROCESSO TEM APENAS 1 CAMADA OU SE O PROCESSO TEM MAIS QUE UMA CAMADA 
                  insert(aux_main_table, x, y, z, SIZE_MAIN_TABLE); // FAZ O INSERT NORMAL NA TABLE HASH!
              }    

          }

  }
       
  fclose (file); 

  int i;
  cell *aux;
  cell current;
  int neighbours;
  int proc_up = mod(id + 1, p); //PROCESSO QUE ESTA SEGUIR AO ATUAL
  int proc_down = mod(id-1, p); //PROCESSO ANTERIOR!

  if (one_row != 1){ //SE TODOS OS PROCESSOR TIVERAM MAIS QUE UMA LINHA DE X!     ??????????????????????????????????????????????????????????????????'
                    for (i=0; i<iter; i++){
                      aux=cur_main_table; 
                      cur_main_table = aux_main_table; //A MAIN TABLE PASSA A TER AS CELULAS VIVAS ATUALIAZADAS
                      aux_main_table=aux; //A AUX TABLE VAI TER AS CELULAS VIVAS DA PROXIMA GERAÇÃO
                      
                      free_list(aux_main_table, SIZE_MAIN_TABLE); 
                    
                      int temp;
                      int num_up = 0;
                      int num_down = 0;
												
                      if (i % 2 == 0){ //SE A ITERAÇÃO FOR PAR
							
                        for(temp=0; temp < SIZE_MAIN_TABLE; temp++){ // VAMOS A CADA POSIÇÃO DA TABELA
                          current = cur_main_table[temp]; //PEGAMOS NUMA CELULA
                          if(current.key!=-1){ //VERIFICAMOS: SE ELA NAO ESTA MORTA, VAI MANDAR PARA PARA FUNÇÃO EM BAIXO:
											  //QUE VAI VERIFICAR OS VIZINHOS VIVOS DESSA CELULA. (É ENVIADA A LOW_MAIN_TABLE QUE CORRESPONDE AO GHOST UP DESTE PROCESSO, E A HIGH MAIN TABLE QUE CORRESPONDE AO GHOST DOWN DESTE PROCESSO!							
                            neighbours= num_alive_neighbours_af(cur_main_table, aux_main_table, current.x, current.y, current.z, low_main_table, high_main_table, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_SIDE_TABLE, SIZE_CUBE);
                            if (neighbours>=2 && neighbours<=4){

                              insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE); //INSERE A CELULA VIVA NA AUX TABLE PARA A PROXIMA GERAÇAO

                            }

                            while (current.next != NULL) { //SE NA POSIÇÃO HASH DA TABELA EXISTIR UMA LINKED LIST!
                              current=*current.next;
                              neighbours= num_alive_neighbours_af(cur_main_table, aux_main_table, current.x, current.y, current.z, low_main_table, high_main_table, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_SIDE_TABLE, SIZE_CUBE);
                              if (neighbours>=2 && neighbours<=4){

                                insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

                              }
                            } 
                          }
                        }


                          free_list(high_next_table, SIZE_SIDE_TABLE);
                          free_list(low_next_table, SIZE_SIDE_TABLE);


                        for(temp=0; temp < SIZE_SIDE_TABLE; temp++){ // ATUALIZAR A HIGH MAIN TABLE TAL COMO SE FEZ PARA MAIN
                          current = high_main_table[temp]; //VAMOS Á POSIÇÃO CURRENT
                          if(current.key!=-1){ //SE A CELULA ESTIVER VIVA
											   //VAI VERIFICAR OS VIZINHOS QUE ESTAO VIVOS.
                            neighbours= num_alive_neighbours_ah(high_main_table, high_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE, SIZE_CUBE, bound_high);

                            if (neighbours>=2 && neighbours<=4 && current.x == mod(bound_high+1, SIZE_CUBE)){ //ELE VAI VERIFICAR TODOS OS VIZINHOS VIVOS DE TODAS AS CELULAS VIVAS DA HIGH MAIN TABLE
                              insert(high_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);	  //MAS SO ENTRA NESTE IF E INSERE A CELULA VIVA PARA A PROXIMA GERAÇÃO SE O X CORRESPONDER
																											  //AO GHOST UP DO PROCESSO (LINHA DO MEIO DA HIGH MAIN TABLE)
                            }

                            while (current.next != NULL) { //SE HOUVER UMA LINKED LIST NA POSIÇÃO DA TABELA, REALIZA O MESMO PROCEDIMENTO QUE ACIMA
                              current=*current.next;
                              neighbours= num_alive_neighbours_ah(high_main_table, high_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE, SIZE_CUBE, bound_high);  
                        
                              if (neighbours>=2 && neighbours<=4 && current.x == mod(bound_high+1, SIZE_CUBE)){

                                insert(high_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);

                              }
                            } 
                          }
                        }
							
						  //IGUAL AO PROCEDIMENTO ACIMA MAS AGORA PARA O LOW MAIN TABLE!
                          for(temp=0; temp < SIZE_SIDE_TABLE; temp++){
                          current = low_main_table[temp];
                          if(current.key!=-1){
                            neighbours= num_alive_neighbours_al(low_main_table, low_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE, SIZE_CUBE, bound_low);
                            if (neighbours>=2 && neighbours<=4 && current.x == mod(bound_low-1, SIZE_CUBE)){ //ENTRA NESTE IF SE O X CORRESPONDER AO GHOST DOWN DO PROCESSO (LINHA DO MEIO DA LOW MAIN TABLE)!

                              insert(low_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);

                            }

                            while (current.next != NULL) {
                              current=*current.next;
                              neighbours= num_alive_neighbours_al(low_main_table, low_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE, SIZE_CUBE, bound_low);            
                              if (neighbours>=2 && neighbours<=4 && current.x == mod(bound_low-1, SIZE_CUBE)){

                                insert(low_next_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);

                              }
                            } 
                          }
                        } 
						

                      } else { //SE A ITERAÇÃO FOR IMPAR!!!!!!!!
						  
						  
                                free_list(high_main_table, SIZE_SIDE_TABLE);
                                free_list(low_main_table, SIZE_SIDE_TABLE);

                                for(temp=0; temp < SIZE_MAIN_TABLE; temp++){ //VAI Á TABELA PRINCIPAL DO PROCESSO
                                current = cur_main_table[temp]; //PERCORRER CADA POSIÇÃO
                                if(current.key!=-1){ // SE A CELULA NAO ESTIVER MORTA
									
                                  neighbours= num_alive_neighbours_a2(cur_main_table, aux_main_table, aux_low_table, aux_high_table, low_main_table, high_main_table, current.x, current.y, current.z,
																	 low_next_table, high_next_table, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_CUBE, SIZE_SIDE_TABLE, &num_down, &num_up);
								  
                                  if (neighbours>=2 && neighbours<=4){// SE ENTRAR ENTAO É PORQUE A CELULA VIVA VAI FICAR VIVA NA PROXIMA GERAÇÃO
									
                                    insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

                                    if (current.x == bound_low || current.x == mod(bound_low+1, SIZE_CUBE)){ //SE FOR O LIMITE DE BAIXO ( LINHA BOUND LOW) OU O LIMITE DE BAIXO MAIS 1
                                      insert(aux_low_table, current.x, current.y, current.z, SIZE_SIDE_TABLE); // ?????????????????????????????????'
                                      num_down = num_down + 1;
                                    }

                                    if (current.x == bound_high || current.x == mod(bound_high-1, SIZE_CUBE)){
                                      insert(aux_high_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                      num_up = num_up + 1;
                                    }

                                    if (current.x == bound_low){
                                      insert(low_main_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                    }

                                    if (current.x == bound_high){
                                      insert(high_main_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                    }
                                  }

                                  while (current.next != NULL) { // IGUAL MAS PARA LINKED LIST!
                                    current=*current.next;
                                    neighbours= num_alive_neighbours_a2(cur_main_table, aux_main_table, aux_low_table, aux_high_table, low_main_table, high_main_table, current.x, current.y, current.z, low_next_table, high_next_table, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_CUBE, SIZE_SIDE_TABLE, &num_down, &num_up);
                                    if (neighbours>=2 && neighbours<=4){


                                      insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

                                      if (current.x == bound_low || current.x == mod(bound_low+1, SIZE_CUBE)){
                                        insert(aux_low_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                        num_down = num_down + 1;
                                      }

                                      if (current.x == bound_high || current.x == mod(bound_high-1, SIZE_CUBE)){
                                        insert(aux_high_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                        num_up = num_up + 1;
                                      }

                                      if (current.x == bound_low){
                                        insert(low_main_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                      }

                                      if (current.x == bound_high){
                                        insert(high_main_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
                                      }
                                    }
                                  } 
                                }
                              }
                    

                                free_list(high_next_table, SIZE_SIDE_TABLE);
                                free_list(low_next_table, SIZE_SIDE_TABLE);

                                MPI_Request request_up;
                                int buffer_recv_up[BUFFER_MAX];
                                int TAG_SDOWN_RUP = 0;
                                
                                //int MPI_Irecv(void *buf,int count, MPI_Datatype datatype,int source,int tag,MPI_Comm comm,MPI_Request *request;
								//PROCESSADOR VAI RECEBER DO PROCESSADOR ACIMA DELE. VAI RECEVER DO PROC_UP COM A TAG = 0
								//É USADO ESTE EM VEZ DO MPI_RECV PARA EVITAR DEADLOCKS, AO BLOQUEAREM-SE UNS AOS OUTROS!
                                MPI_Irecv(buffer_recv_up, BUFFER_MAX, MPI_INT, proc_up, TAG_SDOWN_RUP, MPI_COMM_WORLD, &request_up);
                                
						
                                MPI_Request request_down;
                                int buffer_recv_down[BUFFER_MAX];
                                int TAG_SUP_RDOWN = 1;
								
								//VAI RECEBER DO PROCESSO ABAIXO COM A TAG = 1
                                MPI_Irecv(buffer_recv_down, BUFFER_MAX, MPI_INT, proc_down, TAG_SUP_RDOWN, MPI_COMM_WORLD, &request_down);
								
							
								
                                int send_up[num_up*3]; //NUM_UP É O NUMERO DE CELULAS QUE SE ENCONTRAVAM MORTAS NAS HIGH E LOW MAIN TABLE E QUE VAO ESTAR VIVAS NA PROXIMA GERAÇÃO
                                serialize(aux_high_table, SIZE_SIDE_TABLE, send_up, num_up*3); // VAI ENFIAR AS COORDENADAS DA TABELA NO ARRAY!
                                
                                /*
                                if(id==2){
									for (int t=0; t<num_up*3;t++){
										printf("SEND_UP[%d] = %d\n",t, send_up[t]);
									}
								}
								exit(1);
								* 
								* 
								* 
								* 
                                if(id == 2){
										printf("NUM_UP = %d\n",num_up);
                                }
                                exit(1);
                                */
                                
								//int MPI_Send(void *buf,int count,MPI_Datatype datatype,int dest,int tag,MPI_Comm comm);
								//VAI ENVIAR A HIGH MAIN TABLE ENFIADA NUM ARRAY PARA O PROCESSADOR ACIMA
                                MPI_Send(send_up, num_up*3, MPI_INT, proc_up, TAG_SUP_RDOWN, MPI_COMM_WORLD);
								
                                int send_down[num_down*3]; 
                                serialize(aux_low_table, SIZE_SIDE_TABLE,send_down , num_down*3);
								
								//VAI ENVIAR A LOW MAIN TABLE ENFIADA NUM ARRAY PARA O PROCESSADOR ABAIXO
                                MPI_Send(send_down, num_down*3, MPI_INT, proc_down, TAG_SDOWN_RUP, MPI_COMM_WORLD);


                                MPI_Status status_up;
                                MPI_Status status_down;
								
								
								//MPI_Wait returns after the operation identified by request completes.
								//MPI_WAIT VAI RETURN APENAS QUANDO O REQUEST ESTIVER TERMINADO. O STATUS VAI CONTER O ESTADO DA OPERAÇÃO!
                                MPI_Wait(&request_up, &status_up);
                                MPI_Wait(&request_down, &status_down);



                                int num_recv_up;
                                int num_recv_down;
								
								//int MPI_Get_count(MPI_Status *status,MPI_Datatype datatype,int *count);
								//VAI RETURNAR O NUMERO DE ENTRIES RECEBIDOS!
								//CONTA O NUMERO DE ELEMENTOS RECEBIDOS DO HIGH PROC E DO LOW PROC
								//VAI SERVIR COMO SIZE DO BUFFER!																??????????????????????????????????????????????????
                                MPI_Get_count(&status_up, MPI_INT, &num_recv_up);
                                MPI_Get_count(&status_down, MPI_INT, &num_recv_down);
								
								// VAI INSERIR OS NOVOS ELEMENTOS NA HIGH E LOW TABLE 
                                insert_table(high_main_table, SIZE_SIDE_TABLE, buffer_recv_up, num_recv_up);
                                insert_table(low_main_table, SIZE_SIDE_TABLE, buffer_recv_down, num_recv_down);
                                
                                free_list(aux_high_table, SIZE_SIDE_TABLE);
                                free_list(aux_low_table, SIZE_SIDE_TABLE);

                      }

                            if (i == iter-1){
                     
                            cell * order[SIZE_CUBE];
                            for (int l = 0; l<SIZE_CUBE; l++)
                                order[l] = NULL;
							
							
							//CADA PROCESSO VAI ORGANIZAR A SUA TABELA E METER AS COORDERNADAS NUMA LINKED LIST
                            ordered_list(aux_main_table, order, SIZE_MAIN_TABLE, SIZE_CUBE);
                        	
                        	//FICA PRONTO PARA FAZER O PRINT DE CADA PROCESSO
                            char *buffer = malloc(BUFFER_SIZE);
                            print_list(order, SIZE_CUBE,buffer);
                           
                            if (id == 0){
                              printf("%s", buffer);
                              fflush(stdout);
                              free(buffer);
                              char *buffer_ord = malloc(BUFFER_SIZE);

                              for (int q = 1; q < p; q++){
                                MPI_Status stat;
                                //int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,MPI_Comm comm, MPI_Status *status)
								//VAI RECEBER ORDENADAMENTE CADA PROCESSO IMPRIMINDO A SUA LINKED LIST!
                                MPI_Recv(buffer_ord, BUFFER_SIZE, MPI_CHAR, q, 9, MPI_COMM_WORLD, &stat);
                                printf("%s", buffer_ord);
                                fflush(stdout);
                              }
                              free(buffer_ord);

                            } else {
							  // TODOS OS PROCESSOS VAO MANDAR PARA O PROCESSO 0 A SUA TABELA
                              MPI_Send(buffer, strlen(buffer)+1, MPI_CHAR, 0, 9, MPI_COMM_WORLD);
                              free(buffer);
                            }
                            
                            

                        }

                        


                  }
    } else {											//?????????????????????????????????????????????????????????????????
      for (i=0; i<iter; i++){
    aux=cur_main_table;
    cur_main_table = aux_main_table;
    aux_main_table=aux;
  
    free_list(aux_main_table, SIZE_MAIN_TABLE); 
  
    int temp;
    int num_up = 0;
    int num_down = 0;
            for(temp=0; temp < SIZE_MAIN_TABLE; temp++){
      current = cur_main_table[temp];
      if(current.key!=-1){
        neighbours= num_alive_neighbours_a(cur_main_table, aux_main_table, aux_low_table, aux_high_table, current.x, current.y, current.z, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_CUBE, SIZE_SIDE_TABLE, &num_down, &num_up);
        if (neighbours>=2 && neighbours<=4 && current.x >= bound_low && current.x <= bound_high){
          //printf("Insertion point 1: (%d, %d)\n", bound_low, bound_high);
          insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

          if (current.x == bound_low){
            insert(aux_low_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
            num_down = num_down + 1;
          }

          if (current.x == bound_high){
            insert(aux_high_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
            num_up = num_up + 1;
          }
        }

        while (current.next != NULL) {
          current=*current.next;
          neighbours= num_alive_neighbours_a(cur_main_table, aux_main_table, aux_low_table, aux_high_table, current.x, current.y, current.z, ghost_down, ghost_up, SIZE_MAIN_TABLE, SIZE_CUBE, SIZE_SIDE_TABLE, &num_down, &num_up);
          if (neighbours>=2 && neighbours<=4 && current.x >= bound_low && current.x <= bound_high){
            //if (id == 2) printf("Insertion point 2: (%d, %d, %d)\n", current.x, current.y, current.z);
            insert(aux_main_table, current.x, current.y, current.z, SIZE_MAIN_TABLE);

            if (current.x == bound_low){
              insert(aux_low_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
              num_down = num_down+1;
            }

            if (current.x == bound_high){
              insert(aux_high_table, current.x, current.y, current.z, SIZE_SIDE_TABLE);
              num_up = num_up+1;
            }
          }
        } 
      }
    }
              /*
                if (id == 0){
              char buffer22[5000];
              printf("-------------CUR MAIN TABLE ------------\n");
              print_table(cur_main_table, SIZE_MAIN_TABLE, buffer22);
              printf("%s\n", buffer22);
              memset(buffer22, '\0', 5000);
              printf("%s\n", buffer22);              
              printf("-------------AUX MAIN TABLE ------------\n");
              print_table(aux_main_table, SIZE_MAIN_TABLE, buffer22);
              printf("%s\n", buffer22);
              memset(buffer22, '\0', 5000);
              printf("-------------AUX LOW TABLE-------------\n");
              print_table(aux_low_table, SIZE_SIDE_TABLE, buffer22);
              printf("%s\n", buffer22 );
              memset(buffer22, '\0', 5000);
              printf("-------------AUX HIGH TABLE-------------\n");
              print_table(aux_high_table, SIZE_SIDE_TABLE, buffer22);
              printf("%s\n", buffer22 );
              memset(buffer22, '\0', 5000);
              

          }*/
          

                if (i == iter-1){
                     //elapsed_time += MPI_Wtime();
          cell * order[SIZE_CUBE];
          for (int l = 0; l<SIZE_CUBE; l++)
          order[l] = NULL;
                     
          ordered_list(aux_main_table, order, SIZE_MAIN_TABLE, SIZE_CUBE);

                            
          char *buffer = malloc(BUFFER_SIZE);
          print_list(order, SIZE_CUBE,buffer);
                            
          if (id == 0){
              printf("%s", buffer);
              fflush(stdout);
              char *buffer_ord = malloc(BUFFER_SIZE);

              for (int q = 1; q < p; q++){
                    MPI_Status stat;
                    MPI_Recv(buffer_ord, BUFFER_SIZE, MPI_CHAR, q, 9, MPI_COMM_WORLD, &stat);
                    printf("%s", buffer_ord);
                    fflush(stdout);
              }
              free(buffer_ord);

              } else {

                    MPI_Send(buffer, strlen(buffer)+1, MPI_CHAR, 0, 9, MPI_COMM_WORLD);
                    free(buffer);
              }
                            
              break;              

      }

      MPI_Request request_up;
      int buffer_recv_up[BUFFER_MAX];
      int TAG_SDOWN_RUP = 0;

      MPI_Irecv(buffer_recv_up, BUFFER_MAX, MPI_INT, proc_up, TAG_SDOWN_RUP, MPI_COMM_WORLD, &request_up);

      MPI_Request request_down;
      int buffer_recv_down[BUFFER_MAX];
      int TAG_SUP_RDOWN = 1;

      MPI_Irecv(buffer_recv_down, BUFFER_MAX, MPI_INT, proc_down, TAG_SUP_RDOWN, MPI_COMM_WORLD, &request_down);

      int send_up[num_up*3];
      serialize(aux_high_table, SIZE_SIDE_TABLE, send_up, num_up*3);

      MPI_Send(send_up, num_up*3, MPI_INT, proc_up, TAG_SUP_RDOWN, MPI_COMM_WORLD);

      int send_down[num_down*3]; 
      serialize(aux_low_table, SIZE_SIDE_TABLE,send_down , num_down*3);


      MPI_Send(send_down, num_down*3, MPI_INT, proc_down, TAG_SDOWN_RUP, MPI_COMM_WORLD);


      MPI_Status status_up;
      MPI_Status status_down;

      MPI_Wait(&request_up, &status_up);
      MPI_Wait(&request_down, &status_down);

      int num_recv_up;
      int num_recv_down;

      MPI_Get_count(&status_up, MPI_INT, &num_recv_up);
      MPI_Get_count(&status_down, MPI_INT, &num_recv_down);

      insert_table(aux_main_table, SIZE_MAIN_TABLE, buffer_recv_up, num_recv_up);
      insert_table(aux_main_table, SIZE_MAIN_TABLE, buffer_recv_down, num_recv_down);
      free_list(aux_high_table, SIZE_SIDE_TABLE);
      free_list(aux_low_table, SIZE_SIDE_TABLE);
    
      
      
   } 




    }



  MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time += MPI_Wtime();
   

   if (id == 0){
      //printf("%f\n", elapsed_time);
   }
   MPI_Finalize();
}   
     
   
   
   
   
   
   
   
   
   
   
   

