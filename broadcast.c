// TRABALHO2: CI316 1o semestre 2023 (myBroadcast_rb.c)
// Aluno:
// GRR:
//

// Esqueleto de programa principal disponibilizado pelo prof!

    //////////////////////////////////////////////
    ///// ATENCAO: NAO MUDAR O MAIN, a       /////
    /////  menos que seja MESMO necessário!  /////
        //////////////////////////////////////////////
        
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chrono.c"
#include <assert.h>

#define USE_MPI_Bcast 1  // do NOT change
#define USE_my_Bcast 2   // do NOT change
//choose either BCAST_TYPE in the defines bellow
//#define BCAST_TYPE USE_MPI_Bcast
#define BCAST_TYPE USE_my_Bcast

long nmsg;       // o número total de mensagens
long tmsg;       // o tamanho de cada mensagem
int nproc;      // o número de processos MPI
int raiz;       // maquina que ira enviar as mensagens
int processId; 	// rank dos processos
int ni;			// tamanho do vetor contendo as mensagens

chronometer_t myBroadcastChrono;

//#define DEBUG 1
#define DEBUG 0

const int SEED = 100;


// MACROS para AJUDAR!
#define LOGIC_RANK( my_rank, root, comm_size ) \
        (( my_rank + comm_size - root ) % comm_size)
#define PHYSIC_RANK( logic_rank, root, comm_size ) \
        (( logic_rank + root ) % comm_size)        

// Calcula qual o rank logico do participante que irá enviar para o rank logico recv
int calculaEnviador(int recv){
    int menorPot2 = 1;
    int n = recv;
    while (n) {
        menorPot2 *= 2;
        n /= 2;
    }
    menorPot2/=2;
    return (~menorPot2) & recv;
}

// Calcula em qual iteracao o rank logico recebe sua mensagem
int calculaIteracao(int rank) {
    int n = rank;
    int count = 0;
    while (n) {
        n /= 2;
        count++;
    }
    return count;
}

// int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
void my_Bcast_rb(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

    int rankFis;
    int comm_size;
    MPI_Comm_rank(comm, &rankFis);
    MPI_Comm_size(comm, &comm_size);
    int rankLog = LOGIC_RANK(rankFis, root, comm_size);

    long *bufflong = (long *)buffer;
    long *halfbuff = &bufflong[count/2];
    //printf("%d: bufflong = %p, halfbuff = %p, count = %d\n", rankLog, bufflong, halfbuff, count);

    MPI_Status status;
    // Receber de um processo e enviar para os outros
    if (rankLog != 0){
        int rankLogEnviador = calculaEnviador(rankLog);
        int rankFisEnviador = PHYSIC_RANK(rankLogEnviador, root, comm_size);

        // fprintf(stderr,"%d(%d): espero receber do %d(%d)\n",
        //         rankLog, rankFis, rankLogEnviador, rankFisEnviador);
        // Participantes impares receberao a segunda metade
        if (rankLog % 2 == 1){
            MPI_Recv((void*)halfbuff, count/2 + count%2, datatype, rankFisEnviador, 0, comm, &status);
        }
        // Participantes pares receberam a primeira metade
        else {
            MPI_Recv((void*)bufflong, count/2, datatype, rankFisEnviador, 0, comm, &status);
        }
        // fprintf(stderr,"%d(%d): recebi do %d(%d)\n",
        //         rankLog, rankFis, rankLogEnviador, rankFisEnviador);
    }

    // Enviar para os processoss necessários
    for(int fase = calculaIteracao(rankLog); 1<<fase < comm_size; fase++){
        // Indice logico para qual enviar na proxima iteração
        int proxSend = rankLog + (1<<fase);
                                //2 ^ fase
        // fprintf(stderr,"%d(%d): envio para o %d(%d)\n",
        //         rankLog, rankFis, proxSend, PHYSIC_RANK(proxSend, root, comm_size));
        if (proxSend >= comm_size) {
          break;
        }

        if(proxSend % 2) {
          MPI_Send((void*)halfbuff, count/2 + count%2, datatype, PHYSIC_RANK(proxSend, root, comm_size), 0, comm);
        }
        else {
          MPI_Send(buffer, count/2, datatype, PHYSIC_RANK(proxSend, root, comm_size), 0, comm);
        }
        // fprintf(stderr,"%d(%d): enviei para o %d(%d)\n",
        //         rankLog, rankFis, proxSend, PHYSIC_RANK(proxSend, root, comm_size));
    }

    if (halfbuff == bufflong) return;
    // Fazer troca com o vizinho proximo
    // Pares enviam primeiro e depois impares enviam
    if(rankLog % 2) {
      int dest = (rankLog + 1) % comm_size;
      // recebe
      MPI_Recv((void*)halfbuff, count/2 + count%2, datatype, dest, 0, comm, &status);
      // envia
      MPI_Send(buffer, count/2, datatype, PHYSIC_RANK(dest, root, comm_size), 0, comm);
    }
    else {
      if (rankLog == 0 && comm_size % 2 == 1) return;
      int dest = rankLog - 1;
      if (rankLog == 0) dest = comm_size - 1;
      // envia
      MPI_Send(buffer, count/2, datatype, PHYSIC_RANK(dest, root, comm_size), 0, comm);
      // recebe
      MPI_Recv((void*)halfbuff, count/2 + count%2, datatype, dest, 0, comm, &status);
    }
}

// OBS1: sua função my_Bcast_rb
// deve ter o mesmo protótipo da MPI_Bcast
// ou seja, os dois protótipos são:
//  int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm );
//  void my_Bcast_rb( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm );
// PORÉM:
// a sua função só precisa funcionar se chamada com datatype MPI_LONG

//OBS2:
//O Prof. vai disponibilizar a função
//verifica_my_Bcast_rb( ... )
//Essa função deve ser chamada AO FINAL do seu programa,
//DEPOIS que ele apresentar suas medições,
//E antes de desalocar seu buffer, e também ANTES de finalizar o
//MPI. Essa função vai verificar se sua função my_Bcast está
//funcionando adequadamente
//O tempo gasto nessa função de verificação 
//  NAO deve influenciar suas medidas. 

void verifica_my_Bcast( void *buffer, int count, MPI_Datatype datatype,
                           int root, MPI_Comm comm ){
    int comm_size;
    int my_rank;
    
    MPI_Comm_size( comm, &comm_size );
    MPI_Comm_rank( comm, &my_rank );
    static long *buff = NULL;
    if (!buff)
        buff = (long *) calloc( count*comm_size, sizeof(long) );
    
    
    // preenche a faixa do raiz com alguma coisa (apenas no raiz)
    if( my_rank == root )
       for( int i=0; i<count; i++ )
          buff[ i ] = i+SEED;
    
    #if BCAST_TYPE == USE_MPI_Bcast
       MPI_Bcast( buff, count, datatype, root, comm );
    #elif BCAST_TYPE == USE_my_Bcast
       my_Bcast_rb( buff, count, datatype, root, comm );
    #else
       assert( BCAST_TYPE == USE_MPI_Bcast || BCAST_TYPE == USE_my_Bcast );
    #endif   
	   
    
    // cada nodo verifica se sua faixa recebeu adequadamente o conteudo
    int ok=1;
    int i;
    for( i=0; i<count; i++ )
       if( buff[ i ] != i+SEED ) {
          ok = 0;
          break;
       }
    // imprime na tela se OK ou nao
    if( ok )
        fprintf( stderr, "MY BCAST VERIF: node %d received ok\n", my_rank );
    else
        fprintf( stderr, "MY BCAST VERIF: node %d NOT ok! local position: %d contains %ld\n",
                           my_rank, i, buff[i] );

   free(buff);      
}

int main(int argc, char *argv[]){

    //wz_debug = 1;    // DEBUG_MPI_MESSAGES


    raiz = 0;

    if (argc < 4){
        printf("usage: mpirun -np <np> %s <nmsg> <tmsg> <nproc> (-r <r>)\n",
            argv[0]);
        return 0;
    }
    else{
        nmsg = atoi(argv[1]);
        tmsg = atoi(argv[2]);
        if (tmsg % 8 != 0){
            printf("usage: mpirun -np <np> %s <nmsg> <tmsg> <nproc> (-r <r>)\n",
            argv[0]);
            printf("<tmsg> deve ser multiplo de 8\n");
            return 0;
        }
        nproc = atoi(argv[3]);
        if(argc == 6){
            if (strcmp(argv[4], "-r") == 0)
                raiz = atoi(argv[5]);
        }
    }

    ni = tmsg/sizeof(long int);   // quantidade de inteiros longos nas mensagens  
    MPI_Status Stat;

        // aloca a mensagem
    long int *inmsg = (long int*)calloc(ni, sizeof(long int));

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);
    
    if(processId == 0){
    #if BCAST_TYPE == USE_MPI_Bcast
        fprintf( stderr, "\n****** USING MPI_Bcast ******\n" );
    #elif BCAST_TYPE == USE_my_Bcast
        fprintf( stderr, "\n****** USING my_Bcast_rb ******\n" );
    #else
        assert( BCAST_TYPE == USE_MPI_Bcast || BCAST_TYPE == USE_my_Bcast );
    #endif    
    }
    
    // check my macros
    if(processId == 0){
    fprintf( stderr, "\n---------ranks-------\n" );
    for( int phys=0; phys<nproc; phys++ )
            fprintf( stderr, "%d ", phys );
    fprintf( stderr, " PHYSIC\n" );
    for( int phys=0; phys<nproc; phys++ )
            fprintf( stderr, "%d ", LOGIC_RANK( phys, raiz, nproc ) );
    fprintf( stderr, " LOGIC from PHYSIC\n" );
    for( int phys=0; phys<nproc; phys++ ) {
            int logic_no = LOGIC_RANK( phys, raiz, nproc );
            fprintf( stderr, "%d ", PHYSIC_RANK( logic_no, raiz, nproc ) );
    }
    fprintf( stderr, " PHYSIC from LOGIC\n" );
    fprintf( stderr, "\n---------------------\n" );
    }
    
    // preenche a mensagem
        if(processId == raiz){
        for(long int i = 1; i <= ni; i++)
            inmsg[i-1] = i+SEED;
    }

        if( processId == 0 )
        printf("----- root= %d, nmessages=%ld, nlong=%d"
                " argc:%d argv[4]=%s argv[5]=%s comm_size=%d\n",
                    raiz, nmsg, ni, argc, argv[4], argv[5], nproc );
        
    MPI_Barrier(MPI_COMM_WORLD);

    if(processId == 0){
        chrono_reset(&myBroadcastChrono);
        chrono_start(&myBroadcastChrono);
    }

    for(int m = 0; m < nmsg; m++)
    #if BCAST_TYPE == USE_MPI_Bcast
        MPI_Bcast_rb( inmsg, ni, MPI_LONG, raiz, MPI_COMM_WORLD );
    #elif BCAST_TYPE == USE_my_Bcast
        my_Bcast_rb( inmsg, ni, MPI_LONG, raiz, MPI_COMM_WORLD );
    #else
        assert( BCAST_TYPE == USE_MPI_Bcast || BCAST_TYPE == USE_my_Bcast );
    #endif    
        
        

    MPI_Barrier(MPI_COMM_WORLD);

    if(processId == 0){
        chrono_stop(&myBroadcastChrono);
        chrono_reportTime(&myBroadcastChrono, "myBroadcastChrono");

        // calcular e imprimir a VAZAO (nesse caso: numero de BYTES/s)
        double total_time_in_seconds = (double)chrono_gettotal(&myBroadcastChrono) /
                                    ((double)1000 * 1000 * 1000);
        double total_time_in_micro = (double)chrono_gettotal(&myBroadcastChrono) /
                                    ((double)1000);
        printf("total_time_in_seconds: %lf s\n", total_time_in_seconds);
        printf("Latencia: %lf us (CADA broadcast)\n", (total_time_in_micro / nmsg));
        double MBPS = (((double)nmsg*tmsg) / ((double)total_time_in_seconds*1000*1000));
        printf("Throughput: %lf MB/s\n", MBPS*(nproc-1));
    }
    
    #if DEBUG == 1
        int rank;
        MPI_Comm_rank( MPI_COMM_WORLD, &rank ); 

        printf("rank %d: ", rank);
        for(long int i = 0; i < 2; i++)
            printf("%ld ", inmsg[i]);
        for(long int i = ni-2; i < ni; i++)
            printf("%ld ", inmsg[i]);
        printf("\n");
    #endif
    
    // verifica_my_Bcast a partir de raiz 0  ---------- COM valores da linha de comando
    verifica_my_Bcast( inmsg, ni, MPI_LONG, raiz, MPI_COMM_WORLD );

        // verifica_my_Bcast a partir de raiz 0 ---------- COM OUTROS valores
    //verifica_my_Bcast( inmsg, 7, MPI_LONG, 0, MPI_COMM_WORLD );
        // verifica_my_Bcast a partir de raiz 1 
    //verifica_my_Bcast( inmsg, 7, MPI_LONG, 1, MPI_COMM_WORLD );
        // verifica_my_Bcast a partir de raiz 3 
    //verifica_my_Bcast( inmsg, 7, MPI_LONG, 3, MPI_COMM_WORLD );
    
    free(inmsg);

    MPI_Finalize( );
    return 0;
}
