#pragma once

static_assert((strlen(HOSTNAME) == 0) || ((strlen(HOSTNAME) > 1) && (strlen(HOSTNAME) < 31)), "HOSTNAME must be 1..31 characters");

#if USE_PASSWORD
    static_assert(((strlen(ADMIN_PASS) >= 8) && (strlen(ADMIN_PASS) <= 63)), "ADMIN_PASS must be 8..63 characters");
#endif

