// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/program.h>

#include <libcurv/analyser.h>
#include <libcurv/builtin.h>
#include <libcurv/context.h>
#include <libcurv/definition.h>
#include <libcurv/exception.h>
#include <libcurv/parser.h>
#include <libcurv/scanner.h>
#include <libcurv/system.h>

namespace curv {

void
Program::compile(const Namespace* names)
{
    if (names == nullptr)
        names = &sstate_.system_.std_namespace();
    Builtin_Environ env{*names, sstate_};
    compile(env);
}

void
Program::compile(Environ& env)
{
    phrase_ = parse_program(scanner_);
    if (auto def = phrase_->as_definition(env, Fail::soft)) {
        module_ = analyse_module(*def, env);
    } else {
        meaning_ = phrase_->analyse(env, terp_);
    }
    frame_ = {Frame::make(env.frame_maxslots_, sstate_, sstate_.file_frame_,
        nullptr, nullptr)};
}

const Phrase&
Program::nub()
const
{
    return *nub_phrase(phrase_);
}

Src_Loc
Program::location()
const
{
    if (phrase_ == nullptr) {
        return Src_Loc{
            scanner_.source_,
            Token{unsigned(scanner_.ptr_ - scanner_.source_->begin()),
                  unsigned(scanner_.source_->size())}};
    } else {
        return nub_phrase(phrase_)->location();
    }
}

Value
Program::eval()
{
    if (module_ != nullptr) {
        throw Exception(At_Phrase(*phrase_, scanner_),
            "definition found; expecting an expression");
    } else {
        auto expr = meaning_->to_operation(sstate_);
        frame_->next_op_ = &*expr;
        return tail_eval_frame(move(frame_));
    }
}

Shared<Module>
Program::exec(Operation::Executor& ex)
{
    if (module_) {
        return module_->eval_module(*frame_);
    } else {
        auto op = meaning_->to_operation(sstate_);
        op->exec(*frame_, ex);
        return nullptr;
    }
}

} // namespace curv
