#ifndef HW_KEY_H
#define HW_KEY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { KEY_EVT_PRESS = 1, KEY_EVT_RELEASE = 2 } key_event_t;
typedef void (*key_callback_t)(int gpio, key_event_t evt, void* arg);

/**
 * Initialize key module (creates queue and task, installs ISR service if needed).
 * Returns true on success.
 */
bool hw_key_init(void);

/**
 * Add a key by GPIO number and its active level (e.g., 1 for active-high).
 * This sets interrupt type to ANYEDGE and installs the ISR for this GPIO.
 */
bool hw_key_add(int gpio, int active_level);

/**
 * Register a callback for a specific GPIO key. Callback receives gpio, event and user arg.
 */
bool hw_key_register_callback(int gpio, key_callback_t cb, void* arg);

#ifdef __cplusplus
}
#endif

#endif // HW_KEY_H
