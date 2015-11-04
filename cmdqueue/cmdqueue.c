#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "pthread.h"

#include "cmdqueue.h"
#include "util.h"

enum cmdqueue_mode {
    CMDQUEUE_ASYNC = 0x0,
    CMDQUEUE_SYNC  = 0x1,
};

enum cmdqueue_prio {
    CMDQUEUE_PRIO_LOW  = 0x0,
    CMDQUEUE_PRIO_HIGH = 0x1,
};

struct queue
{
    struct list_tag head_prio;
    struct list_tag head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

enum queue_type {
    CMDFREE,
    CMDTODO,
    CMDDONE,
};

struct cmdqueue_tag {
    struct queue queues[3];
    const char *name;
    pthread_t tid;
    int32_t stop;
    void *cookie;
    void (*cmd_callback)(void *cookie, struct cmd * cmd);
    void *cmdlist;
};

static void cmdqueue_wait_getcmd(cmdqueue_t handle,
                                 struct cmd **cmd)
{
    list_t node;

    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDFREE].mutex));

    while (!list_count(&handle->queues[CMDFREE].head)) {
        PTHREAD_CHK(pthread_cond_wait(&handle->queues[CMDFREE].cond,
                    &handle->queues[CMDFREE].mutex));
    }

    node = handle->queues[CMDFREE].head.next;
    list_remove(node);
    *cmd = (struct cmd*)node;

    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDFREE].mutex));
}

void cmdqueue_getcmd_sync(cmdqueue_t handle,
                          struct cmd **cmd)
{
    cmdqueue_getcmd_async(handle,
                          cmd);
    if (!*cmd) cmdqueue_wait_getcmd(handle,
                                    cmd);
}

void cmdqueue_getcmd_async(cmdqueue_t handle,
                           struct cmd **cmd)
{
    list_t node;
    *cmd = 0;

    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDFREE].mutex));

    if (list_count(&handle->queues[CMDFREE].head)) {
        node = handle->queues[CMDFREE].head.next;
        list_remove(node);
        *cmd = (struct cmd*)node;
    }

    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDFREE].mutex));
}

static void cmdqueue_schedule_cmd(cmdqueue_t handle,
                                  struct cmd *cmd,
                                  int32_t sync,
                                  int32_t prio)
{
    list_t list = (prio == CMDQUEUE_PRIO_LOW) ? &handle->queues[CMDTODO].head : &handle->queues[CMDTODO].head_prio;
    cmd->type = sync;
    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDTODO].mutex));
    list_add_tail(list,
                  &cmd->head);
    PTHREAD_CHK(pthread_cond_broadcast(&handle->queues[CMDTODO].cond));
    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDTODO].mutex));
}

static int32_t cmd_finished(list_t const src, struct cmd *cmd)
{
    uint64_t count = 0;
    int32_t done = 0;
    list_t node = src->next;

    while (node != src) {
        count++;
        node = node->next;
    }

    if (count > 0) {
        node = src->next;

        while (node != src && !done) {
            done = (struct cmd *)node == cmd;
            node = node->next;
        }
    }

    return done;
}

static void cmdqueue_wait_cmd(cmdqueue_t handle, struct cmd *cmd)
{

    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDDONE].mutex));

    while (!cmd_finished(&handle->queues[CMDDONE].head, cmd) ) {
        PTHREAD_CHK(pthread_cond_wait(&handle->queues[CMDDONE].cond,
                    &handle->queues[CMDDONE].mutex));
    }

    list_remove(&cmd->head);

    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDDONE].mutex));

    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDFREE].mutex));
    list_add_front(&handle->queues[CMDFREE].head, &cmd->head);
    PTHREAD_CHK(pthread_cond_broadcast(&handle->queues[CMDFREE].cond));
    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDFREE].mutex));
}

void cmdqueue_sync_cmd(cmdqueue_t handle,
                       struct cmd *cmd)
{
    cmdqueue_schedule_cmd(handle,
                          cmd,
                          CMDQUEUE_SYNC,
                          CMDQUEUE_PRIO_LOW);

    cmdqueue_wait_cmd(handle, cmd);
}

void cmdqueue_sync_highprio_cmd(cmdqueue_t handle,
                                struct cmd *cmd)
{
    cmdqueue_schedule_cmd(handle,
                          cmd,
                          CMDQUEUE_SYNC,
                          CMDQUEUE_PRIO_HIGH);

    cmdqueue_wait_cmd(handle, cmd);
}

void cmdqueue_async_cmd(cmdqueue_t handle,
                        struct cmd *cmd)
{
    cmdqueue_schedule_cmd(handle,
                          cmd,
                          CMDQUEUE_ASYNC,
                          CMDQUEUE_PRIO_LOW);
}

static void * thread_func(void *arg)
{
    cmdqueue_t handle= (cmdqueue_t)arg;
    struct cmd dummy_cmd;

    while (1) {
        struct cmd *cmd = &dummy_cmd;
        enum queue_type type;

        PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDTODO].mutex));

        while (!list_count(&handle->queues[CMDTODO].head) && !list_count(&handle->queues[CMDTODO].head_prio) && !handle->stop) {
            PTHREAD_CHK(pthread_cond_wait(&handle->queues[CMDTODO].cond,
                        &handle->queues[CMDTODO].mutex));
        }

        if (!handle->stop) {
            if (list_count(&handle->queues[CMDTODO].head_prio)) {
                cmd = (struct cmd*)handle->queues[CMDTODO].head_prio.next;
                list_remove(&cmd->head);
            } else {
                cmd = (struct cmd*)handle->queues[CMDTODO].head.next;
                list_remove(&cmd->head);
            }
        }

        PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDTODO].mutex));

        if (handle->stop) return 0;

        handle->cmd_callback(handle->cookie, cmd);

        type = cmd->type ? CMDDONE : CMDFREE;

        PTHREAD_CHK(pthread_mutex_lock(&handle->queues[type].mutex));
        list_add_tail(&handle->queues[type].head, &cmd->head);
        PTHREAD_CHK(pthread_cond_broadcast(&handle->queues[type].cond));
        PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[type].mutex));
    }

    return 0;
}

int32_t cmdqueue_init(cmdqueue_t *handle,
                      const char *name,
                      void (*cmd_callback)(void *cookie, struct cmd *cmd),
                      void *cookie,
                      uint32_t num_commands,
                      uint32_t size_cmd)
{
    *handle = malloc(sizeof(struct cmdqueue_tag));

    if (*handle) {
        cmdqueue_t phandle = *handle;
        uint8_t *iter;
        uint32_t i;

        for (i=0;i<ARRAY_SIZE(phandle->queues);i++) {
            list_init(&phandle->queues[i].head);
            list_init(&phandle->queues[i].head_prio);
            PTHREAD_CHK(pthread_mutex_init(&phandle->queues[i].mutex, 0));
            PTHREAD_CHK(pthread_cond_init(&phandle->queues[i].cond, 0));
        }

        phandle->stop = 0;
        phandle->cookie = cookie;
        phandle->cmd_callback = cmd_callback;
        phandle->name = name;

        phandle->cmdlist = malloc(num_commands*size_cmd);

        iter = (uint8_t*)phandle->cmdlist;
        for (i=0;i<num_commands;i++) {
            struct cmd *cmd = (struct cmd*)iter;
            list_add_tail(&phandle->queues[CMDFREE].head,
                          &cmd->head);
            iter += size_cmd;
        }

        PTHREAD_CHK(pthread_create(&phandle->tid, 
                                   0, 
                                   thread_func, 
                                   phandle));
    } else {
        return 1;
    }
    
    return 0;
}

int32_t cmdqueue_deinit(cmdqueue_t handle)
{
    uint32_t i;
    if (!handle) return 1;

    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDTODO].mutex));
    handle->stop = 1;
    PTHREAD_CHK(pthread_cond_broadcast(&handle->queues[CMDTODO].cond));
    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDTODO].mutex));

    PTHREAD_CHK(pthread_join(handle->tid, 0));

    for (i=0;i<ARRAY_SIZE(handle->queues);i++) {
        PTHREAD_CHK(pthread_mutex_destroy(&handle->queues[i].mutex));
        PTHREAD_CHK(pthread_cond_destroy(&handle->queues[i].cond));
    }

    free(handle->cmdlist);
    free(handle);
    return 0;
}

/* only for async commands on the non prio head */
void cmdqueue_flush(cmdqueue_t handle,
                    void (*flush_callback)(void *cookie, struct cmd *cmd, uint32_t *count),
                    void *cookie,
                    uint32_t *count)
{
    list_t src, dest, node;

    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDTODO].mutex));
    PTHREAD_CHK(pthread_mutex_lock(&handle->queues[CMDFREE].mutex));

    src = &handle->queues[CMDTODO].head;
    dest = &handle->queues[CMDFREE].head;
    node = src->next;

    while (node != src) {
        list_t const tmp_node = node;
        node = node->next;
        list_remove(tmp_node);
        flush_callback(cookie, (struct cmd*)tmp_node, count);
        list_add_tail(dest, tmp_node);
    }

    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDFREE].mutex));
    PTHREAD_CHK(pthread_mutex_unlock(&handle->queues[CMDTODO].mutex));
}
