#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Handle
typedef struct SB_Handle SB_Handle;

// Key/Value Paar für sb_values
typedef struct {
    char* name;
    int   value;
} sb_kv_t;

// Module-Infos für sb_modules_info
typedef struct {
    char*  module_id;
    size_t ncontrols;
    char** controls;
} sb_module_info_t;

// Events
typedef void(*sb_event_cb_t)(const char* name, int value, void* user);


// --- Konstruktion / Destruktion ---
SB_Handle* sb_new(const char* device, unsigned baud);
void       sb_delete(SB_Handle* h);

// --- Status / Sync ---
bool sb_refresh(SB_Handle* h, unsigned timeout_ms);
bool sb_refresh_modules(SB_Handle* h, unsigned timeout_ms);

// --- Einzelwerte / Snapshots ---
int   sb_value(SB_Handle* h, const char* name);
bool  sb_try_value(SB_Handle* h, const char* name, int* out);
sb_kv_t* sb_values(SB_Handle* h, const char* pattern, size_t* count);

char* sb_snapshot_json(SB_Handle* h, bool sorted);
char* sb_controls_json(SB_Handle* h, bool sorted);

// --- Namen / Listen ---
char** sb_names(SB_Handle* h, size_t* count);
char** sb_module_ids(SB_Handle* h, size_t* count);
sb_module_info_t* sb_modules_info(SB_Handle* h, size_t* count);
char** sb_module_list_raw(SB_Handle* h, size_t* count);

// --- Events ---
unsigned long long sb_subscribe(SB_Handle* h, const char* pattern,
                                sb_event_cb_t cb, void* user);
void sb_unsubscribe(SB_Handle* h, unsigned long long id);

// --- Fehler & Speicher ---
const char* sb_last_error(void);
void sb_free_string(char* s);
void sb_free_kv_array(sb_kv_t* arr, size_t count);
void sb_free_str_array(char** arr, size_t count);
void sb_free_modules_info(sb_module_info_t* arr, size_t count);

#ifdef __cplusplus
}
#endif
