//
// Created by rainyx on 16/8/23.
// Copyright (c) 2016 rainyx. All rights reserved.
//

#ifndef RXMEMSCAN_PREFIX_H
#define RXMEMSCAN_PREFIX_H

//#include <arm/types.h/*>*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RX_NAME "ahengmemsearch"
#define RX_VERSION "1.0.0(Beta)"
#define RX_COPYRIGHT "joeyfrancistribbiani@outlook.com"

#define _rxmemcpy(d, s, c) (memcpy((d), (s), (c)))

//#define RX_DEBUG
#ifdef RX_DEBUG
#define RX_MEM_DEBUG
#endif

#define _1MB (1024 * 1024)
#define _10MB (_1MB * 10)
#define _20MB (_1MB * 20)
#define _30MB (_1MB * 30)
#define _50MB (_1MB * 50)
#define _100MB (_1MB * 100)

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif //RXMEMSCAN_PREFIX_H
