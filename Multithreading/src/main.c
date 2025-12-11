#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"

#include <stdio.h>

#define BUF_SIZE 64

volatile int buffer[BUF_SIZE];
volatile int write_pos = 0;
volatile int read_pos = 0;

volatile int shared_var = 0;   // variable partagée pour démo
spin_lock_t *lock;             // spinlock pour protéger buffer + shared_var


// ======================
//   Fonction Core 1
// ======================
void core1_entry()
{
    while (true)
    {

        // ----- Ecrire dans le buffer -----
        uint32_t irq = spin_lock_blocking(lock);

        int value = shared_var + 1;  // produit une nouvelle valeur
        shared_var++;                // modifie la variable partagée
        buffer[write_pos] = value;
        write_pos = (write_pos + 1) % BUF_SIZE;

        spin_unlock(lock, irq);
        // ---------------------------------------------------

        // Informer le core 0 qu'une nouvelle donnée est dispo
        multicore_fifo_push_blocking(1);

        sleep_ms(50);  // fréquence de production
    }
}



// ======================
//     Core 0 : main()
// ======================
int main()
{
    stdio_init_all();
    sleep_ms(2000); // Laisse le temps au terminal USB d’apparaître

    printf("=== Exemple complet multicore ===\n");

    lock = spin_lock_init(0);   // init spinlock n°0

    multicore_launch_core1(core1_entry);  // démarre core1

    while (true)
    {
        // Attendre un message de core1
        multicore_fifo_pop_blocking();

        // ----- Lire buffer + shared_var -----
        uint32_t irq = spin_lock_blocking(lock);

        int value = buffer[read_pos];
        read_pos = (read_pos + 1) % BUF_SIZE;

        int shared_copy = shared_var;

        spin_unlock(lock, irq);
        // ------------------------------------------------------

        // Affichage
        printf("Lu = %d    |    shared_var = %d\n", value, shared_copy);

        // Petit délai pour éviter de saturer l’USB
        sleep_ms(50);
    }
}
