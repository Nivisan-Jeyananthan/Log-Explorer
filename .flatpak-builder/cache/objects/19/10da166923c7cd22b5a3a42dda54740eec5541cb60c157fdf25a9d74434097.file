#pragma once

#include "db.h"

// Start the indexer. This will spawn background threads to:
//  - read systemd journal (if available) using `journalctl -o json -f --since ...`
//  - scan and tail files under /var/log
// Returns 0 on success (threads started) or -1 on failure to start.
int indexer_start(DB *db);
// Stop the indexer and wait for background threads to finish. Returns 0 on success.
int indexer_stop(void);

