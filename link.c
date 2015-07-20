#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define K (24)

typedef struct list_t list_t;
struct list_t {
	size_t size;    /* size including list_t*/
	list_t* succ;   /* next available block. */
};

static int init = 0;
static list_t *avail;
void *malloc(size_t size) {
	if (size == 0) {
		return NULL;
	}
	if (init == 0) {
		void* start = sbrk(1 << K );
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
		t->size = (1 << K - 1);
		avail = t;
		init = 1;
	}

	size += sizeof(list_t) + 8;
	size -= (size % 8);
	list_t *now = avail;
	list_t *old = avail;
	while (now != NULL) {
		if (now->size >= size) {
			if (now->size < size + 64) {
				if (old == avail) {
					avail = now->succ;
					return (char*)now + sizeof(list_t);
				}
				old->succ = now->succ;
				now->succ = NULL;
				return (char*)now + sizeof(list_t);
			} else {
				size_t olds = now->size;
				now->size = size;
				list_t *ny = (char*)now + size;
				memset(ny, 0, sizeof(list_t));
				ny->size = olds - size;
				ny->succ = now->succ;
				if (old == avail) {
					avail = ny;
					return (char*)now + sizeof(list_t);
				}
				old->succ = ny;
				now->succ = NULL;
				return (char*)now + sizeof(list_t);
			}
		}
		old = now;
		now = now->succ;
	}
	return NULL;
}
void free(void *ptr) {
	if (ptr == 0) {
		return;
	}
	list_t *p = avail;
	list_t *q = p->succ;
	list_t* l = (list_t*)((char*)ptr - sizeof(list_t));

	while (q != NULL && l > q) {
		p = q;
		q = q->succ;
	}
	while (1) {
		if ((char*)l + l->size == (char*)q) {
			l->size = l->size + q->size;
			q = q->succ;
		} else {
			l->succ = q;
			break;
		}
	}
	p->succ = l;
}
void *calloc(size_t nmemb, size_t size) {
	if (nmemb == 0 || size == 0) {
		return NULL;
	}
	void *v = malloc(nmemb * size);
	if (v == NULL) {
		return NULL;
	}
	memset(v, 0, nmemb * size);
	return v;
}
void *realloc(void *ptr, size_t size) {
	if (ptr == 0)  {
		return malloc(size);
	}
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	list_t *l = (list_t*)((char*)ptr - sizeof(list_t));
	size_t old = l->size - sizeof(list_t);
	if (old >= size) {
		return ptr;
	}
	void *v = malloc(size);
	if (v == NULL)   {
		return NULL;
	}
	memcpy(v, ptr, old);
	free(ptr);
	return v;
}