#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>

#define FATAL() { printf("%d: %s\n", __LINE__, strerror(errno)); exit(1); }
#define FATAL_MSG(msg) { printf("%d: %s\n", __LINE__, #msg); exit(1); }

#endif // UTIL_H
