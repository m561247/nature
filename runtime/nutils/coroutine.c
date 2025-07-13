#include "runtime/processor.h"

coroutine_t *rt_coroutine_async(void *fn, int64_t flag, n_future_t *fu) {
    coroutine_t *co = rt_coroutine_new(fn, flag, fu, NULL);
    rt_coroutine_dispatch(co);
    DEBUGF("[rt_coroutine_async] co=%p, fn=%p, flag=%ld, fu=%p, size=%ld", co, fn, flag, fu, fu->size);

    return co;
}

void rt_coroutine_yield() {
    n_processor_t *p = processor_get();
    co_yield_runnable(p, p->coroutine);
}

void rt_select_block() {

    co_yield_waiting(coroutine_get(), NULL, NULL);
}


void *rt_coroutine_arg() {
    coroutine_t *co = coroutine_get();
    return co->arg;
}

static void uv_timer_close_cb(uv_handle_t *handle) {
    free(handle);
}

/**
 * repeat 为 0， 所以不会重复执行，所以不需要手动钓孙 uv_stop_timer 停止计时器
 * @param timer
 */
static void uv_on_timer(uv_timer_t *timer) {
    RDEBUGF("[rt_coroutine_sleep.uv_on_timer] callback start, timer=%p, timer->data=%p", timer, timer->data);
    coroutine_t *co = timer->data;

    // - 标记 coroutine 并推送到可调度队列中等待 processor handle
    n_processor_t *p = co->p;
    assert(p);

    TRACEF("[rt_coroutine_sleep.uv_on_timer] will push to runnable_list, p_index=%d, co=%p, status=%d",
           p->index, co, co->status);

    // timer 到时间了, push 到尾部等待调度
    assert(p->status != P_STATUS_EXIT);
    co->status = CO_STATUS_RUNNABLE;
    rt_linked_fixalloc_push(&p->runnable_list, co);

    TRACEF("[rt_coroutine_sleep.uv_on_timer] will stop and clear timer=%p, p_index=%d, co=%p, status=%d", timer,
           p->index, co, co->status);

    uv_timer_stop(timer);

    // 注册 close 事件而不是瞬时 close!
    uv_close((uv_handle_t *) timer, uv_timer_close_cb);

    TRACEF("[rt_coroutine_sleep.uv_on_timer] success stop and clear timer=%p, p_index=%d, co=%p, status=%d", timer,
           p->index, co, co->status);
}

void rt_coroutine_sleep(int64_t ms) {
    n_processor_t *p = processor_get();
    coroutine_t *co = coroutine_get();

    // - 初始化 libuv 定时器(io_run 回调会读取 timer 的地址，所以需要在堆中分配)
    uv_timer_t *timer = NEW(uv_timer_t);
    uv_timer_init(&p->uv_loop, timer);
    timer->data = co;

    // 设定定时器超时时间与回调
    uv_timer_start(timer, uv_on_timer, ms, 0);

    DEBUGF("[runtime.rt_coroutine_sleep] start, co=%p uv_loop=%p, p_index=%d, timer=%p, timer_value=%lu", co,
            &p->uv_loop, p->index, &timer, fetch_addr_value((addr_t) &timer));

    // 退出等待 io 事件就绪
    co_yield_waiting(co, NULL, NULL);

    DEBUGF(
            "[runtime.rt_coroutine_sleep] coroutine sleep resume, co=%p, co_status=%d, uv_loop=%p, p_index=%d, timer=%p",
            co, co->status,
            &p->uv_loop, p->index, &timer);
}


int64_t rt_processor_index() {
    coroutine_t *co = coroutine_get();
    return co->p->index;
}
