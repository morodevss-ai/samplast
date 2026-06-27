#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

void install_signal_handlers();
void crash_handler_set_storage_path(const char* base_path);
void crash_handler_logf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // CRASH_HANDLER_H
