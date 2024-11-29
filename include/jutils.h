#ifndef JUTILS_H
#define JUTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define LOG(lvl, fmt, ...) printf("[" lvl "] " fmt "\n", ##__VA_ARGS__)
#define INFO(fmt, ...) LOG("INFO", fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG("WARN", fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG("ERROR", fmt, ##__VA_ARGS__)
#define DEBUG(fmt, ...) LOG("DEBUG", fmt, ##__VA_ARGS__)
#define CHECK(cond) do { if (!(cond)) { LOG("ERROR", "%s", strerror(errno)); exit(EXIT_FAILURE); } } while (0)

#endif // JUTILS_H