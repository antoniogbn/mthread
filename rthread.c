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
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#ifdef MWIN
#include <windows.h>
#endif
#define ERRO 0
#define OK   1

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
char G_be[6];
char G_bs[6];

void m_usleep(unsigned long pause)
{
    #ifdef MWIN
       Sleep(pause);
    #else
       usleep(pause*1000l);
    #endif
    return;
}

void init_lock(pthread_mutex_t *lock) // inicializa as variáveis de lock, fazer isto antes do inicio das threads
{
   pthread_mutex_init(lock,NULL);
   return;
}

void fini_lock(pthread_mutex_t *lock) // finalize as variaveis de lock, apos o pthread_kill
{
   pthread_mutex_destroy(lock);
   return;
}

#include <stdio.h>
#include <stdlib.h>

int gerar_entrada()
{
    FILE *arq;
    int i;
    if ((arq = fopen("e.txt","wt"))==NULL)
    {
        printf("\nERRO: criando o arquivo de entrada (e.txt)\n");
        return(ERRO);
    }

    for (i = 1 ; i <= 1000; ++i)
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

    printf("Inicio escrita\n");
    arq = fopen("s.txt","wt");
    for (;;)
    {
        while (!acabou)
        {
            pthread_mutex_lock(&G_p_bs);
            if (G_qtd_bs > 0)
            {
                fprintf(arq, "%s\n", G_bs);
                memset((void *) G_bs, 0, sizeof(char)*6);
                G_qtd_bs = 0;
            }
            pthread_mutex_unlock(&G_p_bs);
            pthread_mutex_lock(&G_p_fi);
            if (G_terminou == 1)
            {
                G_terminou = 0;
                acabou = 1;    
                fflush(arq);
                fclose(arq);
                printf("\nAcabou escrita!");
            }
            pthread_mutex_unlock(&G_p_fi);
            m_usleep(TEMPO_SLEEP);
       }
    }    
    return(NULL);
}

void *leitura()
{
    int acabou = 0;
    FILE *arq;
    printf("Inicio leitura\n");
    arq = fopen("e.txt","rt");
    for (;;)
    {
        while (!acabou)
        {
            pthread_mutex_lock(&G_p_be);
            if(G_qtd_be == 0)
            {
                if((fgets(G_be, sizeof G_be, arq) != NULL))
                {
                    G_be[5] = '\0';
                    G_qtd_be = 6;
                }              
                else
                {
                    G_qtd_be = 0;
                    acabou = 1;
                }
            }
            pthread_mutex_unlock(&G_p_be);
            if (acabou)
            {
                pthread_mutex_lock(&G_p_fi);
                if (G_terminou == 3)
                {
                    printf("\nAcabou leitura!");
                    G_terminou = 2;
                    fclose(arq);
                }
                pthread_mutex_unlock(&G_p_fi);
            }
            m_usleep(TEMPO_SLEEP);
        }    
    }         
    return(NULL);
}

void *processamento()
{
    int acabou = 0;
    int i = 0;
    char aux[6];
    memset((void *) aux,0,sizeof(char)*6);
    printf("Inicio processamento\n");
    for (;;)
    {
        while (!acabou)
        {
            if(i == 0)
            {
                pthread_mutex_lock(&G_p_be);
                if (G_qtd_be > 0 && G_be[0] == '0')
                {
                    for (int x = G_qtd_be - 2; x >= 0; x--)
                    {
                        aux[i] = G_be[x];
                        i++;
                    }
                    aux[5] = '\0';
                }
                else
                {
                  i = 0;  
                }
                memset((void *) G_be,0,sizeof(char)*6);
                G_qtd_be = 0;
                pthread_mutex_unlock(&G_p_be);
            }
            if (i > 0)
            {    
                pthread_mutex_lock(&G_p_bs);
                if (G_qtd_bs == 0)
                {
                    for (G_qtd_bs = 0; G_qtd_bs < i; G_qtd_bs++)
                    {
                        G_bs[G_qtd_bs] = aux[G_qtd_bs];
                    }
                    memset((void *) aux,0,sizeof(char)*6);
                }
                pthread_mutex_unlock(&G_p_bs);
                i = 0;
            }
            pthread_mutex_lock(&G_p_fi);
            if (G_terminou == 2)
            {
                acabou = 1;
                G_terminou = 1;
                printf("\nAcabou Processamento!");
            }
            pthread_mutex_unlock(&G_p_fi);
            m_usleep(TEMPO_SLEEP);
        }
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
            printf("\nEm finalizar... Acabou mesmo!\n\n");
            nao_acabou = 0;
        }
        pthread_mutex_unlock(&G_p_fi);
    }
    return;
}

int main(void)
{
    // declaração das pthreads
    pthread_t tLeitura, tProc, tSaida;

    // inicializacao do G_terminou
    G_terminou = 3;

    // inicializacao dos mutexes de lock
    init_lock(&G_p_be);
    init_lock(&G_p_bs);
    init_lock(&G_p_fi);

    // limpeza dos buffers
    memset((void *) G_be, 0,sizeof(char)*6);
    memset((void *) G_bs, 0,sizeof(char)*6);
     
    // inicializacao dos controladores dos buffers
    G_qtd_bs = 0;
    G_qtd_be = 0;

    // geracao do arquivo de entrada
    if (!gerar_entrada())
    {
        printf("\nVou sair");
        return(1);
    }

    // chamada das pthreads
    if(pthread_create(&tLeitura, NULL, leitura, NULL))
    {
        printf("\nErro: criando thread de leitura. \n");
        return ERRO;
    }

    if(pthread_create(&tProc, NULL, processamento, NULL))
    {
        printf("\nErro: criando thread de processamento. \n");
        return ERRO;
    }

    if(pthread_create(&tSaida, NULL, escrita, NULL))
    {
        printf("\nErro: criando thread de escrita. \n");
        return ERRO;
    }

    // Aguarda finalizar
    finalizar();

    // matar as pthreads
    pthread_kill(tLeitura, 0);
    pthread_kill(tProc, 0);
    pthread_kill(tSaida, 0);

    // finalização dos mutexes
    fini_lock(&G_p_be);
    fini_lock(&G_p_bs);
    fini_lock(&G_p_fi);

    return(0);
}
