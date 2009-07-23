/* $Id: .indent.pro 238 2005-09-21 05:26:03Z nenolod $ */

/* copy this file to the source dir then run indent file.c */

--gnu-style

/* This is the indent before the brace not inside the block. */
--brace-indent0

/* Indent case: by 2 and braces inside case by 0(then by 0)... */
--case-brace-indentation0
--case-indentation2

--indent-level4

/* Put while() on the brace from do... */
--cuddle-do-while

/* Disable an annoying format... */
--no-space-after-function-call-names

/* Disable an annoying format... */
--dont-break-procedure-type

/* Disable an annoying format... */
--no-space-after-casts

--line-length200

--no-tabs
