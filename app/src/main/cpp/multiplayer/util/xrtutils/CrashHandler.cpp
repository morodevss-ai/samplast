#include "CrashHandler.h"

#include <android/log.h>
#include <cstdio>
#include <csignal>
#include <cstdarg>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <ucontext.h>
#include <unistd.h>
#include <unwind.h>

#define TAG "CrashHandler"

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

static int g_crash_fd = -1;
static char g_crash_dir[PATH_MAX] = "/storage/emulated/0/GOLDRP/crashhandler";
static char g_crash_log_path[PATH_MAX] = "/storage/emulated/0/GOLDRP/crashhandler/crash.log";
static bool g_path_initialized = false;

static void build_crash_paths(const char* base_path)
{
    const char* resolved = (base_path && *base_path)
        ? base_path
        : "/storage/emulated/0/GOLDRP/";

    char normalized[PATH_MAX];
    size_t len = strnlen(resolved, sizeof(normalized) - 2);
    memcpy(normalized, resolved, len);
    normalized[len] = '\0';
    if (len > 0 && normalized[len - 1] != '/') {
        normalized[len++] = '/';
        normalized[len] = '\0';
    }

    snprintf(g_crash_dir, sizeof(g_crash_dir), "%scrashhandler", normalized);
    snprintf(g_crash_log_path, sizeof(g_crash_log_path), "%s/crash.log", g_crash_dir);
    g_path_initialized = true;
}

static void mkdirs(const char* path)
{
    if (!path || !*path) {
        return;
    }

    char tmp[PATH_MAX];
    size_t len = strnlen(path, sizeof(tmp) - 1);
    if (len == 0) {
        return;
    }
    memcpy(tmp, path, len);
    tmp[len] = '\0';

    for (char* p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static void close_crash_fd()
{
    if (g_crash_fd >= 0) {
        close(g_crash_fd);
        g_crash_fd = -1;
    }
}

static void open_crash_log_if_needed()
{
    if (g_crash_fd >= 0) {
        return;
    }

    if (!g_path_initialized) {
        build_crash_paths(nullptr);
        mkdirs(g_crash_dir);
    }

    g_crash_fd = open(g_crash_log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
}

static void crash_write(const char* buf, size_t len)
{
    if (g_crash_fd < 0 || !buf || len == 0) {
        return;
    }
    (void)write(g_crash_fd, buf, len);
}

static void log_line(const char* fmt, ...)
{
    open_crash_log_if_needed();

    va_list ap1;
    va_start(ap1, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, TAG, fmt, ap1);
    va_end(ap1);

    char buffer[1024];
    va_list ap2;
    va_start(ap2, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, ap2);
    va_end(ap2);

    if (len <= 0) {
        return;
    }
    if (len >= static_cast<int>(sizeof(buffer))) {
        len = static_cast<int>(sizeof(buffer)) - 1;
    }
    if (buffer[len - 1] != '\n') {
        if (len + 1 < static_cast<int>(sizeof(buffer))) {
            buffer[len++] = '\n';
        }
    }
    crash_write(buffer, static_cast<size_t>(len));
}

void crash_handler_logf(const char* fmt, ...)
{
    if (!fmt) {
        return;
    }

    open_crash_log_if_needed();

    va_list ap1;
    va_start(ap1, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, TAG, fmt, ap1);
    va_end(ap1);

    char buffer[1024];
    va_list ap2;
    va_start(ap2, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, ap2);
    va_end(ap2);

    if (len <= 0) {
        return;
    }
    if (len >= static_cast<int>(sizeof(buffer))) {
        len = static_cast<int>(sizeof(buffer)) - 1;
    }
    if (buffer[len - 1] != '\n') {
        if (len + 1 < static_cast<int>(sizeof(buffer))) {
            buffer[len++] = '\n';
        }
    }
    crash_write(buffer, static_cast<size_t>(len));
}

// --- Backtrace Functions ---
struct BacktraceState
{
    void** current;
    void** end;
};

static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        }
        *state->current++ = reinterpret_cast<void*>(pc);
    }
    return _URC_NO_REASON;
}

static size_t capture_backtrace(void** buffer, size_t max)
{
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwind_callback, &state);
    return state.current - buffer;
}

static void dump_backtrace(void** buffer, size_t count)
{
    log_line("Backtrace:");
    for (size_t idx = 0; idx < count; ++idx) {
        const void* addr = buffer[idx];
        const char* symbol = "<no_symbol>";
        const char* fname = "<unknown_lib>";
        uintptr_t offset = 0;

        Dl_info info;
        if (dladdr(addr, &info)) {
            if (info.dli_sname) {
                symbol = info.dli_sname;
            }
            if (info.dli_fname) {
                fname = info.dli_fname;
            }
            if (info.dli_fbase) {
                offset = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(info.dli_fbase);
            }
        }

        log_line("  #%02zu pc %zx  %s (%s)", idx, static_cast<size_t>(offset), fname, symbol);
    }
}

static void log_context(void* context)
{
    if (!context) {
        return;
    }

    ucontext_t* uc = reinterpret_cast<ucontext_t*>(context);
#if defined(__aarch64__)
    uintptr_t pc = static_cast<uintptr_t>(uc->uc_mcontext.pc);
    uintptr_t sp = static_cast<uintptr_t>(uc->uc_mcontext.sp);
    uintptr_t lr = static_cast<uintptr_t>(uc->uc_mcontext.regs[30]);
    log_line("CTX: pc=%p lr=%p sp=%p", reinterpret_cast<void*>(pc),
             reinterpret_cast<void*>(lr), reinterpret_cast<void*>(sp));
#elif defined(__arm__)
    uintptr_t pc = static_cast<uintptr_t>(uc->uc_mcontext.arm_pc);
    uintptr_t sp = static_cast<uintptr_t>(uc->uc_mcontext.arm_sp);
    uintptr_t lr = static_cast<uintptr_t>(uc->uc_mcontext.arm_lr);
    log_line("CTX: pc=%p lr=%p sp=%p", reinterpret_cast<void*>(pc),
             reinterpret_cast<void*>(lr), reinterpret_cast<void*>(sp));
#else
    (void)uc;
#endif
}

// --- Signal Handling with Chaining ---
static struct sigaction g_old_sigaction_segv;
static struct sigaction g_old_sigaction_abrt;
static struct sigaction g_old_sigaction_bus;
static struct sigaction g_old_sigaction_ill;
static struct sigaction g_old_sigaction_fpe;

void detailed_signal_handler(int signum, siginfo_t* info, void* context)
{
    log_line("FATAL SIGNAL: %d, si_code: %d, fault addr: %p", signum,
             info ? info->si_code : 0, info ? info->si_addr : nullptr);

    log_context(context);

    void* buffer[32];
    size_t count = capture_backtrace(buffer, 32);
    dump_backtrace(buffer, count);

    // Chain to the previous handler (e.g., Crashlytics)
    struct sigaction* old_action = nullptr;
    switch (signum) {
        case SIGSEGV: old_action = &g_old_sigaction_segv; break;
        case SIGABRT: old_action = &g_old_sigaction_abrt; break;
        case SIGBUS:  old_action = &g_old_sigaction_bus;  break;
        case SIGILL:  old_action = &g_old_sigaction_ill;  break;
        case SIGFPE:  old_action = &g_old_sigaction_fpe;  break;
    }

    if (old_action) {
        if (old_action->sa_flags & SA_SIGINFO) {
            if (old_action->sa_sigaction) {
                old_action->sa_sigaction(signum, info, context);
                return;
            }
        } else {
            if (old_action->sa_handler) {
                old_action->sa_handler(signum);
                return;
            }
        }
    }

    // If no previous handler or it returned, re-raise with default to terminate.
    signal(signum, SIG_DFL);
    raise(signum);
}

void crash_handler_set_storage_path(const char* base_path)
{
    const char* preferred = "/storage/emulated/0/GOLDRP/";
    const char* fallback = (base_path && *base_path) ? base_path : preferred;

    close_crash_fd();
    build_crash_paths(preferred);
    mkdirs(g_crash_dir);
    g_crash_fd = open(g_crash_log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);

    if (g_crash_fd < 0 && fallback != preferred) {
        build_crash_paths(fallback);
        mkdirs(g_crash_dir);
        g_crash_fd = open(g_crash_log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    }
}

void install_signal_handlers()
{
    crash_handler_set_storage_path(nullptr);

    struct sigaction new_action;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_sigaction = detailed_signal_handler;
    new_action.sa_flags = SA_SIGINFO | SA_ONSTACK;

    sigaction(SIGSEGV, &new_action, &g_old_sigaction_segv);
    sigaction(SIGABRT, &new_action, &g_old_sigaction_abrt);
    sigaction(SIGBUS, &new_action, &g_old_sigaction_bus);
    sigaction(SIGILL, &new_action, &g_old_sigaction_ill);
    sigaction(SIGFPE, &new_action, &g_old_sigaction_fpe);
}
