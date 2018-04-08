/*
//@HEADER
// ************************************************************************
//
//                      registry.h
//                         DARMA
//              Copyright (C) 2018 Sandia Corporation
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

#ifndef DARMAFRONTEND_SERIALIZATION_POLYMORPHIC_REGISTRY_H
#define DARMAFRONTEND_SERIALIZATION_POLYMORPHIC_REGISTRY_H

#include <tinympl/detection.hpp>
#include <tinympl/select.hpp>

#include <functional> // function
#include <vector>
#include <cstdint>
#include <memory>

namespace darma_runtime {
namespace serialization {
namespace detail {

// TODO this could be sped up a bit in specific cases where certain AbstractBase disallow multiple inheritance for derived types

template <typename AbstractBase>
using abstract_base_unpack_registry =
  std::vector<
    std::function<std::unique_ptr<AbstractBase>(char const*& buffer)>
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

struct SerializedPolymorphicObjectHeader {
  size_t n_bases;
  // Pad to 128 bits for now to make alignment work out reasonably in the most
  // common case, where there's only one base
  size_t _unused1;
  size_t _unused2;
};

struct PolymorphicAbstractBasesTableEntry {
  size_t abstract_index;
  size_t concrete_index;
};

namespace _impl {

//==============================================================================
// <editor-fold desc="PolymorphicUnpackRegistrar"> {{{1

template <typename AbstractBase, typename ConcreteType, typename Enable=void>
struct PolymorphicUnpackRegistrar;

//------------------------------------------------------------------------------
// <editor-fold desc="static unpack detection"> {{{2

// Use longer names in the adapters to avoid collisions with user-defined functions named "unpack"
template <typename T, typename AbstractBase>
using _has_static_long_name_unpack_as_archetype = decltype(
  T::template _darma_static_polymorphic_serializable_adapter_unpack_as<AbstractBase>(std::declval<char const*&>())
);

template <typename T, typename AbstractBase>
using _has_static_long_name_unpack_as = tinympl::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_long_name_unpack_as_archetype, T, AbstractBase
>;

template <typename T>
using _has_static_long_name_unpack_archetype = decltype(
  T::_darma_static_polymorphic_serializable_adapter_unpack(std::declval<char const*&>())
);

template <typename T, typename AbstractBase>
using _has_static_long_name_unpack = tinympl::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_long_name_unpack_archetype, T
>;

template <typename T, typename AbstractBase>
using _has_static_unpack_as_archetype = decltype( T::template unpack_as<AbstractBase>(std::declval<char const*&>()) );

template <typename T, typename AbstractBase>
using _has_static_unpack_as = tinympl::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_unpack_as_archetype, T, AbstractBase
>;

template <typename T>
using _has_static_unpack_archetype = decltype( T::unpack(std::declval<char const*&>()) );

template <typename T, typename AbstractBase>
using _has_static_unpack = tinympl::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_unpack_archetype, T
>;

enum struct _static_unpack_hook_option {
  long_name_unpack_as,
  long_name_unpack,
  unpack_as,
  unpack
};

template <_static_unpack_hook_option val>
using _wrapped_hook = std::integral_constant<_static_unpack_hook_option, val>;

template <_static_unpack_hook_option option, typename T, typename AbstractBase>
using _enable_if_static_unpack_hook_case = std::enable_if_t<
  tinympl::select_first<
    _has_static_long_name_unpack_as<T, AbstractBase>,
    /* => */ _wrapped_hook<_static_unpack_hook_option::long_name_unpack_as>,
    _has_static_long_name_unpack<T, AbstractBase>,
    /* => */ _wrapped_hook<_static_unpack_hook_option::long_name_unpack>,
    _has_static_unpack_as<T, AbstractBase>,
    /* => */ _wrapped_hook<_static_unpack_hook_option::unpack_as>,
    _has_static_unpack<T, AbstractBase>,
    /* => */ _wrapped_hook<_static_unpack_hook_option::unpack>
  >::type::value == option
>;

// </editor-fold> end static unpack detection }}}2
//------------------------------------------------------------------------------

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrar<AbstractBase, ConcreteType,
  _enable_if_static_unpack_hook_case<
    _static_unpack_hook_option::unpack_as,
    ConcreteType, AbstractBase
  >
>{
  size_t index;
  PolymorphicUnpackRegistrar() {
    auto& reg = get_polymorphic_unpack_registry<AbstractBase>();
    index = reg.size();
    reg.emplace_back([](char const*& buffer) {
      return ConcreteType::template unpack_as<AbstractBase>(buffer);
    });
  }
};

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrar<AbstractBase, ConcreteType,
  _enable_if_static_unpack_hook_case<
    _static_unpack_hook_option::unpack,
    ConcreteType, AbstractBase
  >
>{
  size_t index;
  PolymorphicUnpackRegistrar() {
    auto& reg = get_polymorphic_unpack_registry<AbstractBase>();
    index = reg.size();
    reg.emplace_back([](char const*& buffer) {
      return ConcreteType::unpack(buffer);
    });
  }
};

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrar<AbstractBase, ConcreteType,
  _enable_if_static_unpack_hook_case<
    _static_unpack_hook_option::long_name_unpack_as,
    ConcreteType, AbstractBase
  >
>{
  size_t index;
  PolymorphicUnpackRegistrar() {
    auto& reg = get_polymorphic_unpack_registry<AbstractBase>();
    index = reg.size();
    reg.emplace_back([](char const*& buffer) {
      return ConcreteType::template _darma_static_polymorphic_serializable_adapter_unpack_as<AbstractBase>(buffer);
    });
  }
};

template <typename AbstractBase, typename ConcreteType>
struct PolymorphicUnpackRegistrar<AbstractBase, ConcreteType,
  _enable_if_static_unpack_hook_case<
    _static_unpack_hook_option::long_name_unpack,
    ConcreteType, AbstractBase
  >
>{
  size_t index;
  PolymorphicUnpackRegistrar() {
    auto& reg = get_polymorphic_unpack_registry<AbstractBase>();
    index = reg.size();
    reg.emplace_back([](char const*& buffer) {
      return ConcreteType::_darma_static_polymorphic_serializable_adapter_unpack(buffer);
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

// </editor-fold> end PolymorphicUnpackRegistrar }}}1
//==============================================================================


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
size_t
get_abstract_type_index() {
  return _impl::AbstractBaseRegistrarWrapper<AbstractType>::registrar.index;
}


} // end namespace detail
} // end namespace serialization
} // end namespace darma_runtime

#endif //DARMAFRONTEND_SERIALIZATION_POLYMORPHIC_REGISTRY_H
