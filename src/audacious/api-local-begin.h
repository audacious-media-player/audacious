/*
 * api-local-begin.h
 * Copyright 2010-2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#if ! defined AUD_API_NAME || ! defined AUD_API_SYMBOL || defined AUD_API_LOCAL_H
#error Bad usage of api-local-begin.h
#endif

#define AUD_API_LOCAL_H

#define AUD_FUNC0(t,n) t n (void);
#define AUD_FUNC1(t,n,t1,n1) t n (t1 n1);
#define AUD_FUNC2(t,n,t1,n1,t2,n2) t n (t1 n1, t2 n2);
#define AUD_FUNC3(t,n,t1,n1,t2,n2,t3,n3) t n (t1 n1, t2 n2, t3 n3);
#define AUD_FUNC4(t,n,t1,n1,t2,n2,t3,n3,t4,n4) t n (t1 n1, t2 n2, t3 n3, t4 n4);
#define AUD_FUNC5(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5) t n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5);
#define AUD_FUNC6(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6) t n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6);
#define AUD_FUNC7(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7) t n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7);
#define AUD_FUNC8(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7,t8,n8) t n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8);

#define AUD_VFUNC0(n) void n (void);
#define AUD_VFUNC1(n,t1,n1) void n (t1 n1);
#define AUD_VFUNC2(n,t1,n1,t2,n2) void n (t1 n1, t2 n2);
#define AUD_VFUNC3(n,t1,n1,t2,n2,t3,n3) void n (t1 n1, t2 n2, t3 n3);
#define AUD_VFUNC4(n,t1,n1,t2,n2,t3,n3,t4,n4) void n (t1 n1, t2 n2, t3 n3, t4 n4);
#define AUD_VFUNC5(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5) void n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5);
#define AUD_VFUNC6(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6) void n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6);
#define AUD_VFUNC7(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7) void n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7);
#define AUD_VFUNC8(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7,t8,n8) void n (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8);
