#ifndef MEMSAFE_H
#define MEMSAFE_H

void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);

#endif
 