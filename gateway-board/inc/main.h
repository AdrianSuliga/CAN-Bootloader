#ifndef __MAIN_H
#define __MAIN_H

// Reset board if watchdog is not fed for 3 minutes
#define WDT_TIMEOUT_MS (3 * 60 * 1000)

// Wait for 1s before retrying
#define WAIT_ON_ERROR_TIME 1

#endif /* __MAIN_H */
