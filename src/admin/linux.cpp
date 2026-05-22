#ifdef __linux__
#include "admin.hpp"
#include <unistd.h>

bool can_request_admin() { return false; };
void request_admin() {};
bool is_admin() { return (getuid() == 0); };

#endif
