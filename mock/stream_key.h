/*
//@HEADER
// ************************************************************************
//
//                          stream_key.h
//                         dharma_new
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

#ifndef MOCK_STREAM_KEY_H_
#define MOCK_STREAM_KEY_H_

#include <util.h>
#include <sstream>
#include <iostream>
#include <cassert>
#include <meta/tuple_for_each.h>
#include <abstract/defaults/key_fwd.h>

namespace mock_backend {

namespace detail {

template <typename T>
struct as_impl {
  T operator()(const std::string& str) const {
    T rv;
    std::stringstream sstr(str);
    sstr >> rv;
    return rv;
  }
};

template <>
struct as_impl<const char*> {
  const char* operator()(const std::string& str) const {
    return str.c_str();
  }
};

struct stream_key_part {

  stream_key_part(const std::string& str) : my_str_(str) { }

  template <typename T>
  T as() const {
    as_impl<T>()(my_str_);
  }

  std::string my_str_;

};

} // end namespace detail

struct key_hash;
struct key_equal;

struct StreamKey  {

  public:

    StreamKey() : StreamKey(dharma_runtime::detail::variadic_constructor_arg) { }

    template <typename... Types>
    explicit StreamKey(
      const dharma_runtime::detail::variadic_constructor_arg_t,
      Types&&... data
    ) {
      std::stringstream sstr;
      dharma_runtime::meta::tuple_for_each(std::make_tuple(data...), [&sstr](const auto& val) {
        sstr << val << std::endl;
      });
      val_ = sstr.str();
      n_parts_ = sizeof...(Types);
    }

    template <typename... Types>
    explicit StreamKey(
      std::tuple<Types...>&& data
    ) {
      std::stringstream sstr;
      dharma_runtime::meta::tuple_for_each(data, [&sstr](const auto& val) {
        sstr << val << std::endl;
      });
      val_ = sstr.str();
      n_parts_ = sizeof...(Types);
    }

    template <unsigned Spot>
    const detail::stream_key_part
    component() const {
      assert(Spot < n_parts_);
      std::stringstream sstr(val_);
      char buffer[255];
      for(int i = 0; i < Spot; ++i) {
        sstr.getline(buffer, 255);
      }
      std::string rv_str(buffer);
      return detail::stream_key_part(rv_str);
    }

  private:

    friend struct key_hash;
    friend struct key_equal;

    std::string val_;
    size_t n_parts_;
};

struct key_hash {
  size_t
  operator()(const StreamKey& a) const {
    return std::hash<std::string>()(a.val_);
  }
};

struct key_equal {
  bool
  operator()(const StreamKey& a, const StreamKey& b) const {
    return a.val_ == b.val_;
  }
};

inline bool
operator==(const StreamKey& a, const StreamKey& b) {
  return key_equal()(a, b);
}

} // end namespace mock_backend

namespace dharma_runtime {

namespace detail {

template <>
struct key_traits<mock_backend::StreamKey>
{
  struct maker {
    constexpr maker() = default;
    template <typename... Args>
    inline mock_backend::StreamKey
    operator()(Args&&... args) const {
      return mock_backend::StreamKey(
        detail::variadic_constructor_arg,
        std::forward<Args>(args)...
      );
    }
  };

  struct maker_from_tuple {
    constexpr maker_from_tuple() = default;
    template <typename... Args>
    inline mock_backend::StreamKey
    operator()(std::tuple<Args...>&& data) const {
      return mock_backend::StreamKey(
        std::forward<std::tuple<Args...>>(data)
      );
    }
  };

  typedef mock_backend::key_hash hasher;
  typedef mock_backend::key_equal key_equal;

};

} // end namespace detail

} // end namespace dharma_runtime



#endif /* MOCK_STREAM_KEY_H_ */
