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

#include <stdlib.h>
#include <string.h>

#include <new>
#include <glib.h>

#include "audstrings.h"
#include "runtime.h"
#include "tuple-compiler.h"

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
    Tuple::Field field;

    bool set (const char * name, bool literal);
    bool exists (const Tuple & tuple) const;
    Tuple::ValueType get (const Tuple & tuple, String & tmps, int & tmpi) const;
};

enum class Op {
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

struct TupleCompiler::Node {
    Op op;
    Variable var1, var2;
    Index<Node> children;
};

typedef TupleCompiler::Node Node;

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
            AUDWARN ("Invalid variable '%s'.\n", name);
            return false;
        }
    }

    return true;
}

bool Variable::exists (const Tuple & tuple) const
{
    g_return_val_if_fail (type == Field, false);
    return tuple.get_value_type (field) != Tuple::Empty;
}

Tuple::ValueType Variable::get (const Tuple & tuple, String & tmps, int & tmpi) const
{
    switch (type)
    {
    case Text:
        tmps = text;
        return Tuple::String;

    case Integer:
        tmpi = integer;
        return Tuple::Int;

    case Field:
        switch (tuple.get_value_type (field))
        {
        case Tuple::String:
            tmps = tuple.get_str (field);
            return Tuple::String;

        case Tuple::Int:
            tmpi = tuple.get_int (field);
            return Tuple::Int;

        default:
            return Tuple::Empty;
        }

    default:
        g_return_val_if_reached (Tuple::Empty);
    }
}

TupleCompiler::TupleCompiler () {}
TupleCompiler::~TupleCompiler () {}

static StringBuf get_item (const char * & str, char endch, bool & literal)
{
    const char * s = str;

    StringBuf buf (-1);
    char * set = buf;
    char * stop = buf + buf.len ();

    if (* s == '"')
    {
        if (! literal)
        {
            buf.steal (StringBuf ());
            AUDWARN ("Unexpected string literal at '%s'.\n", s);
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
                buf.steal (StringBuf ());
                AUDWARN ("Unterminated string literal.\n");
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
        buf.steal (StringBuf ());
        AUDWARN ("Expected '%c' at '%s'.\n", endch, s);
        return StringBuf ();
    }

    str = s + 1;

    buf.resize (set - buf);
    return buf;
}

static bool compile_expression (Index<Node> & nodes, const char * & expression);

static bool parse_construct (Node & node, const char * & c)
{
    bool literal1 = true, literal2 = true;

    StringBuf tmps1 = get_item (c, ',', literal1);
    if (! tmps1)
        return false;

    StringBuf tmps2 = get_item (c, ':', literal2);
    if (! tmps2)
        return false;

    if (! node.var1.set (tmps1, literal1))
        return false;

    if (! node.var2.set (tmps2, literal2))
        return false;

    return compile_expression (node.children, c);
}

/* Compile format expression into Node tree. */
static bool compile_expression (Index<Node> & nodes, const char * & expression)
{
    const char * c = expression;

    while (* c && * c != '}')
    {
        Node & node = nodes.append ();

        if (* c == '$')
        {
            /* Expression? */
            if (c[1] != '{')
            {
                AUDWARN ("Expected '${' at '%s'.\n", c);
                return false;
            }

            c += 2;

            switch (* c)
            {
            case '?':
            case '(':
              {
                if (* c == '?')
                {
                    node.op = Op::Exists;
                    c ++;
                }
                else
                {
                    if (strncmp (c, "(empty)?", 8))
                    {
                        AUDWARN ("Expected '(empty)?' at '%s'.\n", c);
                        return false;
                    }

                    node.op = Op::Empty;
                    c += 8;
                }

                bool literal = false;
                StringBuf tmps = get_item (c, ':', literal);
                if (! tmps)
                    return false;

                if (! node.var1.set (tmps, false))
                    return false;

                if (! compile_expression (node.children, c))
                    return false;

                break;
              }

            case '=':
            case '!':
                node.op = (* c == '=') ? Op::Equal : Op::Unequal;

                if (c[1] != '=')
                {
                    AUDWARN ("Expected '%c=' at '%s'.\n", c[0], c);
                    return false;
                }

                c += 2;

                if (! parse_construct (node, c))
                    return false;

                break;

            case '<':
            case '>':
                if (c[1] == '=')
                {
                    node.op = (* c == '<') ? Op::LessEqual : Op::GreaterEqual;
                    c += 2;
                }
                else
                {
                    node.op = (* c == '<') ? Op::Less : Op::Greater;
                    c ++;
                }

                if (! parse_construct (node, c))
                    return false;

                break;

            default:
              {
                bool literal = false;
                StringBuf tmps = get_item (c, '}', literal);
                if (! tmps)
                    return false;

                c --;

                node.op = Op::Var;

                if (! node.var1.set (tmps, false))
                    return false;
              }
            }

            if (* c != '}')
            {
                AUDWARN ("Expected '}' at '%s'.\n", c);
                return false;
            }

            c ++;
        }
        else if (* c == '{')
        {
            AUDWARN ("Unexpected '%c' at '%s'.\n", * c, c);
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
                        buf.steal (StringBuf ());
                        AUDWARN ("Incomplete escaped character.\n");
                        return false;
                    }
                }

                if (set == stop)
                    throw std::bad_alloc ();

                * set ++ = * c ++;
            }

            buf.resize (set - buf);

            node.op = Op::Var;
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
        AUDWARN ("Unexpected '%c' at '%s'.\n", * c, c);
        return false;
    }

    root_nodes = std::move (nodes);
    return true;
}

void TupleCompiler::reset ()
{
    root_nodes.clear ();
}

/* Evaluate tuple in given TupleEval expression in given
 * context and return resulting string. */
static void eval_expression (const Index<Node> & nodes, const Tuple & tuple, StringBuf & out)
{
    for (const Node & node : nodes)
    {
        switch (node.op)
        {
        case Op::Var:
          {
            String tmps;
            int tmpi;

            switch (node.var1.get (tuple, tmps, tmpi))
            {
            case Tuple::String:
                out.insert (-1, tmps);
                break;

            case Tuple::Int:
                out.combine (int_to_str (tmpi));
                break;

            default:
                break;
            }

            break;
          }

        case Op::Equal:
        case Op::Unequal:
        case Op::Less:
        case Op::LessEqual:
        case Op::Greater:
        case Op::GreaterEqual:
          {
            bool result = false;
            String tmps1, tmps2;
            int tmpi1 = 0, tmpi2 = 0;

            Tuple::ValueType type1 = node.var1.get (tuple, tmps1, tmpi1);
            Tuple::ValueType type2 = node.var2.get (tuple, tmps2, tmpi2);

            if (type1 != Tuple::Empty && type2 != Tuple::Empty)
            {
                int resulti;

                if (type1 == type2)
                {
                    if (type1 == Tuple::String)
                        resulti = strcmp (tmps1, tmps2);
                    else
                        resulti = tmpi1 - tmpi2;
                }
                else
                {
                    if (type1 == Tuple::Int)
                        resulti = tmpi1 - atoi (tmps2);
                    else
                        resulti = atoi (tmps1) - tmpi2;
                }

                switch (node.op)
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
          }

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

void TupleCompiler::format (Tuple & tuple) const
{
    tuple.unset (Tuple::FormattedTitle);  // prevent recursion

    StringBuf buf (0);
    eval_expression (root_nodes, tuple, buf);

    if (buf[0])
    {
        tuple.set_str (Tuple::FormattedTitle, buf);
        return;
    }

    /* formatting failed, try fallbacks */
    for (Tuple::Field fallback : {Tuple::Title, Tuple::Basename})
    {
        String title = tuple.get_str (fallback);
        if (title)
        {
            tuple.set_str (Tuple::FormattedTitle, title);
            return;
        }
    }

    tuple.set_str (Tuple::FormattedTitle, "");
}
