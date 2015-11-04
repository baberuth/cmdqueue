#include <stdint.h>
#include <stdlib.h>
#include "cmdqueue.h"
#include "mycmdqueue.h"

struct mycmdqueue_tag {
    cmdqueue_t cmdqueue;
    void *cookie;
    int32_t (*cmd_callback)(void *cookie, int32_t callback_type, void *data, int32_t len);
};

static void mycmd_callback(void *cookie, struct cmd *cmd)
{
    struct mycmd * mycmd = (struct mycmd *)cmd;
    mycmdqueue_t handle = (mycmdqueue_t)cookie;

    switch (mycmd->type) {
        case INIT:
        {
        }
        break;
        case DEINIT:
        {
        }
        break;
        case START:
        {
            if (handle->cmd_callback) handle->cmd_callback(handle->cookie, 
                                                           START_CALLED, 
                                                           0, 
                                                           0);
        }
        break;
        case STOP:
        {
            if (handle->cmd_callback) handle->cmd_callback(handle->cookie, 
                                                           STOP_CALLED, 
                                                           0, 
                                                           0);
        }
        break;
    }
}

int32_t mycmdqueue_init(mycmdqueue_t * handle, 
                        void *cookie, 
                        int32_t (*mycmdqueue_callback)(void *cookie,
                                                       int32_t callback_type,
                                                       void *data,
                                                       int32_t len))
{
    *handle = malloc(sizeof(struct mycmdqueue_tag));

    if (*handle) {
        const int32_t num_commands = 1;
        mycmdqueue_t phandle = *handle;
        struct mycmd *cmd;

        cmdqueue_init(&phandle->cmdqueue,
                      "mycmdqueue",
                      mycmd_callback,
                      phandle,
                      num_commands,
                      sizeof(struct mycmd));

        cmdqueue_getcmd_sync(phandle->cmdqueue, (struct cmd **)&cmd);
        cmd->type = INIT;
        cmdqueue_sync_cmd(phandle->cmdqueue, &cmd->cmd);

        phandle->cookie           = cookie;
        phandle->cmd_callback     = mycmdqueue_callback;

    }
    return 0;
}

int32_t mycmdqueue_deinit(mycmdqueue_t handle)
{
    struct mycmd *cmd;

    if (!handle) return 1;

    cmdqueue_getcmd_sync(handle->cmdqueue, (struct cmd **)&cmd);
    cmd->type = DEINIT;
    cmdqueue_sync_cmd(handle->cmdqueue, &cmd->cmd);

    cmdqueue_deinit(handle->cmdqueue);
    free(handle);

    return 0;
}

void mycmdqueue_start(mycmdqueue_t handle)
{
    struct mycmd *cmd;

    cmdqueue_getcmd_sync(handle->cmdqueue, 
                         (struct cmd **)&cmd);

    cmd->type = START;

    cmdqueue_sync_cmd(handle->cmdqueue, &cmd->cmd);
}

void mycmdqueue_stop(mycmdqueue_t handle)
{
    struct mycmd *cmd;

    cmdqueue_getcmd_sync(handle->cmdqueue, 
                         (struct cmd **)&cmd);

    cmd->type = STOP;

    cmdqueue_sync_cmd(handle->cmdqueue, &cmd->cmd);
}
