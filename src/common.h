#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#include "cmd_parser.h"
#include "file.h"
#include "key.h"
#include "network.h"
#include "logger.h"
#include "dht.h"
#include "query.h"

#define BOOTSTRAP_SERVER_IP     inet_addr("127.0.0.1")
#define BOOTSTRAP_SERVER_PORT   12345

#endif