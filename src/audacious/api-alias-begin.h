/*
 * api-alias-begin.h
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#if ! defined AUD_API_NAME || ! defined AUD_API_SYMBOL || defined AUD_API_ALIAS
#error Bad usage of api-alias-begin.h
#endif

#define AUD_API_ALIAS

extern AudAPITable * _aud_api_table;

#define AUD_FUNC0(t,n) static inline t aud_##n(void) {return _aud_api_table->AUD_API_SYMBOL->n();}
#define AUD_FUNC1(t,n,t1,n1) static inline t aud_##n(t1 n1) {return _aud_api_table->AUD_API_SYMBOL->n(n1);}
#define AUD_FUNC2(t,n,t1,n1,t2,n2) static inline t aud_##n(t1 n1, t2 n2) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2);}
#define AUD_FUNC3(t,n,t1,n1,t2,n2,t3,n3) static inline t aud_##n(t1 n1, t2 n2, t3 n3) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3);}
#define AUD_FUNC4(t,n,t1,n1,t2,n2,t3,n3,t4,n4) static inline t aud_##n(t1 n1, t2 n2, t3 n3, t4 n4) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4);}
#define AUD_FUNC5(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5) static inline t aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5);}
#define AUD_FUNC6(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6) static inline t aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5,n6);}
#define AUD_FUNC7(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7) static inline t aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5,n6,n7);}
#define AUD_FUNC8(t,n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7,t8,n8) static inline t aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8) {return _aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5,n6,n7,n8);}

#define AUD_VFUNC0(n) static inline void aud_##n(void) {_aud_api_table->AUD_API_SYMBOL->n();}
#define AUD_VFUNC1(n,t1,n1) static inline void aud_##n(t1 n1) {_aud_api_table->AUD_API_SYMBOL->n(n1);}
#define AUD_VFUNC2(n,t1,n1,t2,n2) static inline void aud_##n(t1 n1, t2 n2) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2);}
#define AUD_VFUNC3(n,t1,n1,t2,n2,t3,n3) static inline void aud_##n(t1 n1, t2 n2, t3 n3) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3);}
#define AUD_VFUNC4(n,t1,n1,t2,n2,t3,n3,t4,n4) static inline void aud_##n(t1 n1, t2 n2, t3 n3, t4 n4) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4);}
#define AUD_VFUNC5(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5) static inline void aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5);}
#define AUD_VFUNC6(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6) static inline void aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5,n6);}
#define AUD_VFUNC7(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7) static inline void aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5,n6,n7);}
#define AUD_VFUNC8(n,t1,n1,t2,n2,t3,n3,t4,n4,t5,n5,t6,n6,t7,n7,t8,n8) static inline void aud_##n(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8) {_aud_api_table->AUD_API_SYMBOL->n(n1,n2,n3,n4,n5,n6,n7,n8);}
