/*
 copie os .lib e .a para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\lib\
 copie os .dll para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\bin\
 copie os .h para: c:\Arquivos de Programas (x86)\Codeblocks\mingw\include\
 project -> build options -> linker settings -> Other linker options: inclua -lpthreadGC1
*/
//#define MWIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#ifdef MWIN
#include <windows.h>
#endif


#define ERRO 0
#define OK   1

#define BSIZE 6
#define NITEM 1000

#define TEMPO_SLEEP 10


// Variaveis globais

// declaracao dos mutexes
pthread_mutex_t G_p_fi;  // independente
pthread_mutex_t G_p_be;  // segundo - nao usado com G_p_bs
pthread_mutex_t G_p_bs;  // segundo - nao usado com G_p_be

// controladores dos buffers
int G_qtd_be;
int G_qtd_bs;

// variavel de controle do encerramento
int G_terminou;

// buffers de processamento
char G_be[BSIZE];
char G_bs[BSIZE];

void m_usleep(unsigned long pause)
{
#ifdef MWIN
   Sleep(pause);
#else
   usleep(pause*1000);
#endif
   return;
}

void init_lock(pthread_mutex_t *lock) // inicializa as variaveis de lock, fazer isto antes do inicio das threads
{
   pthread_mutex_init(lock,NULL);
   return;
}

void fini_lock(pthread_mutex_t *lock) // finalize as variaveis de lock, apos o pthread_kill
{
   pthread_mutex_destroy(lock);
   return;
}

int gerar_entrada()
{
    FILE *arq;
    int i;
    if ((arq = fopen("e.txt","wt"))==NULL)
    {
        printf("\n>> ERRO: criando o arquivo de entrada (e.txt)\n");
        return(ERRO);
    }

    for (i = 1 ; i <= NITEM; ++i)
    {
        fprintf(arq,"%05d\n",i);
    }

    fflush(arq);
    fclose(arq);

    return(OK);
}

void *escrita()
{
    int acabou = 0;
    FILE *arq;
    printf("\n>> Iniciou Escrita");
    arq = fopen("s.txt","wt");
    while (1)
    {
        if (!acabou)
        {
	    pthread_mutex_lock(&G_p_bs);
            if (G_qtd_bs > 0)
            {
                fprintf(arq, "%s\n", G_bs);
                memset(G_bs,0,sizeof(G_bs)*BSIZE);   
                G_qtd_bs = 0;
            }
            pthread_mutex_unlock(&G_p_bs);
            pthread_mutex_lock(&G_p_fi);
            if (G_terminou == 1)
            {
                G_terminou = 0;
                fflush(arq);
                fclose(arq);
                acabou = 1;    
                printf("\n>> Terminou Escrita");
            }
            pthread_mutex_unlock(&G_p_fi);
        }
   }
   return(NULL);
}


void *leitura()
{
   int i      = 0;
   int acabou = 0;
   char c, buf[BSIZE];
   FILE *arq;
   printf("\n>> Iniciou Leitura\n");
   arq = fopen("e.txt","rt");

   while (1)
   {
      if (!acabou)
      {
         pthread_mutex_lock(&G_p_be);
         if (G_qtd_be <= 0)
         {
            if (fgets(G_be, BSIZE, arq) != NULL)
               G_qtd_be = 6;
            else
               acabou = 1;
         }
         pthread_mutex_unlock(&G_p_be);
       }
       else 
       {
          pthread_mutex_lock(&G_p_fi);	
	  if (G_terminou == 3)
          { 
              G_terminou = 2;
	      fclose(arq);
              printf("\n>> Terminou Leitura\n");
          }
          pthread_mutex_unlock(&G_p_fi);
       }	
       m_usleep(TEMPO_SLEEP); 
   }
   return(NULL);
}

void *processamento()
{
    int acabou = 0;

    char buf[BSIZE];
    memset(buf, 0, sizeof(buf)*BSIZE);   
    printf("\n>> Iniciou Processamento");
    int i = 0;
    while (1)
    {
        if (!acabou)
        {
           if (i <= 0) 
           {
              pthread_mutex_lock(&G_p_be);
              i = 0;  
              if (G_qtd_be != 0)
              {
                 for (int j=G_qtd_be-2; j>=0; j--, i++)
                     buf[i] = G_be[j];
                 buf[5] = '\0';
                 memset(G_be, 0, sizeof(G_be)*BSIZE);   
                 G_qtd_be = 0;
              }
              pthread_mutex_unlock(&G_p_be);
            }
 	    else 
            {    
                pthread_mutex_lock(&G_p_bs);
                if (G_qtd_bs == 0)
                {
                    
		    sprintf(G_bs,"%s",buf);	
		    G_qtd_bs = sizeof G_bs;	
                    memset(buf, 0, sizeof(buf)*BSIZE);   
                }
                i = 0;
                pthread_mutex_unlock(&G_p_bs);
            }
            pthread_mutex_lock(&G_p_fi);
            if (G_terminou == 2)
            {
               G_terminou = 1;
               acabou = 1;
               printf("\n>> Terminou Processamento");
            }
            pthread_mutex_unlock(&G_p_fi);
	}
        m_usleep(TEMPO_SLEEP); 
    }
    return(NULL);
}

void finalizar()
{
    int nao_acabou = 1;
    while (nao_acabou)
    {
        m_usleep(50);
        pthread_mutex_lock(&G_p_fi);
        if (G_terminou == 0)
        {
            printf("\n>> Finalizou Tudo \n\n");
            nao_acabou = 0;
        }
        pthread_mutex_unlock(&G_p_fi);
    }
    return;
}

int main(void)
{
    // declaracao das pthreads
    pthread_t T_entrada, T_processa, T_saida;	

    // inicializacao do G_terminou
    G_terminou = 3;

    // inicializacao dos mutexes de lock
    init_lock(&G_p_fi);  
    init_lock(&G_p_be);  
    init_lock(&G_p_bs);  

    // limpeza dos buffers
    memset(G_be,0,sizeof(G_be)*BSIZE);   
    memset(G_bs,0,sizeof(G_bs)*BSIZE);   

    // inicializacao dos controladores dos buffers
    G_qtd_bs = 0;
    G_qtd_be = 0;	

    // geracao do arquivo de entrada
    if (!gerar_entrada())
    {
        printf("\nFalha na Geracao do Arquivo de Entrada");
        return(1);
    }
    else
        printf("\n>> Arquivo de Entrada Criado com Sucesso ");


    // chamada das pthreads
    // cria thread de leitura 
    if (pthread_create(&T_entrada,NULL,leitura,NULL))
    {
       printf("\nERRO: criando thread Leitura.\n");
       return(0);
    }
    // cria thread de processamento
    if (pthread_create(&T_processa,NULL,processamento,NULL))
    {
       printf("\nERRO: criando thread Processa.\n");
       return(0);
    }
    // cria thread de saida 
    if (pthread_create(&T_saida,NULL,escrita,NULL))
    {
       printf("\nERRO: criando thread Saida.\n");
       return(0);
    }

    // Aguarda finalizar
//    G_terminou = 0;
    finalizar();

    // matar as pthreads
    pthread_kill(T_entrada,0);
    pthread_kill(T_processa,0); 
    pthread_kill(T_saida,0);

    // finaliza mutexes
    fini_lock(&G_p_fi);  
    fini_lock(&G_p_be);  
    fini_lock(&G_p_bs);  

    return(0);
}
