#include "syscall.h"

#define N 8
#define NB_ITEMS 20

static int buffer[N];
static int in = 0;
static int out = 0;

int emptyS;
int fullS;
int mutexS;

static void producer(void *arg) {
    int id = (int)arg;

    for (int k = 0; k < NB_ITEMS; k++) {
        int item = id * 1000 + k;

        P(emptyS);
        P(mutexS);

        buffer[in] = item;
        in = (in + 1) % N;

        V(mutexS);
        V(fullS);
    }

    UserThreadExit();
}

static void consumer(void *arg) {
    int id = (int)arg;

    for (int k = 0; k < NB_ITEMS; k++) {
        int item;

        P(fullS);
        P(mutexS);

        item = buffer[out];
        out = (out + 1) % N;

        

        PutString("C");
        PutInt(id);
        PutString(" got ");
        PutInt(item);
        PutString("\n");
        V(mutexS);
        V(emptyS);
    }

    UserThreadExit();
}

int main() {

    emptyS = SemCreate(N);
    fullS  = SemCreate(0);
    mutexS = SemCreate(1);

    if (emptyS < 0 || fullS < 0 || mutexS < 0) {
        PutString("SemCreate failed\n");
        Halt();
        return 0;
    }

    int p1 = UserThreadCreate(producer, (void*)1);
    int p2 = UserThreadCreate(producer, (void*)2);
    int c1 = UserThreadCreate(consumer, (void*)1);
    int c2 = UserThreadCreate(consumer, (void*)2);

    if (p1 < 0 || p2 < 0 || c1 < 0 || c2 < 0) {
        PutString("UserThreadCreate failed\n");
        Halt();
        return 0;
    }

    UserThreadJoin(p1);
    UserThreadJoin(p2);
    UserThreadJoin(c1);
    UserThreadJoin(c2);

    SemDestroy(emptyS);
    SemDestroy(fullS);
    SemDestroy(mutexS);

    PutString("=== Done ===\n");
    Halt();
    return 0;
}
