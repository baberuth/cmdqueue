#ifndef MYCMDQUEUE_H
#define MYCMDQUEUE_H

#include <stdint.h>

#include "cmdqueue.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    enum mycmd_type {
        INIT,
        DEINIT,
        START,
        STOP,
    };

    enum mycallback_type {
        START_CALLED,
        STOP_CALLED,
    };

    struct mycmd {
        struct cmd cmd;
        int32_t type;
        union {            
            struct {
                int32_t dontcare;
            } mycmd_start;

            struct {
                int32_t dontcare;
            } mycmd_stop;
        } mycmd;
    };

    typedef struct mycmdqueue_tag * mycmdqueue_t;

    int32_t mycmdqueue_init(mycmdqueue_t * handle, 
                            void *cookie, 
                            int32_t (*mycmdqueue_callback)(void *cookie,
                                                           int32_t callback_type,
                                                           void *data,
                                                           int32_t len));
    int32_t mycmdqueue_deinit(mycmdqueue_t handle);
    void mycmdqueue_start(mycmdqueue_t handle);
    void mycmdqueue_stop(mycmdqueue_t handle);

#ifdef __cplusplus
}
#endif

#endif
