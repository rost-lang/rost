
#include <stdarg.h>
#include "rost_internal.h"

template class ptr_vec<rost_task>;

rost_dom::rost_dom(rost_srv *srv, rost_crate const *root_crate) :
    interrupt_flag(0),
    root_crate(root_crate),
    _log(srv, this),
    srv(srv),
    running_tasks(this),
    blocked_tasks(this),
    dead_tasks(this),
    caches(this),
    root_task(NULL),
    curr_task(NULL),
    rval(0)
{
    logptr("new dom", (uintptr_t)this);
    memset(&rctx, 0, sizeof(rctx));

#ifdef __WIN32__
    {
        HCRYPTPROV hProv;
        win32_require
            (_T("CryptAcquireContext"),
             CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL,
                                 CRYPT_VERIFYCONTEXT|CRYPT_SILENT));
        win32_require
            (_T("CryptGenRandom"),
             CryptGenRandom(hProv, sizeof(rctx.randrsl),
                            (BYTE*)(&rctx.randrsl)));
        win32_require
            (_T("CryptReleaseContext"),
             CryptReleaseContext(hProv, 0));
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    I(this, fd > 0);
    I(this, read(fd, (void*) &rctx.randrsl, sizeof(rctx.randrsl))
      == sizeof(rctx.randrsl));
    I(this, close(fd) == 0);
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024);
    pthread_attr_setdetachstate(&attr, true);
#endif
    randinit(&rctx, 1);

    root_task = new (this) rost_task(this, NULL);
}

static void
del_all_tasks(rost_dom *dom, ptr_vec<rost_task> *v) {
    I(dom, v);
    while (v->length()) {
        dom->log(rost_log::TASK, "deleting task %" PRIdPTR, v->length() - 1);
        delete v->pop();
    }
}

rost_dom::~rost_dom() {
    log(rost_log::TASK, "deleting all running tasks");
    del_all_tasks(this, &running_tasks);
    log(rost_log::TASK, "deleting all blocked tasks");
    del_all_tasks(this, &blocked_tasks);
    log(rost_log::TASK, "deleting all dead tasks");
    del_all_tasks(this, &dead_tasks);
#ifndef __WIN32__
    pthread_attr_destroy(&attr);
#endif
    while (caches.length())
        delete caches.pop();
}

void
rost_dom::activate(rost_task *task) {
    curr_task = task;
    root_crate->get_activate_glue()(task);
    curr_task = NULL;
}

void
rost_dom::log(uint32_t type_bits, char const *fmt, ...) {
    char buf[256];
    if (_log.is_tracing(type_bits)) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        _log.trace_ln(type_bits, buf);
        va_end(args);
    }
}

rost_log &
rost_dom::get_log() {
    return _log;
}

void
rost_dom::logptr(char const *msg, uintptr_t ptrval) {
    log(rost_log::MEM, "%s 0x%" PRIxPTR, msg, ptrval);
}

template<typename T> void
rost_dom::logptr(char const *msg, T* ptrval) {
    log(rost_log::MEM, "%s 0x%" PRIxPTR, msg, (uintptr_t)ptrval);
}


void
rost_dom::fail() {
    log(rost_log::DOM, "domain 0x%" PRIxPTR " root task failed", this);
    I(this, rval == 0);
    rval = 1;
}

void *
rost_dom::malloc(size_t sz) {
    void *p = srv->malloc(sz);
    I(this, p);
    log(rost_log::MEM, "rost_dom::malloc(%d) -> 0x%" PRIxPTR,
        sz, p);
    return p;
}

void *
rost_dom::calloc(size_t sz) {
    void *p = this->malloc(sz);
    memset(p, 0, sz);
    return p;
}

void *
rost_dom::realloc(void *p, size_t sz) {
    void *p1 = srv->realloc(p, sz);
    I(this, p1);
    log(rost_log::MEM, "rost_dom::realloc(0x%" PRIxPTR ", %d) -> 0x%" PRIxPTR,
        p, sz, p1);
    return p1;
}

void
rost_dom::free(void *p) {
    log(rost_log::MEM, "rost_dom::free(0x%" PRIxPTR ")", p);
    I(this, p);
    srv->free(p);
}

#ifdef __WIN32__
void
rost_dom::win32_require(LPCTSTR fn, BOOL ok) {
    if (!ok) {
        LPTSTR buf;
        DWORD err = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, err,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &buf, 0, NULL );
        log(rost_log::ERR, "%s failed with error %ld: %s", fn, err, buf);
        LocalFree((HLOCAL)buf);
        I(this, ok);
    }
}
#endif

size_t
rost_dom::n_live_tasks()
{
    return running_tasks.length() + blocked_tasks.length();
}

void
rost_dom::add_task_to_state_vec(ptr_vec<rost_task> *v, rost_task *task)
{
    log(rost_log::MEM|rost_log::TASK,
        "adding task 0x%" PRIxPTR " in state '%s' to vec 0x%" PRIxPTR,
        (uintptr_t)task, state_vec_name(v), (uintptr_t)v);
    v->push(task);
}


void
rost_dom::remove_task_from_state_vec(ptr_vec<rost_task> *v, rost_task *task)
{
    log(rost_log::MEM|rost_log::TASK,
        "removing task 0x%" PRIxPTR " in state '%s' from vec 0x%" PRIxPTR,
        (uintptr_t)task, state_vec_name(v), (uintptr_t)v);
    I(this, (*v)[task->idx] == task);
    v->swapdel(task);
}

const char *
rost_dom::state_vec_name(ptr_vec<rost_task> *v)
{
    if (v == &running_tasks)
        return "running";
    if (v == &blocked_tasks)
        return "blocked";
    I(this, v == &dead_tasks);
    return "dead";
}

void
rost_dom::reap_dead_tasks()
{
    for (size_t i = 0; i < dead_tasks.length(); ) {
        rost_task *t = dead_tasks[i];
        if (t == root_task || t->refcnt == 0) {
            I(this, !t->waiting_tasks.length());
            dead_tasks.swapdel(t);
            log(rost_log::TASK,
                "deleting unreferenced dead task 0x%" PRIxPTR, t);
            delete t;
            continue;
        }
        ++i;
    }
}

rost_task *
rost_dom::sched()
{
    I(this, this);
    // FIXME: in the face of failing tasks, this is not always right.
    // I(this, n_live_tasks() > 0);
    if (running_tasks.length() > 0) {
        size_t i = rand(&rctx);
        i %= running_tasks.length();
        return (rost_task *)running_tasks[i];
    }
    log(rost_log::DOM|rost_log::TASK,
        "no schedulable tasks");
    return NULL;
}

rost_crate_cache *
rost_dom::get_cache(rost_crate const *crate) {
    log(rost_log::CACHE,
        "looking for crate-cache for crate 0x%" PRIxPTR, crate);
    rost_crate_cache *cache = NULL;
    for (size_t i = 0; i < caches.length(); ++i) {
        rost_crate_cache *c = caches[i];
        if (c->crate == crate) {
            cache = c;
            break;
        }
    }
    if (!cache) {
        log(rost_log::CACHE,
            "making new crate-cache for crate 0x%" PRIxPTR, crate);
        cache = new (this) rost_crate_cache(this, crate);
        caches.push(cache);
    }
    cache->ref();
    return cache;
}


//
// Local Variables:
// mode: C++
// fill-column: 70;
// indent-tabs-mode: nil
// c-basic-offset: 4
// buffer-file-coding-system: utf-8-unix
// compile-command: "make -k -C .. 2>&1 | sed -e 's/\\/x\\//x:\\//g'";
// End:
//