// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_MEANING_H
#define CURV_MEANING_H

#include <vector>
#include <aux/tail_array.h>
#include <curv/shared.h>
#include <curv/phrase.h>
#include <curv/value.h>
#include <curv/atom.h>
#include <curv/list.h>
#include <curv/module.h>
#include <curv/frame.h>

namespace curv {

/// An abstract base class representing a semantically analyzed Phrase.
///
/// TODO: class Meaning doesn't currently pull its weight. It's an abstract
/// class and the only subclass is Expression. I originally planned to have a
/// Definition subclass of Meaning, but definition abstractions now work
/// differently.
struct Meaning : public aux::Shared_Base
{
    /// The original source code for this meaning.
    ///
    /// The syntax of the source code need not have any relation to the meaning
    /// class. Eg, an Identifier phrase can be analyzed into a variety of
    /// different meanings. That's why we separate the Phrase tree from the
    /// Meaning tree.
    Shared<const Phrase> source_;

    Meaning(Shared<const Phrase> source) : source_(std::move(source)) {}
};

/// An Expression is a phrase that (a) can be evaluated to produce a single 
/// value, or (b) can be executed to generate a sequence of zero or more values
/// within a list comprehension. Note that (a) implies (b).
struct Expression : public Meaning
{
    using Meaning::Meaning;

    virtual Value eval(Frame&) const = 0;
    virtual void generate(Frame&, List_Builder&) const;
};

/// A Constant is an Expression whose value is known at compile time.
struct Constant : public Expression
{
    Value value_;

    Constant(Shared<const Phrase> source, Value v)
    : Expression(std::move(source)), value_(std::move(v))
    {}

    virtual Value eval(Frame&) const override;
};

struct Module_Ref : public Expression
{
    size_t slot_;

    Module_Ref(Shared<const Phrase> source, size_t slot)
    : Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Nonlocal_Ref : public Expression
{
    size_t slot_;

    Nonlocal_Ref(Shared<const Phrase> source, size_t slot)
    : Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Let_Ref : public Expression
{
    int slot_;

    Let_Ref(Shared<const Phrase> source, int slot)
    : Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Arg_Ref : public Expression
{
    int slot_;

    Arg_Ref(Shared<const Phrase> source, int slot)
    : Expression(std::move(source)), slot_(slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Local_Function_Ref : public Expression
{
    int lambda_slot_; ///! local slot containing Lambda value
    int env_slot_;    ///! local slot containing List of nonlocal values

    Local_Function_Ref(
        Shared<const Phrase> source,
        int lambda_slot,
        int env_slot)
    :
        Expression(std::move(source)),
        lambda_slot_(lambda_slot),
        env_slot_(env_slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Nonlocal_Function_Ref : public Expression
{
    int lambda_slot_; ///! nonlocal slot containing Lambda value

    Nonlocal_Function_Ref(
        Shared<const Phrase> source,
        int lambda_slot)
    :
        Expression(std::move(source)),
        lambda_slot_(lambda_slot)
    {}

    virtual Value eval(Frame&) const override;
};

struct Dot_Expr : public Expression
{
    Shared<Expression> base_;
    Atom id_;

    Dot_Expr(Shared<const Phrase> source, Shared<Expression> base, Atom id)
    :
        Expression(std::move(source)),
        base_(std::move(base)),
        id_(std::move(id))
    {}

    virtual Value eval(Frame&) const override;
};

struct Call_Expr : public Expression
{
    Shared<Expression> fun_;
    Shared<const Phrase> argsource_;
    std::vector<Shared<Expression>> args_;

    Call_Expr(
        Shared<const Phrase> source,
        Shared<Expression> fun,
        Shared<const Phrase> argsource,
        std::vector<Shared<Expression>> args)
    :
        Expression(std::move(source)),
        fun_(std::move(fun)),
        argsource_(std::move(argsource)),
        args_(std::move(args))
    {}

    virtual Value eval(Frame&) const override;
};

struct Prefix_Expr : public Expression
{
    Token::Kind op_;
    Shared<Expression> arg_;

    Prefix_Expr(
        Shared<const Phrase> source,
        Token::Kind op,
        Shared<Expression> arg)
    :
        Expression(source),
        op_(op),
        arg_(std::move(arg))
    {}

    virtual Value eval(Frame&) const override;
};
struct Prefix_Expr_Base : public Expression
{
    Shared<Expression> arg_;

    Prefix_Expr_Base(
        Shared<const Phrase> source,
        Shared<Expression> arg)
    :
        Expression(source),
        arg_(std::move(arg))
    {}
};
struct Not_Expr : public Prefix_Expr_Base
{
    using Prefix_Expr_Base::Prefix_Expr_Base;
    virtual Value eval(Frame&) const override;
};

struct Infix_Expr : public Expression
{
    Token::Kind op_;
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;

    Infix_Expr(
        Shared<const Phrase> source,
        Token::Kind op,
        Shared<Expression> arg1,
        Shared<Expression> arg2)
    :
        Expression(source),
        op_(op),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}

    virtual Value eval(Frame&) const override;
};
struct Infix_Expr_Base : public Expression
{
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;

    Infix_Expr_Base(
        Shared<const Phrase> source,
        Shared<Expression> arg1,
        Shared<Expression> arg2)
    :
        Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}
};
struct Or_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct And_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Not_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Less_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Greater_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Less_Or_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Greater_Or_Equal_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct Power_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};
struct At_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame&) const override;
};

struct List_Expr_Base : public Expression,
    public aux::Tail_Array_Data<Shared<const Expression>>
{
    List_Expr_Base(Shared<const Phrase> source)
    : Expression(std::move(source)) {}

    virtual Value eval(Frame&) const override;
    Shared<List> eval_list(Frame&) const;
};
using List_Expr = aux::Tail_Array<List_Expr_Base>;

struct Record_Expr : public Expression
{
    Atom_Map<Shared<const Expression>> fields_;

    Record_Expr(Shared<const Phrase> source) : Expression(source) {}

    virtual Value eval(Frame&) const override;
};

struct Module_Expr : public Expression
{
    Shared<Module::Dictionary> dictionary_;
    Shared<List> slots_; // or, a Tail_Array
    Shared<const List_Expr> elements_;
    size_t frame_nslots_;

    Module_Expr(Shared<const Phrase> source) : Expression(source) {}

    virtual Value eval(Frame&) const override;
    Shared<Module> eval_module(Frame*) const;
};

struct Let_Expr : public Expression
{
    size_t first_slot_;
    std::vector<Value> values_; // or, a Tail_Array
    Shared<const Expression> body_;

    Let_Expr(
        Shared<const Phrase> source,
        size_t first_slot,
        std::vector<Value> values,
        Shared<const Expression> body)
    :
        Expression(std::move(source)),
        first_slot_(first_slot),
        values_(std::move(values)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
};

struct For_Expr : public Expression
{
    size_t slot_;
    Shared<const Expression> list_;
    Shared<const Expression> body_;

    For_Expr(
        Shared<const Phrase> source,
        size_t slot,
        Shared<const Expression> list,
        Shared<const Expression> body)
    :
        Expression(std::move(source)),
        slot_(slot),
        list_(std::move(list)),
        body_(std::move(body))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
};

struct If_Expr : public Expression
{
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;

    If_Expr(
        Shared<const Phrase> source,
        Shared<Expression> arg1,
        Shared<Expression> arg2)
    :
        Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
};

struct If_Else_Expr : public Expression
{
    Shared<Expression> arg1_;
    Shared<Expression> arg2_;
    Shared<Expression> arg3_;

    If_Else_Expr(
        Shared<const Phrase> source,
        Shared<Expression> arg1,
        Shared<Expression> arg2,
        Shared<Expression> arg3)
    :
        Expression(source),
        arg1_(std::move(arg1)),
        arg2_(std::move(arg2)),
        arg3_(std::move(arg3))
    {}

    virtual Value eval(Frame&) const override;
    virtual void generate(Frame&, List_Builder&) const override;
};

struct Lambda_Expr : public Expression
{
    Shared<Expression> body_;
    Shared<List_Expr> nonlocals_;
    size_t nargs_;
    size_t nslots_;

    Lambda_Expr(
        Shared<const Phrase> source,
        Shared<Expression> body,
        Shared<List_Expr> nonlocals,
        size_t nargs,
        size_t nslots)
    :
        Expression(source),
        body_(std::move(body)),
        nonlocals_(std::move(nonlocals)),
        nargs_(nargs),
        nslots_(nslots)
    {}

    virtual Value eval(Frame&) const override;
};

} // namespace curv
#endif // header guard
