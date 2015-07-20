#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define K (24)
typedef struct list_t list_t;
struct list_t {
	unsigned reserved: 1; /* one if reserved. */
	unsigned char kval; /* current value of K. */
	list_t* succ; /* successor block in list. */
	list_t* pred; /* predecessor block in list. */
};
static list_t* frelist[K]; /* refers to the frelist with available blocks of size 2^K*/
static int init = 0;
static char* start;

void xprint(int now) {
	int i;
	for (i = 0; i < K; ++i)
	{
		if (i == now) {
			fprintf(stderr, "K: %d->\t", i);
		} else {
			fprintf(stderr, "K: %d\t", i);
		}

		list_t *p = frelist[i];
		int j = 0;
		while (p != NULL && j < 5) {
			fprintf(stderr, "%d %p->\t", p->kval, ((char*)p) - (size_t)start);
			p = p->succ;
			j++;
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}
void *malloc(size_t size) {
	if (size == 0) {
		return NULL;
	}
	if (init == 0) {
		start = sbrk(1 << (K - 1) );
		if (start == (void*) - 1) {
			// fprintf(stderr, "error sbrk\n");
			return NULL;
		}
		if ((unsigned long long)start % 8 != 0) {
			// fprintf(stderr, "error align\n");
			return NULL;
		}
		list_t* t = start;
		memset(t, 0, sizeof(list_t));
		t->kval = K - 1;
		frelist[K - 1] = t;
		init = 1;
	}
	size_t s = size + sizeof(list_t);
	int i = 0;
	while (s) {
		s = s >> 1;
		i++;
	}

	int j = i;
	while (frelist[j] == NULL) {
		j++;
		if (j == K) {
			// fprintf(stderr, "my out of memory\n");
			return NULL;
		}
	}
	while (j > i) {
		char* l = frelist[j];
		frelist[j] = frelist[j]->succ;
		if (frelist[j] != NULL) {
			frelist[j]->pred = NULL;
		}
		j--;
		list_t *t1 = (list_t*)l;
		list_t *t2 = (list_t*)(l + (1 << (j)));
		memset(t1, 0, sizeof(list_t));
		memset(t2, 0, sizeof(list_t));
		t1->succ = t2;
		t2->succ = frelist[j];
		t2->pred = t1;
		if (frelist[j] != NULL) {
			frelist[j]->pred = t2;
		}
		t1->kval = j;
		t2->kval = j;
		frelist[j] = t1;
	}
	char* v = (char*)frelist[i];
	frelist[i]->reserved = 1;
	frelist[i] = frelist[i]->succ;
	if (frelist[i] != NULL) {
		frelist[i]->pred = NULL;
	}
	v += sizeof(list_t);
	return v;

}


void free(void *ptr) {
	if (ptr == 0) {
		return;
	}
	list_t *l = (list_t*)((char*)ptr - sizeof(list_t));
	if (l->reserved == 0) {
		return;
	}
	l->reserved = 0;
	unsigned char k = l->kval;

	char* addrz = (char*)l - (size_t)start;
	while (k<K-1) {
		char* buddyc = (size_t)addrz ^ (size_t)((1 << k));

		list_t* buddy = buddyc +  (size_t)start;

		if (buddy->reserved == 0 && buddy->kval == k) {
			if (buddy->pred != NULL) {
				buddy->pred->succ = buddy->succ;
			} else {
				frelist[k] = NULL;
			}
			if (buddy->succ != NULL) {
				buddy->succ->pred = buddy->pred;
			}
			addrz = (size_t)addrz & (size_t)~(1 << k);
			k++;
		}else{
			break;
		}
	}
	l = addrz + (size_t)start;
	memset(l, 0, sizeof(list_t));
	l->kval = k;
	l->succ = frelist[k];
	if (frelist[k] != NULL) {
		frelist[k]->pred = l;
	}
	frelist[k] = l;

}

void *calloc(size_t nmemb, size_t size) {
	if (nmemb == 0 || size == 0) {
		return NULL;
	}
	void *v = malloc(nmemb * size);
	if (v == NULL)
	{
		return NULL;
	}
	memset(v, 0, nmemb * size);
	return v;
}

void *realloc(void *ptr, size_t size) {
	if (ptr == 0) {
		return malloc(size);
	}
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	list_t *l = (list_t*)((char*)ptr - sizeof(list_t));
	size_t old = (1 << (l->kval)) - sizeof(list_t);
	if (old >= size) {
		return ptr;
	}
	void *v = malloc(size);
	if (v == NULL) {
		return NULL;
	}
	memcpy(v, ptr, old);
	free(ptr);
	return v;
}