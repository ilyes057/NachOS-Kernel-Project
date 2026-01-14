#include "syscall.h"
#include "mem.h"

#ifndef PageSize
#define PageSize 128
#endif

extern int SBRK(int n);

header_t *header = NULL;
#define MEM_ALIGNMENT 8
static mem_fit_function_t *fit_handler = mem_first_fit;


void* mem_init(unsigned int size) {
    if (size < sizeof(header_t) + sizeof(mem_block_t)) {
        return NULL;
    }

    int pages = (int)((size + PageSize - 1) / PageSize);
    unsigned int real_size = (unsigned int)pages * (unsigned int)PageSize;
    int addr = SBRK(pages);
    if (addr == -1) return 0;

    void *memory_start=(void *)addr;

    header = (header_t *)memory_start;
    header->mem_first_free = (mem_block_t *)(header + 1);
    header->mem_first_used = NULL;
    header->mem_first_free->size = real_size - sizeof(header_t)-sizeof(mem_block_t);
    header->mem_first_free->next = NULL;

    return memory_start;
}

void mem_set_fit_handler(mem_fit_function_t *mff) {
	fit_handler = mff;
}

void *mem_alloc(unsigned int size) {
	//Vérification des paramètres
	if (!header) {
		return NULL; 
    }
	// Align the size to MEM_ALIGNMENT
	unsigned int aligned_size =
        (size + (MEM_ALIGNMENT - 1)) & ~(MEM_ALIGNMENT - 1);
	unsigned int needed_size = aligned_size + sizeof(mem_block_t);
	//Recherche d'un bloc libre adéquat
    mem_block_t *prev_free_block = NULL;
    mem_block_t *current_free_block = header->mem_first_free;
	mem_block_t *suitable_block = fit_handler(current_free_block, needed_size);
	if (!suitable_block) {
		return NULL; // Pas de bloc libre adéquat trouvé
	}
	// Retrouver le prédécesseur
    while (current_free_block != suitable_block) {
        prev_free_block = current_free_block;
        current_free_block = current_free_block->next;
        if (!current_free_block) {
            return NULL; // Liste corrompue, aucune correspondance trouvée
        }
    }
	//calcule de la taille restante après allocation
	unsigned int total_free_block_size =
        suitable_block->size + sizeof(mem_block_t);
	unsigned int remaining_size = total_free_block_size - needed_size;
	//Création du bloc occupé
	if (remaining_size < sizeof(mem_block_t)+MEM_ALIGNMENT) {
		//Utilisation complète du bloc libre	
		if (prev_free_block) {
			prev_free_block->next = suitable_block->next;
		} else {
			header->mem_first_free = suitable_block->next;
		}
	} else {
		//Division du bloc libre
		mem_block_t *new_free_block = (mem_block_t *)((char *)suitable_block + needed_size);
		new_free_block->size = remaining_size - sizeof(mem_block_t);	
		new_free_block->next = suitable_block->next;
		if (prev_free_block) {
			prev_free_block->next = new_free_block;
		} else {
			header->mem_first_free = new_free_block;
	}
	}
	//Insertion du bloc occupé dans la liste des blocs occupés
	mem_block_t *new_used_block = (mem_block_t *)suitable_block;
	new_used_block->size = aligned_size;
	new_used_block->next = header->mem_first_used;
	header->mem_first_used = new_used_block;
	//Retour de l'adresse utilisable
	return (void *)(new_used_block + 1);
}

void mem_free(void *zone) {
    if (!header || !zone) {
        return; // Vérification de l'initialisation et du paramètre
    }
    mem_block_t *prev_used_block = NULL;
    mem_block_t *current_used_block = header->mem_first_used;
    mem_block_t *block_to_free = (mem_block_t *)zone - 1;
    //Recherche du bloc occupé à libérer
    while (current_used_block && current_used_block != block_to_free) {
        prev_used_block = current_used_block;
        current_used_block = current_used_block->next;
    }
    if (!current_used_block) {
        return; // Le bloc n'a pas été trouvé dans la liste des blocs occupés
    }
    //Retirer le bloc de la liste des blocs occupés
    if (prev_used_block) {
        prev_used_block->next = current_used_block->next;
    } else {
        header->mem_first_used = current_used_block->next;
    }
    //Création du bloc libre
    mem_block_t *new_free_block = (mem_block_t *)block_to_free;
    new_free_block->size = block_to_free->size;
    new_free_block->next = NULL;
    //Insertion du bloc libre dans la liste des blocs libres
    if (!header->mem_first_free || (char *)new_free_block < (char *)header->mem_first_free) {
        new_free_block->next = header->mem_first_free;
        header->mem_first_free = new_free_block;
    } else {
        mem_block_t *prev_free_block = header->mem_first_free;
        mem_block_t *current_free_block = header->mem_first_free->next;
        while (current_free_block && (char *)new_free_block > (char *)current_free_block) {
            prev_free_block = current_free_block;
            current_free_block = current_free_block->next;
        }
        new_free_block->next = current_free_block;
        prev_free_block->next = new_free_block;
        // Fusion avec le bloc libre précédent si possible
        if ((char *)prev_free_block + sizeof(mem_block_t) + prev_free_block->size == (char *)new_free_block) {
            prev_free_block->size += sizeof(mem_block_t) + new_free_block->size;
            prev_free_block->next = new_free_block->next;
            new_free_block = prev_free_block; // Met à jour new_free_block pour la fusion suivante
        }
    }
    //Fusion avec le bloc libre suivant si possible
    if (new_free_block->next && 
        (char *)new_free_block + sizeof(mem_block_t) + new_free_block->size == (char *)new_free_block->next) {
        new_free_block->size += sizeof(mem_block_t) + new_free_block->next->size;
        new_free_block->next = new_free_block->next->next;
    }
}

mem_block_t *mem_first_fit(mem_block_t *first_free_block, unsigned int wanted_size) {
    mem_block_t *current = first_free_block;

    while (current != NULL) {
        // La taille totale du bloc libre est : Taille des données (current->size) + Taille de son entéte
        unsigned int total_free_size = current->size + sizeof(mem_block_t);
        // Si la taille totale du bloc libre est supérieure ou égale à la taille requise,
        // ce bloc convient.
        if (total_free_size >= wanted_size) {
            return current; // On retourne le bloc trouvé
        }
        current = current->next;
    }
    return NULL; // Aucun bloc libre suffisant trouvé
}
//-------------------------------------------------------------
mem_block_t *mem_best_fit(mem_block_t *first_free_block, unsigned int wanted_size) {
    mem_block_t *current = first_free_block;
    mem_block_t *best = NULL;
    unsigned int best_size = (unsigned int)-1;
    while (current != NULL) {
        unsigned int total_free_size = current->size + sizeof(mem_block_t);
        if (total_free_size >= wanted_size && total_free_size-wanted_size<best_size) {
            best=current;
            best_size=total_free_size-wanted_size;
        }
        current = current->next;
    }
    if (best){
        return best;
    }
    return NULL; // Aucun bloc libre suffisant trouvé
}

//-------------------------------------------------------------
mem_block_t *mem_worst_fit(mem_block_t *first_free_block, unsigned int wanted_size) {
    mem_block_t *current = first_free_block;
    mem_block_t *worst = NULL;
    unsigned int worst_size=0; 
    while (current != NULL) {
        unsigned int total_free_size = current->size + sizeof(mem_block_t);
        if (total_free_size >= wanted_size && total_free_size-wanted_size>worst_size) {
            worst=current;
            worst_size=total_free_size-wanted_size;
        }
        current = current->next;
    }
    if (worst){
        return worst;
    }
    return NULL;
}
void mem_show(void (*print)(void *, unsigned int, int free)) {
    mem_block_t *courant_free = header->mem_first_free;
    mem_block_t *courant_used = header->mem_first_used;
    int compteur_de_bloc_libre = 0, compteur_de_bloc_occupe = 0;
    unsigned int mem_free_size = 0, mem_used_size = 0;

    // Afficher les blocs libres
    while (courant_free) {
        print((void *)(courant_free + 1), courant_free->size, 1);
        mem_free_size += courant_free->size;
        compteur_de_bloc_libre++;
        courant_free = courant_free->next;
    }

    // Afficher les blocs occupés
    while (courant_used) {
        print((void *)(courant_used + 1), courant_used->size, 0);
        mem_used_size += courant_used->size;
        compteur_de_bloc_occupe++;
        courant_used = courant_used->next;
    }

    PutString("---\n");
    PutString("Blocs libres: ");
    PutInt(compteur_de_bloc_libre);
    PutString("\n");
    PutString("Blocs occupés: \n");
    PutInt(compteur_de_bloc_occupe);
    PutString("\n");
    PutString("Mémoire libre totale: octets\n");
    PutInt(mem_free_size);
    PutString("\n");
    PutString("Mémoire occupée totale: octets\n");
    PutInt(mem_used_size);
    PutString("\n");
}