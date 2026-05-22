#pragma once

bool can_request_admin();
void request_admin();
bool is_admin();

#ifdef __linux__
const char *polkit_path();
#endif
