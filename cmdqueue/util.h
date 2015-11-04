#ifndef UTIL_H
#define UTIL_H

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(*(a))))

#define PTHREAD_CHK(expr) do { if (expr != 0) {assert(0);fprintf((FILE *)2, "System call error\n");};} while(0)

#endif
