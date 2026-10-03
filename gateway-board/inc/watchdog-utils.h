#ifndef __WATCHDOG_UTILS_H
#define __WATCHDOG_UTILS_H

// First point of interaction with this module,
// called once to setup watchdog
int setup_watchdog();

// Call periodically to prevent board reset
int feed_watchdog();

#endif /* __WATCHDOG_UTILS_H */
