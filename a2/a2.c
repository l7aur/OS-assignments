#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include "a2_helper.h"
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <fcntl.h>

#define NUMBER_OF_THREADS_IN_P4 42
#define NUMBER_OF_THREADS_IN_P5 4
#define MAX_CONCURRENT_THREADS 5
#define THREAD_ID 15
sem_t semaphore;
sem_t sem;
sem_t semm;
sem_t order_sema, sem2;
sem_t *mes1;
sem_t *mes2;
pthread_mutex_t lock;
int th_no;

void P(sem_t *sem)
{
    sem_wait(sem);
}
void V(sem_t *sem)
{
    sem_post(sem);
}

void *th_function_6_2(void *arg)
{
    info(BEGIN, 6, 2);
    info(END, 6, 2);
    return NULL;
}

void *th_function_5(void *arg)
{
    if(*(int *) arg == 1)
        P(mes2);
    info(BEGIN, 5, *(int *)arg);
    info(END, 5, *(int *)arg);
    if (*(int *)arg == 3)
        V(mes1);
    return NULL;
}

void *th_function_6_34(void *arg)
{
    if (*(int *)arg == 4)
       P(mes1);
    info(BEGIN, 6, *(int *)arg);
    info(END, 6, *(int *)arg);
    if(*(int *) arg == 4)
        V(mes2);
    return NULL;
}

void *th_function_6_1(void *argument)
{
    info(BEGIN, 6, 1);
    pthread_t t2;
    pthread_create(&t2, NULL, th_function_6_2, NULL);
    pthread_join(t2, NULL);
    info(END, 6, 1);
    return NULL;
}

void *thread_function_4(void *arg)
{
    int c;
    if (*(int *)arg != THREAD_ID)
    {
        P(&order_sema); // try to enter but can enter only if 15 has entered
        V(&order_sema);
        P(&semaphore);
        info(BEGIN, 4, *(int *)arg);
    }
    else
    {
        P(&semaphore);
        info(BEGIN, 4, *(int *)arg);
        for (int k = 0; k < MAX_CONCURRENT_THREADS - 1; k++) // 15 entered so generate the permissions
            V(&order_sema);
    }

    // critical region
    pthread_mutex_lock(&lock);
    th_no++;
    c = th_no;
    pthread_mutex_unlock(&lock);
    // end of critical region

    if (c == MAX_CONCURRENT_THREADS)
        V(&semm);
    else if (c < MAX_CONCURRENT_THREADS)
    {
        P(&semm); // can t enter until condition 3 fulfiled
        V(&semm);
    }
    if (*(int *)arg != THREAD_ID) // force first thread to finish to be 15
        P(&sem2);
    info(END, 4, *(int *)arg);
    free(arg);
    V(&sem2);
    V(&semaphore);
    return NULL;
}

int main(int argc, char **argv)
{
    init();
    int pid2, pid3, pid4, pid5, pid6, pid7, pid8;
    if ((mes1 = sem_open("eman1", O_CREAT, 0644, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        printf("CANNOT CREATE SEMAPHORE\n");
        exit(-1);
    }
    if ((mes2 = sem_open("eman2", O_CREAT, 0644, 0)) == SEM_FAILED)
    {
        perror("sem_open");
        printf("CANNOT CREATE SEMAPHORE\n");
        exit(-1);
    }

    info(BEGIN, 1, 0);

    // mes2 = sem_open("name2", O_CREAT, 0660, 0);
    if (sem_init(&semaphore, 0, MAX_CONCURRENT_THREADS) < 0 || sem_init(&semm, 0, 0) < 0 || sem_init(&sem, 0, 1) < 0 || sem_init(&order_sema, 0, 0) < 0 || sem_init(&sem2, 0, 0) < 0)
    {
        perror("Error creating the semaphore");
        exit(2);
    }
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("Cannot initialize the lock");
        exit(2);
    }

    pid3 = fork();
    switch (pid3)
    {
    case -1:
        printf("ERROR\n");
        break;
    case 0:
        info(BEGIN, 3, 0);
        info(END, 3, 0);
        break;
    default: // P1
        waitpid(pid3, NULL, 0);
        pid4 = fork();
        switch (pid4)
        {
        case -1:
            printf("ERROR\n");
            break;
        case 0: // P4
            info(BEGIN, 4, 0);
            // /////////////////////////////////2.4begin

            pthread_t t[NUMBER_OF_THREADS_IN_P4];
            for (int i = 0; i < NUMBER_OF_THREADS_IN_P4; i++)
            {
                int *arg = (int *)malloc(sizeof(int));
                *arg = i + 1;
                pthread_create(&t[i], NULL, thread_function_4, arg);
            }

            for (int i = 0; i < NUMBER_OF_THREADS_IN_P4; i++)
                pthread_join(t[i], NULL);

            // /////////////////////////////////2.4end
            info(END, 4, 0);
            break;
        default: // P1
            waitpid(pid4, NULL, 0);
            pid5 = fork();
            switch (pid5)
            {
            case -1:
                printf("ERROR\n");
                break;
            case 0: // P5
                info(BEGIN, 5, 0);
                /// start
                pthread_t t[NUMBER_OF_THREADS_IN_P5];
                for (int i = 0; i < NUMBER_OF_THREADS_IN_P5; i++)
                {
                    int *arg = (int *)malloc(sizeof(int));
                    *arg = i + 1;
                    pthread_create(&t[i], NULL, th_function_5, arg);
                }

                for (int i = 0; i < NUMBER_OF_THREADS_IN_P5; i++)
                    pthread_join(t[i], NULL);
                /// end
                info(END, 5, 0);
                break;
            default: // P1
                pid2 = fork();
                switch (pid2)
                {
                case -1:
                    printf("error\n");
                    break;
                case 0:
                    info(BEGIN, 2, 0);
                    pid6 = fork();
                    switch (pid6)
                    {
                    case -1:
                        printf("ERROR\n");
                        break;
                    case 0: // P6
                        info(BEGIN, 6, 0);

                        // //////////////////////////////////////2.3begin
                        pthread_t t1, t3, t4;
                        pthread_create(&t1, NULL, th_function_6_1, NULL);
                        pthread_join(t1, NULL);

                        int arg3 = 3;
                        int arg4 = 4;
                        pthread_create(&t3, NULL, th_function_6_34, &arg3);
                        pthread_join(t3, NULL);
                        pthread_create(&t4, NULL, th_function_6_34, &arg4);
                        pthread_join(t4, NULL);

                        // //////////////////////////////////////2.3end
                        info(END, 6, 0);
                        break;
                    default: // P2
                        waitpid(pid6, NULL, 0);
                        pid7 = fork();
                        switch (pid7)
                        {
                        case -1:
                            printf("ERROR\n");
                            break;
                        case 0: // P7
                            info(BEGIN, 7, 0);
                            info(END, 7, 0);
                            break;
                        default: // P2
                            waitpid(pid7, NULL, 0);
                            pid8 = fork();
                            switch (pid8)
                            {
                            case -1:
                                printf("ERROR\n");
                                break;
                            case 0: // P8
                                info(BEGIN, 8, 0);
                                info(END, 8, 0);
                                break;
                            default: // P2
                                waitpid(pid8, NULL, 0);
                                info(END, 2, 0);
                                break;
                            }
                            break;
                        }
                        break;
                    }
                    break;
                default:
                    waitpid(pid2, NULL, 0);
                    waitpid(pid5, NULL, 0);
                    break;
                }
                break;
            }
            break;
        }

        break;
    }
    info(END, 1, 0);
    sem_destroy(&sem);
    sem_destroy(&order_sema);
    sem_destroy(&sem2);
    sem_destroy(&semm);
    sem_destroy(&semaphore);
    sem_unlink("eman1");
    sem_unlink("eman2");
    sem_close(mes1);
    if (pthread_mutex_destroy(&lock) != 0)
    {
        perror("Cannot destroy the lock");
        exit(8);
    }
    return 0;
}