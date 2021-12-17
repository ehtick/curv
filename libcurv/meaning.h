// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_MEANING_H
#define LIBCURV_MEANING_H

#include <libcurv/frame.h>
#include <libcurv/phrase.h>
#include <libcurv/sc_frame.h>
#include <libcurv/symbol.h>
#include <libcurv/value.h>

namespace curv {

struct Operation;

// An abstract base class representing a semantically analysed Phrase.
// Currently, a Meaning is a Metafunction or an Operation.
//
// PROPOSAL: Convert Metafunction to a value type, replacing Meaning
// with Operation.
// * A Metafunction value can be used as the left argument of a function call,
//   only if the Metafunction is a compile time constant. This will become
//   useful once we have compile time constant folding and partial evaluation
//   (ie, abstraction over constants).
//
// PROPOSAL: Make Curv homoiconic, so that Operations are values.
// A Metafunction becomes a tagged record value, and there are user defined
// metafunctions.
struct Meaning : public Shared_Base
{
    // The original syntax tree for this meaning.
    //
    // The syntax need not have any relation to the meaning class.
    // Eg, an Identifier phrase can be analysed into a variety of different
    // meanings. That's why we separate the Phrase tree from the Meaning tree.
    //
    // If the Meaning is algorithmically generated by native code, and not
    // constructed by analysing a syntax tree, then the `syntax_` might be
    // a Call_Phrase for whatever Curv function call generated the Meaning.
    // So it can be quite arbitrary, and you can't assume that a particular
    // Meaning class is paired with a particular Phrase class.
    Shared<const Phrase> syntax_;

    Meaning(Shared<const Phrase> syntax) : syntax_(std::move(syntax)) {}

    // These functions are called during semantic analysis.
    virtual Shared<Operation> to_operation(Source_State&) = 0;
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&);
};

/// A Metafunction is a function that is called during analysis, instead of
/// at run time.
///
/// A call to a Metafunction is compiled to a Meaning using `call`.
/// Metafunctions enable the client to add new syntax to the language
/// without modifying the grammar or modifying the Curv library,
/// as long as that new syntax parses as a function call.
///
/// Metafunctions are not values, & Metafunction is not a subclass of Operation.
/// Metafunctions are similar to the macros of the Scheme and Rust languages,
/// but we currently have no plan to support user defined metafunctions.
struct Metafunction : public Meaning
{
    using Meaning::Meaning;
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&) override = 0;
    virtual Shared<Operation> to_operation(Source_State&) override;
    virtual void print_help(std::ostream&) const = 0;
};

/// An Operation is a fragment of compiled code that "does something" at run
/// time. During analysis, a syntax tree is compiled into an Operation tree.
///
/// At present, the Operation tree has two roles. It is our "IR" (Intermediate
/// Representation) to which optimizations are applied, and it is also our
/// executable format. In the future, we should separate these roles, add a
/// separate code generation phase, and use a more efficient executable code
/// representation.
///
/// Kinds of Operations:
/// * An Expression is evaluated to return a single value using `eval`.
///   Every expression is also a value generator that produces 1 value.
///   For example, `2+2`.
/// * A Statement is executed for its side effects using `exec`.
///   There are 3 kinds of statement, and the Executor argument of `exec`
///   determines which kinds are legal in a given execution context.
///   * A Value Generator is executed to produce a sequence of zero
///     or more values, calling `executor.push_value` for each value.
///     For example, `for (i in 1..10) i^2`.
///   * A Field Generator is executed to produce a sequence of zero or
///     more record fields (name/value pairs), calling `executor.push_field`
///     for each field. For example, `x : 42`.
///   * An Action is executed to cause a debug side effect or assign
///     a local variable, and no values or fields are produced.
///     Every action is also a value generator that produces 0 values.
///     Every action is also a field generator that produces 0 fields.
///     For example, `assert(x>0)`.
struct Operation : public Meaning
{
    Operation(Shared<const Phrase> syntax, bool is_expr)
    :
        Meaning(std::move(syntax)),
        is_expr_(is_expr)
    {}

    // pure_ is true if the Operation can be proven to be a referentially
    // transparent expression whose value does not depend on mutable variables.
    bool pure_ = false;
    // is_expr_ is true if the Operation is an expression and if eval() can be
    // called. Otherwise, the Operation is a statement but not an expression.
    bool is_expr_;

    // These functions are called during semantic analysis.
    virtual Shared<Operation> to_operation(Source_State&);
    virtual Shared<Meaning> call(const Call_Phrase&, Environ&);

    // An object that is used to execute a statement,
    // in conjunction with Operation::exec().
    struct Executor
    {
        // The Context argument denotes the statement which generated the
        // value or field, not the value or field itself.
        virtual void push_value(Value, const Context&) = 0;
        virtual void push_field(Symbol_Ref, Value, const Context&) = 0;
    };

    // These functions are called during evaluation.
    virtual Value eval(Frame&) const;
    virtual void tail_eval(std::unique_ptr<Frame>&) const;
    virtual void exec(Frame&, Executor&) const = 0;

    // These functions are called by the Shape Compiler.
    virtual SC_Value sc_eval(SC_Frame&) const;
    virtual void sc_exec(SC_Frame&) const;

    // Called when using a pure Operation as a key to an unordered_map.
    virtual size_t hash() const noexcept;
    virtual bool hash_eq(const Operation&) const noexcept;

    virtual void print(std::ostream&) const;
};

inline std::ostream&
operator<<(std::ostream& out, const Operation& op)
{
    op.print(out);
    return out;
}

// A Locative is the phrase on the left side of an assignment statement.
//
// TODO: Efficient indexed update using "linear logic".
// For indexed update, we move the value out of the location (without changing
// its reference count), use COW to amend the moved value, then store().
// We need a 'steal' or 'fetch_move' operation.
struct Locative
{
    Locative(Shared<const Phrase> syntax)
    :
        syntax_(std::move(syntax))
    {}
    virtual ~Locative() = default;
    Shared<const Phrase> syntax_;

    virtual Value fetch(Frame&) const = 0;
    virtual void store(Frame&, Value, const At_Syntax&) const = 0;
    virtual SC_Type sc_print(SC_Frame&) const;
};

} // namespace curv
#endif // header guard
