#include <unistd.h>
#include <stdlib.h>
#include "cmdqueue/list.h"
#include "ctest/ctest.h"

CTEST(list, test_init) {
    struct list_tag L;
    list_init(&L);
    ASSERT_NOT_NULL(L.next);
    ASSERT_NOT_NULL(L.prev);
}

CTEST(list, test_add_tail) {
    struct list_tag L;
    struct list_tag L2;
    list_init(&L);
    list_init(&L2);
    ASSERT_EQUAL(0, list_count(&L));
    list_add_tail(&L, &L2);
    ASSERT_EQUAL(1, list_count(&L));
}

