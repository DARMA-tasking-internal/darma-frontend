/*
//@HEADER
// ************************************************************************
//
//                      simple_key.h
//                         DARMA
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

#ifndef DARMA_IMPL_KEY_SIMPLE_KEY_H_H
#define DARMA_IMPL_KEY_SIMPLE_KEY_H_H

#include <darma/impl/meta/tuple_for_each.h>
#include <darma/impl/meta/splat_tuple.h>

#include <darma/impl/key/bytes_convert.h>
#include <darma/impl/key/raw_bytes.h>

#include <darma/impl/serialization/serialization_fwd.h>
#include <darma/impl/serialization/nonintrusive.h>

#include <darma/impl/key_concept.h>
#include <darma/impl/darma_assert.h>

#include "simple_key_fwd.h"

namespace darma_runtime {

namespace serialization {

template <> struct Serializer<darma_runtime::detail::SimpleKey>;

} // end namespace serialization

namespace detail {

namespace _simple_key_impl {

template <typename TDeduced, typename Enable=void>
struct add_arg {
  void operator()(
    TDeduced&& arg,
    std::vector<raw_bytes>& components, std::vector<bytes_type_metadata>& types
  ) const {
    bytes_convert<std::remove_cv_t<std::remove_reference_t<TDeduced>>> bc;
    size_t size = bc.get_size(arg);
    components.emplace_back(size);
    types.emplace_back();
    bc.get_bytes_type_metadata(&(types.back()), arg);
    bc(std::forward<TDeduced>(arg), components.back().data.get(), size, 0);
  }
};


struct _traits_impl;

} // end namespace _simple_key_impl


class SimpleKey {

  protected:

    static constexpr uint8_t not_given = std::numeric_limits<uint8_t>::max();
    static constexpr uint8_t max_num_parts = std::numeric_limits<uint8_t>::max() - (uint8_t)1;

    class SimpleKeyComponent {
      public:

        SimpleKeyComponent(raw_bytes const& comp, bytes_type_metadata const& type)
          : component_(comp), type_(type)
        { }

        template <typename T>
        T as() && {
          return bytes_convert<T>().get_value(
            &type_,
            component_.data.get(), component_.get_size()
          );
        }


      private:
        raw_bytes const& component_;
        bytes_type_metadata const& type_;

    };

    template <typename... Args>
    SimpleKey(variadic_constructor_arg_t const, Args&&... args);


  public:

    SimpleKey() = default;

    template <uint8_t N=not_given>
    SimpleKeyComponent
    component(size_t N_dynamic=not_given) const {
      assert(N_dynamic == not_given || N_dynamic < (size_t)max_num_parts);
      static_assert(N == not_given || N < max_num_parts,
        "tried to get key component greater than max_num_parts");
      assert(N == not_given xor N_dynamic == not_given);
      const uint8_t actual_N = N == not_given ? (uint8_t)N_dynamic : N;
      DARMA_ASSERT_RELATED_VERBOSE((int)actual_N, <, n_components());
      return { components_.at(actual_N), types_.at(actual_N) };
    }

    size_t n_components() const { return components_.size() - has_internal_last_component; }

    SimpleKey
    subkey(size_t first_component, size_t end, bool propagate_internal_data = true) const & {
      SimpleKey rv;
      if(end <= first_component) return rv;
      assert(end <= n_components());
      assert(first_component < n_components());
      bool do_last_componenet = propagate_internal_data and has_internal_last_component;
      rv.components_.reserve(end - first_component + do_last_componenet);
      std::copy(
        components_.begin() + first_component,
        components_.begin() + end,
        std::back_inserter(rv.components_)
      );
      rv.types_.reserve(end - first_component + do_last_componenet);
      std::copy(
        types_.begin() + first_component,
        types_.begin() + end,
        std::back_inserter(rv.types_)
      );
      if(do_last_componenet) {
        rv.components_.push_back(components_.back());
        rv.types_.push_back(types_.back());
      }
      rv.has_internal_last_component = propagate_internal_data;
      return rv;
    }

    SimpleKey&&
    subkey(size_t first_component, size_t end, bool propagate_internal_data = true) && {
      SimpleKey rv;
      if(end <= first_component) return std::move(rv);
      assert(end <= n_components());
      assert(first_component < n_components());
      bool do_last_componenet = propagate_internal_data and has_internal_last_component;
      rv.components_.reserve(end - first_component + do_last_componenet);
      std::move(
        components_.begin() + first_component,
        components_.begin() + end,
        std::back_inserter(rv.components_)
      );
      rv.types_.reserve(end - first_component + do_last_componenet);
      std::move(
        types_.begin() + first_component,
        types_.begin() + end,
        std::back_inserter(rv.types_)
      );
      if(do_last_componenet) {
        rv.components_.emplace_back(std::move(components_.back()));
        rv.types_.push_back(std::move(types_.back()));
      }
      rv.has_internal_last_component = propagate_internal_data;
      return std::move(rv);
    }


    std::string
    human_readable_string(const char* sep = ", ",
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
      // TODO this should work for empty keys
      for(size_t i = 0; i < n_components(); ++i) {
        const auto* md = &(types_[i]);
        if(md->is_string_like) {
          o << component(i).as<std::string>();
        }
        else if(md->is_int_like_type) {
          if(not reinterpret_cast<const int_like_type_metadata*>(md)->is_enumerated) {
            o << component(i).as<intmax_t>();
          }
          else {
            o << "<enum>";
          }
        }
        else if(md->is_floating_point_like_type) {
          o << component(i).as<double>();
        }
        else {
          assert(false); // not implemented
        }

        if(i != n_components()) o << sep;
      }

      assert(not has_internal_last_component); // not implemented

    }

  private:

    std::vector<raw_bytes> components_;
    std::vector<bytes_type_metadata> types_;
    bool has_internal_last_component = false;

    friend struct _simple_key_impl::_traits_impl;
    // This is ugly, but...
    friend struct _simple_key_impl::add_arg<SimpleKey>;
    friend struct _simple_key_impl::add_arg<SimpleKey&>;
    friend struct _simple_key_impl::add_arg<const SimpleKey>;
    friend struct _simple_key_impl::add_arg<const SimpleKey&>;
    friend struct _simple_key_impl::add_arg<SimpleKey&&>;

    friend struct serialization::Serializer<SimpleKey>;

};


namespace _simple_key_impl {

template <typename SimpleKeyDeduced>
struct add_arg<SimpleKeyDeduced,
  std::enable_if_t<std::is_same<std::decay_t<SimpleKeyDeduced>, SimpleKey>::value>
>{
  // rvalue overload
  void operator()(
    std::remove_reference_t<SimpleKeyDeduced>&& arg, std::vector<raw_bytes>& components,
    std::vector<bytes_type_metadata>& types
  ) const {
    for(auto&& part : arg.components_) {
      components.emplace_back(std::move(part));
    }
    types = std::move(arg.types_);
  }

  // lvalue overload (need to create new and memcopy data)
  void operator()(
    std::remove_reference_t<SimpleKeyDeduced> const& arg, std::vector<raw_bytes>& components,
    std::vector<bytes_type_metadata>& types
  ) const {
    int i = 0;
    for(auto const& part : arg.components_) {
      components.emplace_back(part.get_size());
      ::memcpy(components.back().data.get(), part.data.get(), part.get_size());
    }
    types = arg.types_;
  }
};

} // end namespace _simple_key_impl


template <typename... Args>
SimpleKey::SimpleKey(variadic_constructor_arg_t const, Args&&... args) {
  // We can't just do add_arg<Args>()(std::forward<Args>(args), components_)... because
  // argument evaluation order isn't guaranteed
  meta::tuple_for_each(
    std::forward_as_tuple(std::forward<Args>(args)...),
    [this](auto&& arg) {
      _simple_key_impl::add_arg<decltype(arg)>()(
        std::forward<decltype(arg)>(arg), components_, types_
      );
    }
  );
}


namespace _simple_key_impl {

struct _traits_impl {
  template <typename... Args>
  static SimpleKey
  make(Args&&... args) {
    return SimpleKey(variadic_constructor_arg, std::forward<Args>(args)...);
  }

  template <typename Tuple>
  static SimpleKey
  make_from_tuple(Tuple&& tup) {
    return meta::splat_tuple(std::forward<Tuple>(tup),
      [](auto&&... args) {
        return SimpleKey(variadic_constructor_arg, std::forward<decltype(args)>(args)...);
      }
    );
  }

  static bool
  do_key_equal(SimpleKey const& k1, SimpleKey const& k2) {
    if(k1.n_components() != k2.n_components()) return false;
    for(size_t i = 0; i < k1.n_components(); ++i) {
      const raw_bytes& c1 = k1.components_.at(i);
      const raw_bytes& c2 = k2.components_.at(i);
      if(not equal_as_bytes(c1.data.get(), c1.get_size(), c2.data.get(), c2.get_size())) {
        return false;
      }
      if(not equal_as_bytes(k1.types_.at(i), k2.types_.at(i))) {
        return false;
      }
    }
    return true;
  }

  static size_t
  do_key_hash(SimpleKey const& k) {
    size_t rv = 0;
    for(auto const& c : k.components_) {
      hash_combine(rv, hash_as_bytes(c.data.get(), c.get_size()));
    }
    for(auto const& t : k.types_) {
      hash_combine(rv, hash_as_bytes(t));
    }
    return rv;
  }

  template <typename T>
  static SimpleKey
  add_internal_last_component(
    SimpleKey const& k, T const& to_add
  ) {
    assert(!k.has_internal_last_component);
    static_assert(not std::is_same<T, SimpleKey>::value, "Can't use SimpleKey as internal last component");
    auto rv = make(k, to_add);
    rv.has_internal_last_component = true;
    return rv;
  }

  static SimpleKey
  without_internal_last_component(
    SimpleKey const& k
  ) {
    assert(k.has_internal_last_component);
    SimpleKey rv;
    // Note that n_components() doesn't include the internal component, so this just works
    for(int i = 0; i < k.n_components(); ++i) {
      rv.components_.emplace_back(k.components_[i].get_size());
      memcpy(rv.components_.back().data.get(), k.components_[i].data.get(), k.components_[i].get_size());
      rv.types_.push_back(k.types_[i]);
    }
    rv.has_internal_last_component = false;
    return rv;
  }

  static SimpleKey::SimpleKeyComponent
  get_internal_last_component(SimpleKey const& k) {
    assert(k.has_internal_last_component);
    return SimpleKey::SimpleKeyComponent( k.components_.back(), k.types_.back() );
  }

  static bool
  has_internal_last_component(SimpleKey const& k) {
    return k.has_internal_last_component;
  }

};

} // end namespace _simple_key_impl


template <>
struct key_traits<SimpleKey> {
  struct maker {
    template <typename... Args>
    inline SimpleKey
    operator()(Args&&... args) const {
      return _simple_key_impl::_traits_impl::make(std::forward<Args>(args)...);
    }
  };
  struct maker_from_tuple {
    template <typename Tuple>
    inline SimpleKey
    operator()(Tuple&& tup) const {
      return _simple_key_impl::_traits_impl::make_from_tuple(std::forward<Tuple>(tup));
    }
  };
  struct key_equal {
    inline bool
    operator()(SimpleKey const& k1, SimpleKey const& k2) const {
      return _simple_key_impl::_traits_impl::do_key_equal(k1, k2);
    }
  };
  struct hasher {
    inline size_t
    operator()(SimpleKey const& k) const {
      return _simple_key_impl::_traits_impl::do_key_hash(k);
    }
  };

  typedef _simple_key_impl::_traits_impl internal_use_access;
};



} // end namespace detail

namespace serialization {

template <>
struct Serializer<darma_runtime::detail::SimpleKey> {
  template <typename ArchiveT>
  void compute_size(darma_runtime::detail::SimpleKey const& val, ArchiveT& ar) const {
    ar % (size_t)val.types_.size();
    for(auto&& t : val.types_) ar % t;
    for(auto&& c : val.components_) {
      ar % c.get_size();
      Serializer_attorneys::ArchiveAccess::spot(ar) += c.get_size();
    }
  }
  template <typename ArchiveT>
  void pack(darma_runtime::detail::SimpleKey const& val, ArchiveT& ar) const {
    ar << static_cast<size_t>(val.types_.size());
    for(auto&& t : val.types_) ar << t;
    for(auto&& c : val.components_) {
      ar << c.get_size();
      memcpy(
        Serializer_attorneys::ArchiveAccess::spot(ar),
        c.data.get(),
        c.get_size()
      );
      Serializer_attorneys::ArchiveAccess::spot(ar) += c.get_size();
    }
  }
  template <typename ArchiveT>
  void unpack(void* allocated, ArchiveT& ar) const {
    auto* val = new (allocated) darma_runtime::detail::SimpleKey;
    size_t ncomp = 0;
    ar >> ncomp;
    val->types_.reserve(ncomp);
    for(size_t i = 0; i < ncomp; ++i) {
      val->types_.emplace_back();
      ar >> val->types_.back();
    }
    val->components_.reserve(ncomp);
    for(size_t i = 0; i < ncomp; ++i) {
      size_t size = 0;
      ar >> size;
      val->components_.emplace_back(size);
      memcpy(
        val->components_.back().data.get(),
        Serializer_attorneys::ArchiveAccess::spot(ar),
        size
      );
      Serializer_attorneys::ArchiveAccess::spot(ar) += size;
    }
  }
};

} // end namespace serialization

} // end namespace darma_runtime

STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(darma_runtime::detail::SimpleKey,
  darma_runtime::serialization::SimplePackUnpackArchive,
  "SimpleKey must be serializable"
)
STATIC_ASSERT_SERIALIZABLE_WITH_ARCHIVE(darma_runtime::detail::bytes_type_metadata,
  darma_runtime::serialization::SimplePackUnpackArchive,
  "bytes_type_metadata must be serializable"
)


#endif //DARMA_IMPL_KEY_SIMPLE_KEY_H_H
