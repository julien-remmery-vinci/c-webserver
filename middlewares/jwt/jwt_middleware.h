#ifndef JWT_MIDDLEWARE_H
#define JWT_MIDDLEWARE_H

#include "server.h"
#include "http.h"

int
authorize(Route* route, Http_Request* req, Http_Response* res);

#endif JWT_MIDDLEWARE_H