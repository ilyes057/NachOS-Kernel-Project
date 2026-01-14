#include "syscall.h"
#include "mem.h"

static void print_line(const char *s) {
    PutString((char *)s);
    PutChar('\n');
}

static void print_result(const char *name, int ok) {
    PutString((char *)name);
    PutString((char *)": ");
    PutString((char *)(ok ? "OK" : "FAIL"));
    PutChar('\n');
}

static void print_ulong(unsigned long value) {
    char buf[32];
    int i = 0;

    if (value == 0) {
        PutChar('0');
        return;
    }

    while (value > 0 && i < (int)(sizeof(buf) - 1)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0) {
        PutChar(buf[--i]);
    }
}

void afficher_zone(void *adresse, unsigned int taille, int free) {
    PutString((char *)"Zone ");
    PutString((char *)(free ? "libre" : "occupee"));
    PutString((char *)", Adresse : ");
    print_ulong((unsigned long)adresse);
    PutString((char *)", Taille : ");
    print_ulong((unsigned long)taille);
    PutChar('\n');
}

int main(void) {
    void *base = mem_init(1024);
    if (!base) {
        print_line("mem_init failed");
        return 1;
    }

    print_line("=== Best-fit demonstration ===");
    mem_set_fit_handler(mem_first_fit);

    void *a = mem_alloc(200);
    void *b = mem_alloc(120);
    void *c = mem_alloc(160);
    void *d = mem_alloc(100);
    print_result("alloc a", a != NULL);
    print_result("alloc b", b != NULL);
    print_result("alloc c", c != NULL);
    print_result("alloc d", d != NULL);
    mem_show(afficher_zone);

    print_line("free middle block (b)");
    mem_free(b);
    mem_free(c);
    mem_show(afficher_zone);

    mem_set_fit_handler(mem_best_fit);
    print_line("alloc size 100 with best_fit (should reuse middle block)");
    void *e = mem_alloc(160);
    print_result("best_fit alloc e", e != NULL);
    mem_show(afficher_zone);

    mem_free(a);
    mem_free(c);
    mem_free(d);
    mem_free(e);
    print_line("cleanup done");

    print_line("memTest finished");
    return 0;
}
