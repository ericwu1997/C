#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

timer_t firstTimerID;
timer_t secondTimerID;
timer_t thirdTimerID;

static void timerHandler(int sig, siginfo_t *si, void *uc)
{
    timer_t *tidp;
    tidp = si->si_value.sival_ptr;

    if (*tidp == firstTimerID)
    {
        printf("Hello 1\n");
    }
    else if (*tidp == secondTimerID)
    {
        printf("Hello 2\n");
    }
    else
    {
        printf("Hello 3\n");
    }
}

static int makeTimer(char *name, timer_t *timerID, int expireMS, int intervalMS)
{
    struct sigevent te;
    struct itimerspec its;
    struct sigaction sa;
    int sigNo = SIGRTMIN;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(sigNo, &sa, NULL) == -1)
    {
        fprintf(stderr, "main: Failed to setup signal handling for %s.\n", name);
        return (-1);
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &te, timerID);

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = intervalMS * 1000000;
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    timer_settime(*timerID, 0, &its, NULL);

    return (0);
}

int main(int argc, char *argv[])
{
    makeTimer("First Timer", &firstTimerID, 1000, 0);
    makeTimer("First Timer", &secondTimerID, 800, 0);
    makeTimer("First Timer", &thirdTimerID, 600, 0);
    while (1)
    {
        /* code */
        sleep(5);
    }
}