/*
//@HEADER
// ************************************************************************
//
//                      SSO_key.h
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

#ifndef DARMA_IMPL_KEY_SSO_KEY_H
#define DARMA_IMPL_KEY_SSO_KEY_H

#include <darma/key/bytes_convert.h>
#include <darma/key/SSO_key_fwd.h>
#include <darma/key/key_concept.h>

#include <darma/serialization/serialization_fwd.h>

#include <darma/utility/tag_types.h>
#include <darma/utility/as_bytes.h>

#include <tinympl/vector.hpp>

#include <string>

namespace darma_runtime {

namespace detail {

namespace _impl {

static constexpr uint8_t _component_index_not_given = std::numeric_limits<uint8_t>::max();

template <typename T> struct is_sso_key : std::false_type { };
template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
struct is_sso_key<SSOKey<BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal>>
  : std::true_type { };

template <
  typename T,
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
size_t sso_key_add(
  SSOKey<BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal>& key,
  std::enable_if_t<not is_sso_key<std::decay_t<T>>::value, char*&> buffer,
  T&& arg
) {

  bytes_convert<std::remove_reference_t<T>> bc;
  DARMA_ASSERT_RELATED_VERBOSE(
    bc.get_size(arg), <=, std::numeric_limits<PieceSizeOrdinal>::max()
  );
  const PieceSizeOrdinal size = bc.get_size(arg);
  *reinterpret_cast<PieceSizeOrdinal*>(buffer) = size;
  buffer += sizeof(PieceSizeOrdinal);
  auto* md = reinterpret_cast<bytes_type_metadata*>(buffer);
  bc.get_bytes_type_metadata(md, arg);
  DARMA_ASSERT_MESSAGE(not has_category_extension_byte(md),
    "Extended enum support not yet available; bug the developers if you need this (too many different enum types used with keys in your program)"
  );
  buffer += sizeof(bytes_type_metadata);
  bc(std::forward<decltype(arg)>(arg), buffer, size, 0);
  buffer += size;
  return 1;
}

//==============================================================================
// <editor-fold desc="SSOKeyAttorney">

struct SSOKeyAttorney {
  template <typename SSOKeyT>
  static const char* get_data_pointer(
    SSOKeyT const& key
  ) {
    return key._data_pointer();
  }
  template <typename SSOKeyT>
  static size_t get_data_size(
    SSOKeyT const& key
  ) {
    return key._data_size();
  }
  template <typename SSOKeyT>
  static _impl::sso_key_mode_t get_mode(
    SSOKeyT const& key
  ) {
    return key.mode;
  }
  template <typename SSOKeyT>
  static
  typename SSOKeyT::piece_size_ordinal_t
  _get_piece_size_ordinal_t();
  template <typename SSOKeyT>
  using piece_size_ordinal_t = decltype(_get_piece_size_ordinal_t<SSOKeyT>());
  template <typename SSOKeyT>
  static
  typename SSOKeyT::component_count_ordinal_t
  _get_component_count_ordinal_t();
  template <typename SSOKeyT>
  using component_count_ordinal_t = decltype(_get_component_count_ordinal_t<
    SSOKeyT
  >());
  template <typename SSOKeyT>
  static bool is_awaiting_backend_key(
    SSOKeyT const& key
  ) {
    return key._needs_backend_assigned_key();
  }

};

// </editor-fold> end SSOKeyAttorney
//==============================================================================

template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
size_t sso_key_add(
  SSOKey<
    BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal
  >& key,
  char*& buffer,
  SSOKey<
    BufferSize, BackendAssignedKeyType, PieceSizeOrdinal, ComponentCountOrdinal
  > const& add_key
) {
  DARMA_ASSERT_MESSAGE(
    SSOKeyAttorney::get_mode(add_key) != _impl::BackendAssigned,
    "Combining a backend generated key into a user generated key is not allowed"
  );
  const char* add_buffer = SSOKeyAttorney::get_data_pointer(add_key);
  auto n_comps = *reinterpret_cast<ComponentCountOrdinal const*>(add_buffer);
  add_buffer += sizeof(ComponentCountOrdinal);
  ::memcpy(buffer, add_buffer,
    SSOKeyAttorney::get_data_size(add_key) - sizeof(ComponentCountOrdinal));
  buffer += SSOKeyAttorney::get_data_size(add_key) - sizeof(ComponentCountOrdinal);
  return n_comps;
}

template <typename...> struct _do_sum;

// Need to pop off the back to be left-associative
template <typename TN, typename Arg0, typename... Args>
struct _do_sum<TN, tinympl::vector<Arg0, Args...>> {
  using _rest_vector_t = tinympl::vector<Arg0, Args...>;
  inline auto
  operator()(Arg0&& arg0, Args&&... args, TN&& vn) const {
    return _do_sum<
      typename _rest_vector_t::back::type,
      typename _rest_vector_t::pop_back::type
    >()(std::forward<Arg0>(arg0), std::forward<Args>(args)...) + vn;
  }
};

template <typename TN>
struct _do_sum<TN, tinympl::vector<>> {
  inline auto
  operator()(TN&& vn) const {
    return vn;
  }
};


// TODO move this somewhere more logical
template <typename Arg0, typename... Args>
inline auto _sum(Arg0&& arg0, Args&&... args) {
  using _rest_vector_t = tinympl::vector<Arg0, Args...>;
  return _do_sum<
    typename _rest_vector_t::back::type,
    typename _rest_vector_t::pop_back::type
  >()(std::forward<Arg0>(arg0), std::forward<Args>(args)...);

}

inline auto _sum() {
  return 0ul;
}

} // end namespace _impl

//==============================================================================
// <editor-fold desc="SSOKey implementation">

// A key that employs an optimization similar to the short-string optimization in std::string
template <
  /* default allows it to fit in a cache line */
  size_t BufferSize /* = 64 - sizeof(size_t) - sizeof(_impl::sso_key_mode_t) */,
  typename BackendAssignedKeyType /*= size_t */,
  // TODO this could be removed because the size each piece is redundant information
  typename PieceSizeOrdinal /*= uint8_t */,
  typename ComponentCountOrdinal /*= uint8_t */
>
class SSOKey {
  private:

    using piece_size_ordinal_t = PieceSizeOrdinal;
    using component_count_ordinal_t = ComponentCountOrdinal;

    struct backend_assigned_key_tag { };
    struct request_backend_assigned_key_tag { };
    struct unpack_ctor_tag { };

    struct _short {
      size_t size;
      char data[BufferSize];
    };

    struct _long {
      size_t size;
      char* data;
    };

    struct _backend_assigned {
      BackendAssignedKeyType backend_assigned_key;
    };

    struct alignas(8) _repr {
      _repr() = default;
      union {
        _long as_long;
        _short as_short;
        _backend_assigned as_backend_assigned;
      };
    };

    alignas(8) _impl::sso_key_mode_t mode = _impl::Long;
    alignas(8) _repr repr;


    bool _needs_backend_assigned_key() const {
      return mode == _impl::NeedsBackendAssignedKey;
    }

    explicit SSOKey(request_backend_assigned_key_tag) {
      // both values are defaults, but just for readability...
      repr.as_long = _long{};
      repr.as_long.data = nullptr;
      mode = _impl::NeedsBackendAssignedKey;
    }

    template <typename Archive>
    SSOKey(unpack_ctor_tag, Archive& ar);

    SSOKey(
      backend_assigned_key_tag,
      BackendAssignedKeyType const& value
    ) : mode(_impl::BackendAssigned)
    {
      repr.as_backend_assigned = _backend_assigned{value};
    }

    SSOKey(
      backend_assigned_key_tag,
      BackendAssignedKeyType&& value
    ) : mode(_impl::BackendAssigned)
    {
      repr.as_backend_assigned = _backend_assigned{std::move(value)};
    }

    template <typename... Args>
    SSOKey(
      utility::variadic_constructor_arg_t,
      Args&& ... args
    );

    bool _is_long() const { return mode == _impl::Long; }
    bool _is_short() const { return mode == _impl::Short; }
    bool _is_backend_assigned() const { return mode == _impl::BackendAssigned; }

    size_t _data_size() const {
      switch (mode) {
        case _impl::BackendAssigned:
          return sizeof(BackendAssignedKeyType);
        case _impl::Short:
          return repr.as_short.size;
        case _impl::Long:
          return repr.as_long.size;
        case _impl::NeedsBackendAssignedKey:
          return 0ul;
      }
    }

    char const* _data_pointer() const {
      switch (mode) {
        case _impl::BackendAssigned:
          return reinterpret_cast<const char*>(&repr.as_backend_assigned.backend_assigned_key);
        case _impl::Short:
          return repr.as_short.data;
        case _impl::Long:
          return repr.as_long.data;
        case _impl::NeedsBackendAssignedKey:
          return nullptr;
      }
      // unreachable, but prevents warnings on certain compilers
      return nullptr;                                                           // LCOV_EXCL_LINE
    }

    struct SSOKeyComponent {
      private:

        char const* data;
        PieceSizeOrdinal size;

      public:
        bytes_type_metadata const* md;

        SSOKeyComponent(
          char const* data,
          const PieceSizeOrdinal size,
          bytes_type_metadata const* md
        )
          : data(data), size(size), md(md) {}


        template <typename T>
        T as() const {
          return bytes_convert<std::remove_reference_t<T>>().get_value(
            md, data, size
          );

        }
    };


    friend struct key_traits<SSOKey>;
    friend struct bytes_convert<SSOKey, void>;

    friend struct _impl::SSOKeyAttorney;

    friend struct serialization::Serializer<SSOKey>;
    friend struct serialization::Serializer<const SSOKey>;

  public:

    SSOKey() : SSOKey(utility::variadic_constructor_arg) { }

    ~SSOKey();

    SSOKey(SSOKey const& other);
    SSOKey& operator=(SSOKey const& other) {
      this->~SSOKey();
      new (this) SSOKey(other);
      return *this;
    }

    size_t n_components() const {
      if (mode == _impl::BackendAssigned) return 0;
      else {
        assert(_data_pointer() != nullptr);
        return *reinterpret_cast<ComponentCountOrdinal const*>(_data_pointer());
      }
    }

    bool is_backend_generated() const { return _is_backend_assigned(); }

    std::string
    human_readable_string(
      const char* sep = ", ",
      const char* left_paren = "{", const char* right_paren = "}"
    ) const {
      std::ostringstream sstr;
      sstr << left_paren;
      print_human_readable(sep, sstr);
      sstr << right_paren;
      return sstr.str();
    }

    void
    print_human_readable(
      const char* sep = ", ", std::ostream& o = std::cout
    ) const {
      if (_is_backend_assigned()) {
        o << "<generated key: " << repr.as_backend_assigned.backend_assigned_key
          << ">";
        return;
      }

      if(_needs_backend_assigned_key()) {
        o << "<key pending backend assignment>";
        return;
      }

      assert(_data_pointer() != nullptr);

      char const* buffer = _data_pointer() + sizeof(ComponentCountOrdinal);
      const size_t n_comps = n_components();
      for (size_t i = 0; i < n_comps; ++i
        ) {
        const PieceSizeOrdinal
          piece_size = *reinterpret_cast<PieceSizeOrdinal const*>(buffer);
        buffer += sizeof(PieceSizeOrdinal);
        const auto* md = reinterpret_cast<bytes_type_metadata const*>(buffer);
        buffer += sizeof(bytes_type_metadata);
        if (md->is_string_like) {
          o << bytes_convert<std::string>().get_value(md, buffer, piece_size);
        } else if (md->is_int_like_type) {
          if (not reinterpret_cast<const int_like_type_metadata*>(md)->is_enumerated) {
            o << bytes_convert<intmax_t>().get_value(md, buffer, piece_size);
          } else {
            // TODO more enum verbosity
            o << "<enum>";
          }
        } else if (md->is_floating_point_like_type) {
          o << bytes_convert<double>().get_value(md, buffer, piece_size);
        } else {
          assert(false); // not implemented
        }
        buffer += piece_size;

        if (i != n_components()) o << sep;
      }
    }

    template <uint8_t N = _impl::_component_index_not_given>
    SSOKeyComponent
    component(size_t N_dynamic = _impl::_component_index_not_given) const {
      assert(N_dynamic == _impl::_component_index_not_given || N_dynamic
        < (size_t)std::numeric_limits<ComponentCountOrdinal>::max());
      static_assert(
        N == _impl::_component_index_not_given || N < std::numeric_limits<ComponentCountOrdinal>::max(),
        "tried to get key component greater than max_num_parts"
      );
      assert(N == _impl::_component_index_not_given xor N_dynamic == _impl::_component_index_not_given);
      const uint8_t actual_N = N == _impl::_component_index_not_given ? (uint8_t)N_dynamic : N;
      DARMA_ASSERT_RELATED_VERBOSE((int)actual_N, <, n_components());
      DARMA_ASSERT_MESSAGE(mode != _impl::BackendAssigned,
        "Can't get component of backend-assigned key"
      );
      assert(_data_pointer() != nullptr);
      char const* buffer = _data_pointer() + sizeof(ComponentCountOrdinal);
      for (int i = 0; i < actual_N; ++i
        ) {
        PieceSizeOrdinal
          psize = *reinterpret_cast<PieceSizeOrdinal const*>(buffer);
        buffer +=
          sizeof(PieceSizeOrdinal) + psize + sizeof(bytes_type_metadata);
      }
      return SSOKeyComponent(
        buffer + sizeof(PieceSizeOrdinal) + sizeof(bytes_type_metadata),
        *reinterpret_cast<PieceSizeOrdinal const*>(buffer),
        reinterpret_cast<bytes_type_metadata const*>(buffer
          + sizeof(PieceSizeOrdinal))
      );
    }

    bool operator<(SSOKey const& other) const {
      if(mode != other.mode) {
        return mode < other.mode;
      }
      else {
        switch(mode) {
          case _impl::BackendAssigned: {
            return repr.as_backend_assigned.backend_assigned_key
                < other.repr.as_backend_assigned.backend_assigned_key;
          }
          case _impl::Short: {
            return less_as_bytes(
                repr.as_short.data, repr.as_short.size,
                other.repr.as_short.data, other.repr.as_short.size
            );
          }
          case _impl::Long: {
            return less_as_bytes(
                repr.as_long.data, repr.as_short.size,
                other.repr.as_long.data, other.repr.as_short.size
            );
          }
          default: {
            assert(false);
          }
        }
      }
      // unreachable, but prevents warnings on certain compilers
      return false;                                                             // LCOV_EXCL_LINE
    }
};


// </editor-fold> end SSOKey implementation
//==============================================================================


//==============================================================================
// <editor-fold desc="Partial bytes_convert specialization for SSOKey">

// Note: only get_size is specialized, since the other parts should be unnecessary

template <typename SSOKeyT>
struct bytes_convert<
  SSOKeyT,
  std::enable_if_t<_impl::is_sso_key<std::decay_t<SSOKeyT>>::value>
> {
  int get_size(std::add_const_t<SSOKeyT>& key) const {
    // subtract off the extra PieceSizeOrdinal and bytes_type_metadata that were
    // allocated for this as if it were a single component rather that multiple
    // (these are included in _data_size() already for each piece of key)
    // Also subtract off the ComponentCount slot
    return (int)_impl::SSOKeyAttorney::get_data_size(key)
      - sizeof(typename _impl::SSOKeyAttorney::template piece_size_ordinal_t<
        SSOKeyT
      >)
      - sizeof(bytes_type_metadata)
      - sizeof(typename _impl::SSOKeyAttorney::template component_count_ordinal_t<
        SSOKeyT
      >);
  }
};

// </editor-fold> end Partial bytes_convert specialization for SSOKey
//==============================================================================


//==============================================================================
// <editor-fold desc="key_traits<SSOKey> implementation">

template <
  size_t BufferSize,
  typename BackendAssignedKeyType,
  typename PieceSizeOrdinal,
  typename ComponentCountOrdinal
>
struct key_traits<
  SSOKey<
    BufferSize,
    BackendAssignedKeyType,
    PieceSizeOrdinal,
    ComponentCountOrdinal
  >
> {
  using sso_key_t = SSOKey<
    BufferSize,
    BackendAssignedKeyType,
    PieceSizeOrdinal,
    ComponentCountOrdinal
  >;

  struct maker {
    template <typename... Args>
    inline sso_key_t
    operator()(Args&& ... args) const {
      return sso_key_t(
        utility::variadic_constructor_arg,
        std::forward<Args>(args)...
      );
    }
  };

  struct backend_maker {
    inline sso_key_t
    operator()(BackendAssignedKeyType const& k) const {
      return sso_key_t(
        typename sso_key_t::backend_assigned_key_tag{}, k
      );
    }
    inline sso_key_t
    operator()(BackendAssignedKeyType&& k) const {
      return sso_key_t(
        typename sso_key_t::backend_assigned_key_tag{}, std::move(k)
      );
    }
  };

  struct key_equal {
    inline bool
    operator()(sso_key_t const& k1, sso_key_t const& k2) const {
      // TODO we need to think about how we can speed this up...
      if(k1._needs_backend_assigned_key() and k2._needs_backend_assigned_key())
        return true;
      else if(k1._needs_backend_assigned_key() xor k2._needs_backend_assigned_key())
        return false;
      else
        return
          // also check that they are not mismatched with respect to backend-assigned-ness
          not(k1.mode == _impl::BackendAssigned
            xor k2.mode == _impl::BackendAssigned)
            and detail::equal_as_bytes(
              k1._data_pointer(), k1._data_size(),
              k2._data_pointer(), k2._data_size()
            );
    }
  };

  struct hasher {
    inline size_t
    operator()(sso_key_t const& k) const {
      // "empty" key hashes to 0...
      if(k._needs_backend_assigned_key()) { return 0; }
      return hash_as_bytes(k._data_pointer(), k._data_size());
    }
  };

  // This is for frontend use only; backend should use
  // abstract::frontend::Handle::has_user_defined_key()
  static bool
  needs_backend_key(sso_key_t const& key) {
    return key._needs_backend_assigned_key();
  }

  static sso_key_t
  make_awaiting_backend_assignment_key() {
    return sso_key_t(typename sso_key_t::request_backend_assigned_key_tag{});
  }

};

// </editor-fold> end key_traits<SSOKey> implementation
//==============================================================================

} // end namespace detail

//==============================================================================
// <editor-fold desc="Serializer specialization for SSOKey">


// </editor-fold> end Serializer specialization for SSOKey
//==============================================================================

} // end namespace darma_runtime



#endif //DARMA_IMPL_KEY_SSO_KEY_H
