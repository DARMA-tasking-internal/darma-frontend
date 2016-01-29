/*
//@HEADER
// ************************************************************************
//
//                   keyword_arguments/macros.h
//                         dharma_runtime
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

#ifndef KEYWORD_ARGUMENTS_MACROS_H_
#define KEYWORD_ARGUMENTS_MACROS_H_

////////////////////////////////////////////////////////////////////////////////

/* Keyword argument declaration macros                                   {{{1 */ #if 1 // begin fold

#define __remove_parens(...) __VA_ARGS__

#define __DHARMA_declare_keyword_tag_stuff(argument_for, name, argtype, arg_catagory)               \
namespace dharma_runtime {                                                                          \
namespace keyword_tags_for_##argument_for {                                                         \
                                                                                                    \
class name : public detail::keyword_tag                                                             \
{                                                                                                   \
  public:                                                                                           \
    typedef std::true_type is_tag_t;                                                                \
    typedef __remove_parens argtype value_t;                                                        \
};                                                                                                  \
                                                                                                    \
} /* end keyword_tags_for_<argument_for> */                                                         \
namespace keyword_arguments_for_##argument_for {                                                    \
                                                                                                    \
typedef detail::keyword_argument_name<                                                              \
  __remove_parens argtype ,                                                                         \
  dharma_runtime::keyword_tags_for_##argument_for::name,                                             \
  dharma_runtime::detail::keyword_catagory_##arg_catagory                                            \
> name##_keyword_name_t;                                                                            \
                                                                                                    \
namespace {                                                                                         \
/* anonymous namespace so that static values are linked as one per translation unit */              \
static constexpr name##_keyword_name_t name;                                                        \
} /* end of anonymous namespace */                                                                  \
} /* end keyword_arguments_for_<argument_for> */                                                    \
} /* end namespace dharma_runtime */

#define __DHARMA_declare_keyword_tag_data(argument_for, name, argtype, arg_catagory)                \
namespace dharma_runtime {                                                                          \
namespace detail {                                                                                  \
template<>                                                                                          \
struct tag_data<dharma_runtime::keyword_tags_for_##argument_for::name>                               \
{                                                                                                   \
  typedef keyword_arguments_for_##argument_for::name##_keyword_name_t keyword_name_t;               \
  typedef __remove_parens argtype value_t;                                                          \
  typedef keyword_catagory_##arg_catagory keyword_catagory_t;                                       \
  static constexpr bool has_default_value = false;                                                  \
};                                                                                                  \
} /* end namespace detail */                                                                        \
} /* end namespace dharma_runtime */

#define __DHARMA_declare_keyword_helper(argument_for, name, argtype, arg_catagory)                  \
    __DHARMA_declare_keyword_tag_stuff(argument_for, name, argtype, arg_catagory)                   \
    __DHARMA_declare_keyword_tag_data(argument_for, name, argtype, arg_catagory)                    \

#define DeclareDharmaKeyword(argument_for, name, ...)                                               \
    __DHARMA_declare_keyword_helper(argument_for, name, (__VA_ARGS__), unknown)

#define DeclareDharmaArgument(argument_for, name, ...)                                              \
    __DHARMA_declare_keyword_helper(argument_for, name, (__VA_ARGS__), argument)

#define DeclareDharmaInput(argument_for, name, ...)                                                 \
    __DHARMA_declare_keyword_helper(argument_for, name, (__VA_ARGS__), input)

#define DeclareDharmaOutput(argument_for, name, ...)                                                \
    __DHARMA_declare_keyword_helper(argument_for, name, (__VA_ARGS__), output)

#define DeclareDharmaArgumentWithDefaultGetter(argument_for, name, arg_type, ...)                   \
    __DHARMA_declare_keyword_tag_stuff(argument_for, name, (arg_type), argument)                    \
    namespace dharma_runtime {                                                                      \
    namespace detail {                                                                              \
    template<>                                                                                      \
    struct tag_data<dharma_runtime::keyword_tags_for_##argument_for::name>                          \
    {                                                                                               \
      typedef keyword_arguments_for_##argument_for::name##_keyword_name_t keyword_name_t;           \
      typedef arg_type value_t;                                                                     \
      typedef keyword_catagory_argument keyword_catagory_t;                                         \
      static constexpr bool has_default_value = true;                                               \
      static value_t get_default_value() __VA_ARGS__                                                \
    };                                                                                              \
    } /* end namespace detail */                                                                    \
    } /* end namespace dharma_runtime */

#define DeclareDharmaArgumentWithDefault(argument_for, name, arg_type, ...)                         \
  DeclareDharmaArgumentWithDefaultGetter(argument_for, name, arg_type, { return __VA_ARGS__; })

/*                                                                            */ #endif // end fold

////////////////////////////////////////////////////////////////////////////////





#endif /* KEYWORD_ARGUMENTS_MACROS_H_ */
