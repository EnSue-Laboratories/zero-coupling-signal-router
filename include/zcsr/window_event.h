#ifndef ZCSR_WINDOW_EVENT_H
#define ZCSR_WINDOW_EVENT_H
/* Agent 1 (Game Core) — window-level events (close request) + clean shutdown.
 * Ties to the platform surface. Implementer: modules/input (or platform). */
#include "platform.h"
#include <stdbool.h>

/* True if the user requested to close the window since the last poll (consumes the flag). */
bool zcsr_window_close_requested(zcsr_surface*);

#endif /* ZCSR_WINDOW_EVENT_H */
