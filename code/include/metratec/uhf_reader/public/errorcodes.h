/**
 * @file: errorcodes.h                                                                             *
 * Project: metratec-uhf-sdk-c                                                                     *
 * Created Date: 2025-05-05                                                                        *
 * Author: Martin Koehler                                                                          *
 * -----                                                                                           *
 * Copyright (C) 2025                                                                              *
 * Metratec GmbH - All Rights Reserved                                                             *
 * Unauthorized copying of this file, via any medium, is strictly prohibited.                      *
 * Proprietary and confidential                                                                    *
 * 
 * @brief Declaration of errorcodes in case the system built on lacks them
 */

#pragma once

#ifndef EAGAIN
#define EAGAIN 11
#endif

#ifndef ENOMEM
#define ENOMEM 12
#endif

#ifndef EFAULT
#define EFAULT 14
#endif

#ifndef EINVAL
#define EINVAL 22
#endif

#ifndef EBADSLT
#define EBADSLT 57
#endif

#ifndef ENODATA
#define ENODATA 61
#endif

#ifndef EBADMSG
#define EBADMSG 74
#endif

#ifndef ENOBUFS
#define ENOBUFS 105
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT 110
#endif

#ifndef EALREADY
#define EALREADY 114
#endif

#ifndef ENAVAIL
#define ENAVAIL 119
#endif