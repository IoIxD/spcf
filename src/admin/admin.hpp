#pragma once

/**
 * Returns whether this a system where we can request admin
 * (i.e. via UAC prompt on Windows).
 * Used to determine whether we show the button to do so.
 * */
bool can_request_admin();
/**
 * Try to request admin.
 */
void request_admin();
/** Whether we are admin. */
bool is_admin();

#ifdef __linux__
/** Returns the path to the pkexec executable on Linux, if the user has it
 * installed. NULL otherwise. */
const char *polkit_path();
#endif
