/*
//@HEADER
// ************************************************************************
//
//                      macros.h
//                         DARMA
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
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
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#ifndef DARMA_MACROS_H
#define DARMA_MACROS_H

// Borrowed from google test
// Due to C++ preprocessor weirdness, we need double indirection to
// concatenate two tokens when one of them is __LINE__.  Writing
//
//   foo ## __LINE__
//
// will result in the token foo__LINE__, instead of foo followed by
// the current line number.  For more details, see
// https://isocpp.org/wiki/faq/misc-technical-issues#macros-with-token-pasting
#define DARMA_CONCAT_TOKEN_(foo, bar) DARMA_CONCAT_TOKEN_IMPL_(foo, bar)
#define DARMA_CONCAT_TOKEN_IMPL_(foo, bar) foo ## bar

// Used for static debugging
template <typename T>
struct _____________________________TYPE_DISPLAY________________________________;
#define _DARMA_HIDE_TYPE_DISPLAY() _____________________________TYPE_DISPLAY________________________________
#define DARMA_TYPE_DISPLAY(...) \
  _DARMA_HIDE_TYPE_DISPLAY()<__VA_ARGS__> DARMA_CONCAT_TOKEN_(_type_display_macro_on_line_, __LINE__);


#define _darma_PP_FOR_EACH(macro, ...) _darma_PP_EVAL( _darma_PP_FOR_EACH_IMPL(macro, __VA_ARGS__) )

#define _darma_REMOVE_PARENS(args...) args

#define _darma_PP_FOR_EACH_IMPL(macro, ...) _darma_PP_FOR_EACH_NEXT_IMPL(__VA_ARGS__) (macro, __VA_ARGS__)
#define _darma_PP_FOR_EACH_NEXT_IMPL(...) _darma_PP_CAT(_darma_PP_FOR_EACH_IMPL_, _darma_PP_EACH_ARG_HELPER(__VA_ARGS__))

#define _darma_PP_COUNTING_HELPER(X100, X99, X98, X97, X96, X95, X94, X93, X92, X91, X90, X89, X88, X87, X86, X85, X84, X83, X82, X81, X80, X79, X78, X77, X76, X75, X74, X73, X72, X71, X70, X69, X68, X67, X66, X65, X64, X63, X62, X61, X60, X59, X58, X57, X56, X55, X54, X53, X52, X51, X50, X49, X48, X47, X46, X45, X44, X43, X42, X41, X40, X39, X38, X37, X36, X35, X34, X33, X32, X31, X30, X29, X28, X27, X26, X25, X24, X23, X22, X21, X20, X19, X18, X17, X16, X15, X14, X13, X12, X11, X10, X9, X8, X7, X6, X5, X4, X3, X2, X1, N, ...)   N
#define _darma_PP_EACH_ARG_HELPER(...) _darma_PP_COUNTING_HELPER(__VA_ARGS__, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, RECURSE, \
  RECURSE, RECURSE, RECURSE, RECURSE, BASE \
)
#define _darma_PP_FOR_EACH_IMPL_DO(macro, arg) macro(arg)

#define _darma_PP_CAT(a, ...) _darma_PP_CAT_IMPL(a, __VA_ARGS__)
#define _darma_PP_CAT_IMPL(a, ...) a ## __VA_ARGS__

#define _darma_PP_EMPTY()
#define _darma_PP_DEFER(id) id _darma_PP_EMPTY()
#define _darma_PP_OBSTRUCT(...) __VA_ARGS__ _darma_PP_DEFER(_darma_PP_EMPTY)()
#define _darma_PP_EXPAND(...) __VA_ARGS__

// Evaluate on different layers of the preprocessor to make sure everything gets evaluated
#define _darma_PP_EVAL(...)  _darma_PP_EVAL1(_darma_PP_EVAL1(_darma_PP_EVAL1(__VA_ARGS__)))
#define _darma_PP_EVAL1(...) _darma_PP_EVAL2(_darma_PP_EVAL2(_darma_PP_EVAL2(__VA_ARGS__)))
#define _darma_PP_EVAL2(...) _darma_PP_EVAL3(_darma_PP_EVAL3(_darma_PP_EVAL3(__VA_ARGS__)))
#define _darma_PP_EVAL3(...) _darma_PP_EVAL4(_darma_PP_EVAL4(_darma_PP_EVAL4(__VA_ARGS__)))
#define _darma_PP_EVAL4(...) _darma_PP_EVAL5(_darma_PP_EVAL5(_darma_PP_EVAL5(__VA_ARGS__)))
#define _darma_PP_EVAL5(...) __VA_ARGS__

#define _darma_PP_FOR_EACH_IMPL_RECURSE(macro, arg, ...) _darma_PP_FOR_EACH_IMPL_DO(macro, arg), _darma_PP_OBSTRUCT(_darma_PP_FOR_EACH_IMPL_INDIRECT) ( ) (macro, __VA_ARGS__)
#define _darma_PP_FOR_EACH_IMPL_BASE(macro, arg) _darma_PP_FOR_EACH_IMPL_DO(macro, arg)
#define _darma_PP_FOR_EACH_IMPL_INDIRECT() _darma_PP_FOR_EACH_IMPL


#endif //DARMA_MACROS_H
