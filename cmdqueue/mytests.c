#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "ctest.h"

#include "mycmdqueue.h"

static int32_t start = 0;
static int32_t stop = 0;

static int32_t cmd_callback(void *cookie, int32_t callback_type, void *data, int32_t len)
{
    switch (callback_type) {
        case START_CALLED:
            start = 1;
        break;
        case STOP_CALLED:
            stop = 1;
        break;
    }
    return 0;
}

CTEST(suite1, init) {
    mycmdqueue_t handle;

    mycmdqueue_init(&handle,
                    0,
                    0);
}

CTEST(suite1, deinit) {
    mycmdqueue_t handle;

    mycmdqueue_init(&handle,
                    0,
                    0);

    mycmdqueue_deinit(handle);
}

CTEST(suite1, start) {
    mycmdqueue_t handle;

    mycmdqueue_init(&handle,
                    0,
                    cmd_callback);
    
    mycmdqueue_start(handle);
    
    ASSERT_EQUAL(start, 1);

    mycmdqueue_deinit(handle);
}

CTEST(suite1, stop) {
    mycmdqueue_t handle;

    mycmdqueue_init(&handle,
                    0,
                    cmd_callback);
    
    mycmdqueue_stop(handle);

    ASSERT_EQUAL(stop, 1);

    mycmdqueue_deinit(handle);
}
