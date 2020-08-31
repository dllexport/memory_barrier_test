#include <semaphore.h>
#include <stdio.h>
#include <thread>

#define TEST_SL_FENCE 0 // reordering still happens
#define TEST_M_FENCE 1  // no reordering

sem_t beginSema1;
sem_t beginSema2;
sem_t endSema;

int X, Y;
int r1, r2;

void thread1Func()
{
    while (1)
    {
        sem_wait(&beginSema1);

        X = 1;
#if TEST_SL_FENCE
        asm volatile("sfence" ::
                         : "memory");
        asm volatile("lfence" ::
                         : "memory");
#elif (TEST_M_FENCE)
        asm volatile("mfence" ::
                         : "memory"); // Prevent compiler reordering
#else
        asm volatile("" ::
                         : "memory");
#endif
        r1 = Y;

        sem_post(&endSema);
    }
};

void thread2Func()
{
    while (1)
    {
        sem_wait(&beginSema2);

        Y = 1;
#if TEST_SL_FENCE
        asm volatile("sfence" ::
                         : "memory");
        asm volatile("lfence" ::
                         : "memory");
#elif (TEST_M_FENCE)
        asm volatile("mfence" ::
                         : "memory"); // Prevent compiler reordering
#else
        asm volatile("" ::
                         : "memory");
#endif
        r2 = X;

        sem_post(&endSema);
    }
};

int main()
{
    sem_init(&beginSema1, 0, 0);
    sem_init(&beginSema2, 0, 0);
    sem_init(&endSema, 0, 0);

    std::thread([]() {
        thread1Func();
    }).detach();

    std::thread([]() {
        thread2Func();
    }).detach();

    // Repeat the experiment ad infinitum
    int detected = 0;
    for (int iterations = 1;; iterations++)
    {
        // Reset X and Y
        X = 0;
        Y = 0;
        // Signal both threads
        sem_post(&beginSema1);
        sem_post(&beginSema2);
        // Wait for both threads
        sem_wait(&endSema);
        sem_wait(&endSema);
        // Check if there was a simultaneous reorder
        if (r1 == 0 && r2 == 0)
        {
            detected++;
            printf("%d reorders detected after %d iterations\n", detected, iterations);
        }
    }
    return 0; // Never returns
}