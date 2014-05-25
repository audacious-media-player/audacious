/*
 * tuple_compiler.c
 * Copyright (c) 2007 Matti 'ccr' Hämäläinen
 * Copyright (c) 2011-2014 John Lindgren
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <new>
#include <glib.h>

#include "audstrings.h"
#include "tuple_compiler.h"

#define MAX_VARS    4

#define tuple_error(...) fprintf (stderr, "Tuple compiler: " __VA_ARGS__)

struct Variable
{
    enum {
        Invalid = 0,
        Text,
        Integer,
        Field
    } type;

    String text;
    int integer;
    int field;

    bool set (const char * name, bool literal);
    bool exists (const Tuple & tuple) const;
    TupleValueType get (const Tuple & tuple, String & tmps, int & tmpi) const;
};

enum class TupleCompiler::Op {
    Invalid = 0,
    Var,
    Exists,
    Equal,
    Unequal,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Empty
};

struct TupleCompiler::Node
{
    Op opcode;               /* operator */
    Variable var1, var2;     /* variables */
    Index<Node> children;    /* children of this node */
};

bool Variable::set (const char * name, bool literal)
{
    if (g_ascii_isdigit (name[0]))
    {
        type = Integer;
        integer = atoi (name);
    }
    else if (literal)
    {
        type = Text;
        text = String (name);
    }
    else
    {
        type = Field;
        field = Tuple::field_by_name (name);

        if (field < 0)
        {
            tuple_error ("Invalid variable '%s'.\n", name);
            return false;
        }
    }

    return true;
}

bool Variable::exists (const Tuple & tuple) const
{
    g_return_val_if_fail (type == Field, false);
    return tuple.get_value_type (field) != TUPLE_UNKNOWN;
}

TupleValueType Variable::get (const Tuple & tuple, String & tmps, int & tmpi) const
{
    switch (type)
    {
    case Text:
        tmps = text;
        return TUPLE_STRING;

    case Integer:
        tmpi = integer;
        return TUPLE_INT;

    case Field:
        switch (tuple.get_value_type (field))
        {
        case TUPLE_STRING:
            tmps = tuple.get_str (field);
            return TUPLE_STRING;

        case TUPLE_INT:
            tmpi = tuple.get_int (field);
            return TUPLE_INT;

        default:
            return TUPLE_UNKNOWN;
        }

    default:
        g_return_val_if_reached (TUPLE_UNKNOWN);
    }
}

TupleCompiler::TupleCompiler () {}
TupleCompiler::~TupleCompiler () {}

static StringBuf get_item (const char * & str, char endch, bool & literal, const char * item)
{
    const char * s = str;

    StringBuf buf (-1);
    char * set = buf;
    char * stop = buf + buf.len ();

    if (* s == '"')
    {
        if (! literal)
        {
            tuple_error ("Unexpected string literal at '%s'.\n", s);
            return StringBuf ();
        }

        s ++;
    }
    else
        literal = false;

    if (literal)
    {
        while (* s != '"')
        {
            if (* s == '\\')
                s ++;

            if (! * s)
            {
                tuple_error ("Unterminated string literal.\n");
                return StringBuf ();
            }

            if (set == stop)
                throw std::bad_alloc ();

            * set ++ = * s ++;
        }

        s ++;
    }
    else
    {
        while (g_ascii_isalnum (* s) || * s == '-')
        {
            if (set == stop)
                throw std::bad_alloc ();

            * set ++ = * s ++;
        }
    }

    if (* s != endch)
    {
        tuple_error ("Expected '%c' at '%s'.\n", endch, s);
        return StringBuf ();
    }

    str = s + 1;

    buf.resize (set - buf);
    return buf;
}

bool TupleCompiler::parse_construct (Node & node, const char * item, const char * & c, Op opcode)
{
    bool literal1 = true, literal2 = true;

    StringBuf tmps1 = get_item (c, ',', literal1, item);
    if (! tmps1)
        return false;

    StringBuf tmps2 = get_item (c, ':', literal2, item);
    if (! tmps2)
        return false;

    node.opcode = opcode;

    if (! node.var1.set (tmps1, literal1))
        return false;

    if (! node.var2.set (tmps2, literal2))
        return false;

    return compile_expression (node.children, c);
}

/* Compile format expression into Node tree. */
bool TupleCompiler::compile_expression (Index<Node> & nodes, const char * & expression)
{
    const char * c = expression, * item;
    bool literal;

    while (* c && * c != '}')
    {
        if (* c == '$')
        {
            /* Expression? */
            item = c ++;

            if (* c != '{')
            {
                tuple_error ("Expected '${' at '%s'.\n", c - 1);
                return false;
            }

            Op opcode;
            c ++;

            switch (* c)
            {
            case '?':
            {
                c ++;
                /* Exists? */
                literal = false;

                StringBuf tmps = get_item (c, ':', literal, item);
                if (! tmps)
                    return false;

                Node & node = nodes.append ();
                node.opcode = Op::Exists;

                if (! node.var1.set (tmps, false))
                    return false;

                if (! compile_expression (node.children, c))
                    return false;

                break;
            }
            case '=':
                c ++;

                if (* c != '=')
                {
                    tuple_error ("Expected '==' at '%s'.\n", c - 1);
                    return false;
                }

                c ++;

                /* Equals? */
                if (! parse_construct (nodes.append (), item, c, Op::Equal))
                    return false;

                break;

            case '!':
                c ++;

                if (* c != '=')
                {
                    tuple_error ("Expected '!=' at '%s'.\n", c - 1);
                    return false;
                }

                c ++;

                if (! parse_construct (nodes.append (), item, c, Op::Unequal))
                    return false;

                break;

            case '<':
                c ++;

                if (* c == '=')
                {
                    opcode = Op::LessEqual;
                    c ++;
                }
                else
                    opcode = Op::Less;

                if (! parse_construct (nodes.append (), item, c, opcode))
                    return false;

                break;

            case '>':
                c ++;

                if (* c == '=')
                {
                    opcode = Op::GreaterEqual;
                    c ++;
                }
                else
                    opcode = Op::Greater;

                if (! parse_construct (nodes.append (), item, c, opcode))
                    return false;

                break;

            case '(':
            {
                c ++;

                if (strncmp (c, "empty)?", 7))
                {
                    tuple_error ("Expected '(empty)?' at '%s'.\n", c - 1);
                    return false;
                }

                c += 7;
                literal = false;

                StringBuf tmps = get_item (c, ':', literal, item);
                if (! tmps)
                    return false;

                Node & node = nodes.append ();
                node.opcode = Op::Empty;

                if (! node.var1.set (tmps, false))
                    return false;

                if (! compile_expression (node.children, c))
                    return false;

                break;
            }
            default:
                /* Get expression content */
                literal = false;

                StringBuf tmps = get_item (c, '}', literal, item);
                if (! tmps)
                    return false;

                c --;

                /* I HAS A FIELD - A field. You has it. */
                Node & node = nodes.append ();
                node.opcode = Op::Var;

                if (! node.var1.set (tmps, false))
                    return false;
            }

            if (* c != '}')
            {
                tuple_error ("Expected '}' at '%s'.\n", c);
                return false;
            }

            c ++;
        }
        else if (* c == '{')
        {
            tuple_error ("Unexpected '%c' at '%s'.\n", * c, c);
            return false;
        }
        else
        {
            /* Parse raw/literal text */
            StringBuf buf (-1);
            char * set = buf;
            char * stop = buf + buf.len ();

            while (* c && * c != '$' && * c != '{' && * c != '}')
            {
                if (* c == '\\')
                {
                    c ++;

                    if (! * c)
                    {
                        tuple_error ("Incomplete escaped character.\n");
                        return false;
                    }
                }

                if (set == stop)
                    throw std::bad_alloc ();

                * set ++ = * c ++;
            }

            buf.resize (set - buf);

            Node & node = nodes.append ();
            node.opcode = Op::Var;
            node.var1.type = Variable::Text;
            node.var1.text = String (buf);
        }
    }

    expression = c;
    return true;
}

bool TupleCompiler::compile (const char * expr)
{
    const char * c = expr;
    Index<Node> nodes;

    if (! compile_expression (nodes, c))
        return false;

    if (* c)
    {
        tuple_error ("Unexpected '%c' at '%s'.\n", * c, c);
        return false;
    }

    root_nodes = std::move (nodes);
    return true;
}

/* Evaluate tuple in given TupleEval expression in given
 * context and return resulting string. */
void TupleCompiler::eval_expression (const Index<Node> & nodes,
 const Tuple & tuple, StringBuf & out) const
{
    TupleValueType type0, type1;
    String tmps0, tmps1;
    int tmpi0 = 0, tmpi1 = 0;
    bool result;
    int resulti;

    for (const Node & node : nodes)
    {
        switch (node.opcode)
        {
        case Op::Var:
            switch (node.var1.get (tuple, tmps0, tmpi0))
            {
            case TUPLE_STRING:
                str_insert (out, -1, tmps0);
                break;

            case TUPLE_INT:
                out.combine (int_to_str (tmpi0));
                break;

            default:
                break;
            }

            break;

        case Op::Equal:
        case Op::Unequal:
        case Op::Less:
        case Op::LessEqual:
        case Op::Greater:
        case Op::GreaterEqual:
            type0 = node.var1.get (tuple, tmps0, tmpi0);
            type1 = node.var2.get (tuple, tmps1, tmpi1);
            result = false;

            if (type0 != TUPLE_UNKNOWN && type1 != TUPLE_UNKNOWN)
            {
                if (type0 == type1)
                {
                    if (type0 == TUPLE_STRING)
                        resulti = strcmp (tmps0, tmps1);
                    else
                        resulti = tmpi0 - tmpi1;
                }
                else
                {
                    if (type0 == TUPLE_INT)
                        resulti = tmpi0 - atoi (tmps1);
                    else
                        resulti = atoi (tmps0) - tmpi1;
                }

                switch (node.opcode)
                {
                case Op::Equal:
                    result = (resulti == 0);
                    break;

                case Op::Unequal:
                    result = (resulti != 0);
                    break;

                case Op::Less:
                    result = (resulti < 0);
                    break;

                case Op::LessEqual:
                    result = (resulti <= 0);
                    break;

                case Op::Greater:
                    result = (resulti > 0);
                    break;

                case Op::GreaterEqual:
                    result = (resulti >= 0);
                    break;

                default:
                    g_warn_if_reached ();
                }
            }

            if (result)
                eval_expression (node.children, tuple, out);

            break;

        case Op::Exists:
            if (node.var1.exists (tuple))
                eval_expression (node.children, tuple, out);

            break;

        case Op::Empty:
            if (! node.var1.exists (tuple))
                eval_expression (node.children, tuple, out);

            break;

        default:
            g_warn_if_reached ();
        }
    }
}

StringBuf TupleCompiler::evaluate (const Tuple & tuple) const
{
    StringBuf buf (0);
    eval_expression (root_nodes, tuple, buf);
    return buf;
}
