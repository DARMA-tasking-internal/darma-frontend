/*
//@HEADER
// ************************************************************************
//
//                       test_tuple_for_each.cc
//                         darma
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_TESTS_META_BLABBERMOUTH_H
#define DARMA_TESTS_META_BLABBERMOUTH_H

#include <type_traits>

#include <gmock/gmock.h>
#include <tinympl/string.hpp>
#include <tinympl/size.hpp>
#include <tinympl/find.hpp>

class AbstractBlabbermouthListener {
  public:
    virtual void default_ctor() const = 0;
    virtual void string_ctor() const = 0;
    virtual void nonconst_copy_ctor() const = 0;
    virtual void copy_ctor() const = 0;
    virtual void copy_assignment_op() const = 0;
    virtual void move_ctor() const = 0;
    virtual void move_assignment_op() const = 0;
};

class MockBlabbermouthListener {
  public:
    MOCK_CONST_METHOD0(default_ctor, void());
    MOCK_CONST_METHOD0(string_ctor, void());
    MOCK_CONST_METHOD0(nonconst_copy_ctor, void());
    MOCK_CONST_METHOD0(copy_ctor, void());
    MOCK_CONST_METHOD0(move_ctor, void());
    MOCK_CONST_METHOD0(copy_assignment_op, void());
    MOCK_CONST_METHOD0(move_assignment_op, void());
};

typedef enum CTorType {
  Default,
  String,
  NonConstCopy,
  Copy,
  Move,
  CopyAssignment,
  MoveAssignment
} CTorType;


template <CTorType... enabled_ctors>
class EnableCTorBlabbermouth
{
  private:

    typedef tinympl::vector_c<CTorType, enabled_ctors...> enabled_ctors_t;

    template <CTorType C, typename _Ignored>
    using enable_ctor_if_t = std::enable_if_t<
      (tinympl::find<enabled_ctors_t, std::integral_constant<CTorType, C>>::value
        != tinympl::size<enabled_ctors_t>::value)
      && std::is_same<_Ignored, void>::value
    >;

  public:

    template <typename _Ignored = void, typename = enable_ctor_if_t<Default, _Ignored>>
    EnableCTorBlabbermouth() noexcept
    { listener->default_ctor(); }

    template <typename _Ignored = void, typename = enable_ctor_if_t<String, _Ignored>>
    EnableCTorBlabbermouth(std::string const& str) noexcept
      : data(str)
    { listener->string_ctor(); }

    template <typename _Ignored = void, typename = enable_ctor_if_t<NonConstCopy, _Ignored>>
    EnableCTorBlabbermouth(EnableCTorBlabbermouth& b) noexcept
      : data(b.data)
    { listener->nonconst_copy_ctor(); }

    template <typename _Ignored = void, typename = enable_ctor_if_t<Copy, _Ignored>>
    EnableCTorBlabbermouth(EnableCTorBlabbermouth const& b) noexcept
      : data(b.data)
    { listener->copy_ctor(); }

    template <typename _Ignored = void, typename = enable_ctor_if_t<Move, _Ignored>>
    EnableCTorBlabbermouth(EnableCTorBlabbermouth&& b) noexcept
      : data(std::move(b.data))
    { listener->move_ctor(); }

    template <typename _Ignored = void, typename = enable_ctor_if_t<MoveAssignment, _Ignored>>
    EnableCTorBlabbermouth&
    operator=(EnableCTorBlabbermouth&& b) noexcept
    { data = std::move(b.data); listener->move_assignment_op(); return *this; }

    template <typename _Ignored = void, typename = enable_ctor_if_t<CopyAssignment, _Ignored>>
    EnableCTorBlabbermouth&
    operator=(EnableCTorBlabbermouth const& b) noexcept
    { data = b.data; listener->copy_assignment_op(); return *this; }

    std::string data;

    static MockBlabbermouthListener* listener;
};
template <CTorType... ctors>
MockBlabbermouthListener* EnableCTorBlabbermouth<ctors...>::listener;


template <CTorType... ctors>
class DisableCTorBlabbermouth
{
  private:

    typedef tinympl::vector_c<CTorType, ctors...> enabled_ctors_t;

    template <CTorType C, typename _Ignored>
    using disable_ctor_if_t = std::enable_if_t<
      (tinympl::find<enabled_ctors_t, std::integral_constant<CTorType, C>>::value
        == tinympl::size<enabled_ctors_t>::value)
        && std::is_same<_Ignored, void>::value
    >;

  public:

    template <typename _Ignored = void, typename = disable_ctor_if_t<Default, _Ignored>>
    DisableCTorBlabbermouth() noexcept
    { listener->default_ctor(); }

    template <typename _Ignored = void, typename = disable_ctor_if_t<String, _Ignored>>
    DisableCTorBlabbermouth(std::string const&) noexcept
    { listener->string_ctor(); }

    template <typename _Ignored = void, typename = disable_ctor_if_t<NonConstCopy, _Ignored>>
    DisableCTorBlabbermouth(DisableCTorBlabbermouth&) noexcept
    { listener->nonconst_copy_ctor(); }

    template <typename _Ignored = void, typename = disable_ctor_if_t<Copy, _Ignored>>
    DisableCTorBlabbermouth(DisableCTorBlabbermouth const&) noexcept
    { listener->copy_ctor(); }

    template <typename _Ignored = void, typename = disable_ctor_if_t<Move, _Ignored>>
    DisableCTorBlabbermouth(DisableCTorBlabbermouth&&) noexcept
    { listener->move_ctor(); }

    template <typename _Ignored = void, typename = disable_ctor_if_t<MoveAssignment, _Ignored>>
    DisableCTorBlabbermouth&
    operator=(DisableCTorBlabbermouth&&) noexcept
    { listener->move_assignment_op(); return *this; }

    template <typename _Ignored = void, typename = disable_ctor_if_t<CopyAssignment, _Ignored>>
    DisableCTorBlabbermouth&
    operator=(DisableCTorBlabbermouth const&) noexcept
    { listener->copy_assignment_op(); return *this; }

    static MockBlabbermouthListener* listener;

};
template <CTorType... ctors>
MockBlabbermouthListener* DisableCTorBlabbermouth<ctors...>::listener;

#endif //DARMA_TESTS_META_BLABBERMOUTH_H
