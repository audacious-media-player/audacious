/*
 * tuple_compiler.h
 * Copyright (c) 2007 Matti 'ccr' Hämäläinen
 * Copyright (c) 2014 John Lindgren
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

/*
 * the tuple formatter:
 *
 * this is a data-driven meta-language.
 *
 * language constructs follow the following basic rules:
 *   - begin with ${
 *   - end with }
 *
 * language constructs:
 *   - ${field}: prints a field
 *   - ${?field:expr}: evaluates expr if field exists
 *   - ${==field,field:expr}: evaluates expr if both fields are the same
 *   - ${!=field,field:expr}: evaluates expr if both fields are not the same
 *   - ${(empty)?field:expr}: evaluates expr if field does not exist
 *
 * everything else is treated as raw text.
 */

#ifndef LIBAUDCORE_TUPLE_COMPILER_H
#define LIBAUDCORE_TUPLE_COMPILER_H

#include <libaudcore/index.h>
#include <libaudcore/tuple.h>

class TupleCompiler
{
public:
    struct Node;

    TupleCompiler ();
    ~TupleCompiler ();

    bool compile (const char * expr);
    void reset ();

    void format (Tuple & tuple) const;

private:
    Index<Node> root_nodes;
};

#endif /* LIBAUDCORE_TUPLE_COMPILER_H */
