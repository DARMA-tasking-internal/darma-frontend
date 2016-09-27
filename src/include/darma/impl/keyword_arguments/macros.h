/*
//@HEADER
// ************************************************************************
//
//                   keyword_arguments/macros.h
//                         darma_runtime
//              Copyright (C) 2015 Sandia Corporation
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

#include <type_traits>

#include <darma/impl/keyword_arguments/keyword_tag.h>

#ifndef KEYWORD_ARGUMENTS_MACROS_H_
#define KEYWORD_ARGUMENTS_MACROS_H_

////////////////////////////////////////////////////////////////////////////////

/* Keyword argument declaration macros                                   {{{1 */ #if 1 // begin fold

#define __DARMA_detail_remove_parens(...) __VA_ARGS__

#define __DARMA_declare_keyword_tag_typeless(argument_for, name)                             \
namespace darma_runtime {                                                                          \
namespace keyword_tags_for_##argument_for {                                                         \
                                                                                                    \
class name : public detail::keyword_tag                                                             \
{                                                                                                   \
  public:                                                                                           \
    typedef std::true_type is_tag_t;                                                                \
    typedef __DARMA_detail_remove_parens (darma_runtime::detail::unknown_type) value_t;           \
};                                                                                                  \
                                                                                                    \
} /* end keyword_tags_for_<argument_for> */                                                         \
namespace keyword_arguments_for_##argument_for {                                                    \
                                                                                                    \
typedef detail::typeless_keyword_argument_name<                                                              \
  darma_runtime::keyword_tags_for_##argument_for::name                                             \
> name##_keyword_name_t;                                                                            \
                                                                                                    \
namespace {                                                                                         \
/* anonymous namespace so that static values are linked as one per translation unit */              \
static constexpr name##_keyword_name_t name;                                                        \
} /* end of anonymous namespace */                                                                  \
} /* end keyword_arguments_for_<argument_for> */                                                    \
} /* end namespace darma_runtime */

#define __DARMA_declare_keyword_tag_stuff(argument_for, name, argtype)                             \
namespace darma_runtime {                                                                          \
namespace keyword_tags_for_##argument_for {                                                         \
                                                                                                    \
class name : public detail::keyword_tag                                                             \
{                                                                                                   \
  public:                                                                                           \
    typedef std::true_type is_tag_t;                                                                \
    typedef __DARMA_detail_remove_parens argtype value_t;                                                        \
};                                                                                                  \
                                                                                                    \
} /* end keyword_tags_for_<argument_for> */                                                         \
namespace keyword_arguments_for_##argument_for {                                                    \
                                                                                                    \
typedef detail::keyword_argument_name<                                                              \
  __DARMA_detail_remove_parens argtype ,                                                                         \
  darma_runtime::keyword_tags_for_##argument_for::name                                             \
> name##_keyword_name_t;                                                                            \
                                                                                                    \
namespace {                                                                                         \
/* anonymous namespace so that static values are linked as one per translation unit */              \
static constexpr name##_keyword_name_t name;                                                        \
} /* end of anonymous namespace */                                                                  \
} /* end keyword_arguments_for_<argument_for> */                                                    \
} /* end namespace darma_runtime */

#define __DARMA_declare_keyword_tag_data(argument_for, name, argtype)                              \
namespace darma_runtime {                                                                          \
namespace detail {                                                                                  \
template<>                                                                                          \
struct tag_data<darma_runtime::keyword_tags_for_##argument_for::name>                              \
{                                                                                                   \
  typedef keyword_arguments_for_##argument_for::name##_keyword_name_t keyword_name_t;               \
  typedef __DARMA_detail_remove_parens argtype value_t;                                            \
  static constexpr bool has_default_value = false;                                                  \
  static constexpr bool has_type = not                                                              \
    std::is_same<value_t, darma_runtime::detail::unknown_type>::type::value;                        \
};                                                                                                  \
} /* end namespace detail */                                                                        \
} /* end namespace darma_runtime */

#define __DARMA_declare_keyword_helper(argument_for, name, argtype)                                \
    __DARMA_declare_keyword_tag_stuff(argument_for, name, argtype)                                 \
    __DARMA_declare_keyword_tag_data(argument_for, name, argtype)                                  \

// DEPRECATED
#define DeclareDarmaKeyword(argument_for, name, ...)                                               \
    __DARMA_declare_keyword_helper(argument_for, name, (__VA_ARGS__))

// This is the only way we should declare keyword arguments from now on
#define DeclareDarmaTypeTransparentKeyword(argument_for, name)                                               \
    __DARMA_declare_keyword_tag_typeless(argument_for, name) \
    __DARMA_declare_keyword_tag_data(argument_for, name, (darma_runtime::detail::unknown_type))

// DEPRECATED
#define DeclareDarmaArgumentWithDefaultGetter(argument_for, name, arg_type, ...)                   \
    __DARMA_declare_keyword_tag_stuff(argument_for, name, (arg_type))                              \
    namespace darma_runtime {                                                                      \
    namespace detail {                                                                              \
    template<>                                                                                      \
    struct tag_data<darma_runtime::keyword_tags_for_##argument_for::name>                          \
    {                                                                                               \
      typedef keyword_arguments_for_##argument_for::name##_keyword_name_t keyword_name_t;           \
      typedef arg_type value_t;                                                                     \
      static constexpr bool has_default_value = true;                                               \
      static constexpr bool has_type = true;                                                        \
      static value_t get_default_value() __VA_ARGS__                                                \
    };                                                                                              \
    } /* end namespace detail */                                                                    \
    } /* end namespace darma_runtime */

// DEPRECATED
#define DeclareDarmaArgumentWithDefault(argument_for, name, arg_type, ...)                         \
  DeclareDarmaArgumentWithDefaultGetter(argument_for, name, arg_type, { return __VA_ARGS__; })

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////





#endif /* KEYWORD_ARGUMENTS_MACROS_H_ */
