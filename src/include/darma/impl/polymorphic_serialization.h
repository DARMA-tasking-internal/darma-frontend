/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_serialization.h
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

#ifndef DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H
#define DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H

#include <functional> // function
#include <memory> // unique_ptr
#include <vector>
#include <array>

#include <darma/interface/frontend/polymorphic_serializable_object.h>
#include <cassert>
#include <darma/impl/meta/detection.h>
#include <darma/impl/serialization/archive.h>
#include <darma/impl/serialization/manager.h>

//#ifndef DARMA_INITIAL_ABSTRACT_BASE_REGISTRY_SIZE
//#define DARMA_INITIAL_ABSTRACT_BASE_REGISTRY_SIZE 32
//#endif


namespace darma_runtime {
namespace detail {

// TODO this could be sped up a bit in specific cases where certain AbstractBase disallow multiple inheritance for derived types

template <typename AbstractBase>
using abstract_base_unpack_registry =
  std::vector<
    std::function<std::unique_ptr<AbstractBase>(char const* buffer, size_t size)>
  >;

template <typename=void>
size_t&
get_known_abstract_base_count() {
  static size_t abstract_base_count = 0;
  return abstract_base_count;
}

template <typename AbstractBase>
abstract_base_unpack_registry<AbstractBase>&
get_polymorphic_unpack_registry() {
  static abstract_base_unpack_registry<AbstractBase> _reg;
  return _reg;
}

namespace _impl {

template <typename T, typename AbstractBase>
using _has_static_unpack_as_archetype = decltype( T::template unpack_as<AbstractBase>(nullptr, 0ul) );

template <typename T, typename AbstractBase>
using _has_static_unpack_as = meta::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_unpack_as_archetype, T, AbstractBase
>;

template <typename T>
using _has_static_unpack_archetype = decltype( T::unpack(nullptr, 0ul) );

template <typename T, typename AbstractBase>
using _has_static_unpack = meta::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_unpack_archetype, T
>;

template <typename AbstractBase, typename ConcreteType, typename Enable=void>
struct PolymorphicUnpackRegistrar;

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrar<AbstractBase, ConcreteType,
  std::enable_if_t<_has_static_unpack_as<ConcreteType, AbstractBase>::value>
>{
  size_t index;
  PolymorphicUnpackRegistrar() {
    auto& reg = get_polymorphic_unpack_registry<AbstractBase>();
    index = reg.size();
    reg.emplace_back([](char const* buffer, size_t size) {
      return ConcreteType::template unpack_as<AbstractBase>(buffer, size);
    });
  }
};

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrar<AbstractBase, ConcreteType,
  std::enable_if_t<_has_static_unpack<ConcreteType, AbstractBase>::value>
>{
  size_t index;
  PolymorphicUnpackRegistrar() {
    auto& reg = get_polymorphic_unpack_registry<AbstractBase>();
    index = reg.size();
    reg.emplace_back([](char const* buffer, size_t size) {
      return ConcreteType::unpack(buffer, size);
    });
  }
};

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrarWrapper {
  static PolymorphicUnpackRegistrar<AbstractBase, ConcreteType> registrar;
};

template <typename AbstractBase, typename ConcreteType>
PolymorphicUnpackRegistrar<AbstractBase, ConcreteType>
PolymorphicUnpackRegistrarWrapper<AbstractBase, ConcreteType>::registrar = { };


template <typename AbstractBase>
struct AbstractBaseRegistrar {
  size_t index;
  AbstractBaseRegistrar() {
    auto& count = get_known_abstract_base_count();
    index = count;
    ++count;
  }
};

template <typename AbstractBase>
struct AbstractBaseRegistrarWrapper {
  static AbstractBaseRegistrar<AbstractBase> registrar;
};

template <typename AbstractBase>
AbstractBaseRegistrar<AbstractBase>
  AbstractBaseRegistrarWrapper<AbstractBase>::registrar = { };

} // end namespace _impl

template <typename AbstractType>
const size_t
get_abstract_type_index() {
  return _impl::AbstractBaseRegistrarWrapper<AbstractType>::registrar.index;
}

template <typename ConcreteType>
struct polymorphic_serialization_details {
  template <typename... AbstractBases>
  struct with_abstract_bases {
    static void
    add_registry_frontmatter_in_place(char* buffer) {
      new (buffer) uint8_t(static_cast<uint8_t>(sizeof...(AbstractBases)));
      char* off_buffer = buffer + sizeof(uint8_t);
      new (off_buffer) std::array<
        std::pair<size_t, size_t>, sizeof...(AbstractBases)
      >({
        std::make_pair(
          get_abstract_type_index<AbstractBases>(),
          _impl::PolymorphicUnpackRegistrarWrapper<
            AbstractBases,
            ConcreteType
          >::registrar.index
        )...
      });
    }
    static constexpr auto registry_frontmatter_size = sizeof(uint8_t)
      + sizeof(std::array<std::pair<size_t, size_t>, sizeof...(AbstractBases)>);
  };
};


template <typename ConcreteT, typename AbstractT>
struct PolymorphicSerializationAdapter : AbstractT {

  using polymorphic_details = typename
    polymorphic_serialization_details<ConcreteT>
      ::template with_abstract_bases<AbstractT>;


  size_t get_packed_size() const override {
    serialization::SimplePackUnpackArchive ar;
    using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
    ArchiveAccess::start_sizing(ar);
    serialization::Serializer<ConcreteT>().compute_size(
      *static_cast<ConcreteT const*>(this),
      ar
    );
    return ArchiveAccess::get_size(ar) + polymorphic_details::registry_frontmatter_size;
  }

  void pack(char* buffer) const override {
    polymorphic_details::add_registry_frontmatter_in_place(buffer);
    buffer += polymorphic_details::registry_frontmatter_size;
    serialization::SimplePackUnpackArchive ar;
    using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
    ArchiveAccess::start_packing_with_buffer(ar, buffer);
    serialization::Serializer<ConcreteT>().pack(
      *static_cast<ConcreteT const*>(this), ar
    );
  }

  static
  std::unique_ptr<AbstractT>
  unpack(char const* buffer, size_t size) {
    serialization::SimplePackUnpackArchive ar;
    using serialization::detail::DependencyHandle_attorneys::ArchiveAccess;
    ArchiveAccess::start_unpacking_with_buffer(ar, buffer);

    // TODO do allocation (and, consequently, deletion) through the backend
    void* allocated_spot = ::operator new(sizeof(ConcreteT));

    serialization::Serializer<ConcreteT>().unpack(
      allocated_spot, ar
    );
    std::unique_ptr<ConcreteT> rv(reinterpret_cast<ConcreteT*>(allocated_spot));

    return std::move(rv);
  }

};


} // end namespace detail

namespace abstract {
namespace frontend {

template <typename AbstractType>
std::unique_ptr<AbstractType>
PolymorphicSerializableObject<AbstractType>::unpack(char const* buffer, size_t size) {
  static const size_t abstract_type_index =
    darma_runtime::detail::get_abstract_type_index<AbstractType>();
  const uint8_t& n_bases = *reinterpret_cast<uint8_t const*>(buffer);
  buffer += sizeof(uint8_t);
  uint8_t i_base = 0;
  size_t concrete_index = std::numeric_limits<size_t>::max();
  for(; i_base < n_bases; ++i_base) {
    const auto& pair = *reinterpret_cast<std::pair<size_t, size_t> const*>(buffer);
    if (abstract_type_index == pair.first) {
      concrete_index = pair.second;
      break;
    }
    buffer += sizeof(std::pair<size_t, size_t>);
  }
  // TODO better error here
  assert(concrete_index != std::numeric_limits<size_t>::max());

  buffer += sizeof(std::pair<size_t, size_t>) * (n_bases - i_base);

  auto& reg = darma_runtime::detail::get_polymorphic_unpack_registry<AbstractType>();
  return reg[concrete_index](buffer, size - sizeof(uint8_t) - n_bases*sizeof(std::pair<size_t, size_t>));
}

} // end namespace frontend
} // end namespace abstract

} // end namespace darma_runtime


#endif //DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H
