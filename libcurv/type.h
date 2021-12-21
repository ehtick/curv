// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TYPE_H
#define LIBCURV_TYPE_H

#include <libcurv/context.h>
#include <libcurv/conval.h>
#include <libcurv/value.h>

namespace curv {

// A plex type is either a scalar type (Bool, Num)
// or it is one of the list types that are treated specially
// by GPU shader languages (the WGSL and SPIR-V type systems).
// Plex types are the argument and result types of SPIR-V primitive functions.
enum class Plex_Type : short
{
    missing,    // flag value: not a plex type
    Bool,
    Bool2,
    Bool3,
    Bool4,
    Bool32,
    Bool2x32,
    Bool3x32,
    Bool4x32,
    Num,
    Vec2,
    Vec3,
    Vec4,
    Mat2,
    Mat3,
    Mat4
};

extern const char* glsl_plex_type_name[];

struct Type : public Ref_Value
{
    Plex_Type plex_type_;
    Type(int st, Plex_Type pt) : Ref_Value(ty_type, st), plex_type_(pt) {}
    static Shared<const Type> Error();
    static Shared<const Type> Bool();
    static Shared<const Type> Bool32();
    static Shared<const Type> Num();

    static bool equal(const Type&, const Type&);
    virtual bool contains(Value, const At_Syntax&) const = 0;
    unsigned rank() const;

    Shared<const Type> plex_array_base() const;
    unsigned plex_array_rank() const;
    unsigned plex_array_dim(unsigned) const;

    static const char name[];
};

Shared<const Type> value_to_type(Value, Fail, const Context&);

inline std::ostream&
operator<<(std::ostream& out, const Type& type)
{
    type.print_repr(out, Prec::listing);
    return out;
}

// A concrete value class for Curv type values.
struct CType : public Concrete_Value
{
    Shared<const Type> data_;
    CType() : data_(nullptr) {}
    CType(Shared<const Type> data) : data_(std::move(data)) {}
    CType(Value v, const At_Syntax& cx) : data_(v.to<const Type>(cx)) {}
    /*
    static CType from_value(Value v, const At_Syntax&)
      { return {v.maybe<const Type>()}; }
    static CType from_value(Value v, Fail fl, const At_Syntax&cx)
      { return {v.to<const Type>(fl,cx)}; }
     */
    static CType from_value(Value v, const At_Syntax& cx)
      { return {value_to_type(v, Fail::soft, cx)}; }
    static CType from_value(Value v, Fail fl, const At_Syntax& cx)
      { return {value_to_type(v, fl, cx)}; }
    Value to_value() const { return Value{data_}; }
    bool operator==(CType t) const
      { return Type::equal(*data_, *t.data_); }
    bool operator!=(CType t) const
      { return !Type::equal(*data_, *t.data_); }
    void print_repr(std::ostream& o, Prec p) const { data_->print_repr(o,p); }
    void print_string(std::ostream& o) const { data_->print_string(o); }
    friend std::ostream& operator<<(std::ostream& o, CType t)
      { t.print_repr(o,Prec::item); return o; }
    explicit operator bool() const noexcept { return data_ != nullptr; }

    bool contains(Value v, const At_Syntax& cx) const
      { return data_->contains(v, cx); }
};

} // namespace curv
#endif // header guard
