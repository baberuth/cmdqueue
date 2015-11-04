#ifndef CMDQUEUE_H
#define CMDQUEUE_H

#include <stdint.h>
#include "pthread.h"

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct cmd {
        struct list_tag head;
        int32_t type;
    };

    typedef struct cmdqueue_tag * cmdqueue_t;

    int32_t cmdqueue_init(cmdqueue_t *handle,
                          const char *name,
                          void (*cmd_callback)(void *cookie,
                                               struct cmd *cmd),
                          void *cookie,
                          uint32_t num_commands,
                          uint32_t size_cmd);

    int32_t cmdqueue_deinit(cmdqueue_t handle);

    void cmdqueue_flush(cmdqueue_t handle,
                        void (*flush_callback)(void *cookie,
                                               struct cmd *cmd,
                                               uint32_t *count),
                        void *cookie,
                        uint32_t *count);

    void cmdqueue_getcmd_sync(cmdqueue_t handle,
                              struct cmd **cmd);

    void cmdqueue_getcmd_async(cmdqueue_t handle,
                               struct cmd **cmd);

    void cmdqueue_sync_cmd(cmdqueue_t handle,
                           struct cmd *cmd);

    void cmdqueue_sync_highprio_cmd(cmdqueue_t handle,
                                    struct cmd *cmd);

    void cmdqueue_async_cmd(cmdqueue_t handle,
                            struct cmd *cmd);

#ifdef __cplusplus
}
#endif

#endif

