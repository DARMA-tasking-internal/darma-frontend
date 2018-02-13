/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_serialization.h
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

#ifndef DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H
#define DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H

#include <functional> // function
#include <memory> // unique_ptr
#include <vector>
#include <array>

#include <tinympl/variadic/find.hpp>

#include <darma/interface/frontend/polymorphic_serializable_object.h>
#include <cassert>
#include <darma/impl/meta/detection.h>
#include <darma/impl/serialization/manager.h>

#include <darma/interface/backend/runtime.h>

//#ifndef DARMA_INITIAL_ABSTRACT_BASE_REGISTRY_SIZE
//#define DARMA_INITIAL_ABSTRACT_BASE_REGISTRY_SIZE 32
//#endif


namespace darma_runtime {
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
  size_t n_bases : 8;
  // Pad to 128 bits for now to make alignment work out reasonably in the most
  // common case, where there's only one base
  size_t _unused1 : 56;
  size_t _unused2 : 64;
};

struct PolymorphicAbstractBasesTableEntry {
  size_t abstract_index : 64;
  size_t concrete_index : 64;
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
using _has_static_long_name_unpack_as = meta::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_long_name_unpack_as_archetype, T, AbstractBase
>;

template <typename T>
using _has_static_long_name_unpack_archetype = decltype(
  T::_darma_static_polymorphic_serializable_adapter_unpack(std::declval<char const*&>())
);

template <typename T, typename AbstractBase>
using _has_static_long_name_unpack = meta::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_long_name_unpack_archetype, T
>;

template <typename T, typename AbstractBase>
using _has_static_unpack_as_archetype = decltype( T::template unpack_as<AbstractBase>(std::declval<char const*&>()) );

template <typename T, typename AbstractBase>
using _has_static_unpack_as = meta::is_detected_convertible<
  std::unique_ptr<AbstractBase>, _has_static_unpack_as_archetype, T, AbstractBase
>;

template <typename T>
using _has_static_unpack_archetype = decltype( T::unpack(std::declval<char const*&>()) );

template <typename T, typename AbstractBase>
using _has_static_unpack = meta::is_detected_convertible<
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

template <typename ConcreteType>
struct polymorphic_serialization_details {
  template <typename... AbstractBases>
  struct with_abstract_bases {
    static void
    add_registry_frontmatter_in_place(char* buffer) {
      // add the header
      new (buffer) SerializedPolymorphicObjectHeader{
        /* n_bases = */ static_cast<uint8_t>(sizeof...(AbstractBases)),
        /* unused */ 0ull,
        /* also unused */ 0ull
      };
      // advance the buffer
      buffer += sizeof(SerializedPolymorphicObjectHeader);

      // add the array of abstract-to-concrete index pairs
      new (buffer) PolymorphicAbstractBasesTableEntry[sizeof...(AbstractBases)] {
        PolymorphicAbstractBasesTableEntry{
          get_abstract_type_index<AbstractBases>(),
          _impl::PolymorphicUnpackRegistrarWrapper<
            AbstractBases,
            ConcreteType
          >::registrar.index
        }...
      };
    }
    static constexpr auto registry_frontmatter_size =
      sizeof(SerializedPolymorphicObjectHeader)
        + sizeof(PolymorphicAbstractBasesTableEntry[sizeof...(AbstractBases)]);
    using concrete_t = ConcreteType;
  };
};

template <typename Details, typename BaseT, typename SerializationHandler>
struct _polymorphic_serialization_adapter_impl : BaseT {

  private:
    using polymorphic_details_t = Details;
    using concrete_t = typename Details::concrete_t;
    using serialization_handler_t = SerializationHandler;

  protected:
    // Perfect forwarding ctor
    template <typename... Args>
    _polymorphic_serialization_adapter_impl(
      Args&&... args
    ) : BaseT(std::forward<Args>(args)...)
    { }

  public:

    size_t get_packed_size() const override {
      auto ar = serialization_handler_t::make_sizing_archive();
      // call the customization point, allow ADL
      darma_compute_size(*static_cast<concrete_t const*>(this), ar);
      return serialization_handler_t::get_size(ar) + polymorphic_details_t::registry_frontmatter_size;
    }

    void pack(char*& buffer) const override {
      polymorphic_details_t::add_registry_frontmatter_in_place(buffer);
      buffer += polymorphic_details_t::registry_frontmatter_size;
      auto ar = serialization_handler_t::make_packing_archive(
        buffer
      );
      // call the customization point, allow ADL
      darma_pack(*static_cast<concrete_t const*>(this), ar);
    }

};

//==============================================================================
// <editor-fold desc="PolymorphicSerializationAdapter"> {{{1

// Adapter for single abstract base
template <typename ConcreteT, typename AbstractT, typename BaseT = AbstractT,
  typename SerializationHandler =
    darma_runtime::serialization::PointerReferenceSerializationHandler<
      darma_runtime::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
    >
>
struct PolymorphicSerializationAdapter
  : _polymorphic_serialization_adapter_impl<
      typename polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<AbstractT>,
      BaseT,
      SerializationHandler
    >
{
  private:
    using serialization_handler_t = SerializationHandler;
    using impl_t = _polymorphic_serialization_adapter_impl<
      typename polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<AbstractT>,
      BaseT,
      serialization_handler_t
    >;

  protected:

    template <typename... Args>
    PolymorphicSerializationAdapter(
      Args&&... args
    ) : impl_t(std::forward<Args>(args)...)
    { }

  public:

    static
    std::unique_ptr<AbstractT>
    _darma_static_polymorphic_serializable_adapter_unpack(char const*& buffer) {

      // TODO ask the abstract object for an allocator?
      // There's no way to use make_unique here; we just have to allocate and then
      // construct the unique_ptr from the pointer we allocate
      void* allocated_spot = darma_runtime::abstract::backend::get_backend_memory_manager()
        ->allocate(sizeof(ConcreteT));

      auto ar = serialization_handler_t::make_unpacking_archive(buffer);

      // call the customization point, allow ADL
      darma_unpack(
        darma_runtime::serialization::allocated_buffer_for<ConcreteT>(allocated_spot), ar
      );

      std::unique_ptr<ConcreteT> rv(reinterpret_cast<ConcreteT*>(allocated_spot));

      return std::move(rv);
    }
};

// Adapter for multiple abstract bases
template <typename ConcreteT, typename BaseT, typename... AbstractTypes>
struct PolymorphicSerializationAdapter<ConcreteT, tinympl::vector<AbstractTypes...>, BaseT,
  darma_runtime::serialization::PointerReferenceSerializationHandler<
    darma_runtime::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
  >
> : _polymorphic_serialization_adapter_impl<
      typename polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<AbstractTypes...>,
      BaseT,
      darma_runtime::serialization::PointerReferenceSerializationHandler<
        darma_runtime::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
      >
    >
{
  private:
    using serialization_handler_t = darma_runtime::serialization::PointerReferenceSerializationHandler<
      darma_runtime::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
    >;
    using impl_t = _polymorphic_serialization_adapter_impl<
      typename polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<
        AbstractTypes...
      >,
      BaseT, serialization_handler_t
    >;

  protected:

    template <typename... Args>
    PolymorphicSerializationAdapter(
      Args&&... args
    ) : impl_t(std::forward<Args>(args)...)
    { }

  public:

    template <
      typename AbstractT,
      typename=std::enable_if_t<
        tinympl::variadic::find<AbstractT, AbstractTypes...>::value != sizeof...(AbstractTypes)
      >
    >
    static
    std::unique_ptr<AbstractT>
    _darma_static_polymorphic_serializable_adapter_unpack_as(char const*& buffer) {

      // TODO ask the abstract object for an allocator?
      // There's no way to use make_unique here; we just have to allocate and then
      // construct the unique_ptr from the pointer we allocate
      void* allocated_spot = darma_runtime::abstract::backend::get_backend_memory_manager()
        ->allocate(sizeof(ConcreteT));

      auto ar = serialization_handler_t::make_unpacking_archive(buffer);

      darma_unpack(
        darma_runtime::serialization::allocated_buffer_for<ConcreteT>(allocated_spot), ar
      );

      std::unique_ptr<ConcreteT> rv(reinterpret_cast<ConcreteT*>(allocated_spot));

      return std::move(rv);
    }
};

// </editor-fold> end PolymorphicSerializationAdapter }}}1
//==============================================================================

} // end namespace detail

namespace abstract {
namespace frontend {

template <typename AbstractType>
std::unique_ptr<AbstractType>
PolymorphicSerializableObject<AbstractType>::unpack(char const*& buffer) {
  // Get the abstract type index that we're looking for
  static const size_t abstract_type_index =
    darma_runtime::detail::get_abstract_type_index<AbstractType>();

  // Get the header
  auto const& header = *reinterpret_cast<
    darma_runtime::detail::SerializedPolymorphicObjectHeader const*
  >(buffer);
  buffer += sizeof(darma_runtime::detail::SerializedPolymorphicObjectHeader);

  // Look through the abstract bases that this object is registered for until
  // we find the entry that corresponds to a callable that unpacks the object
  // as a std::unique_ptr<AbstractType>
  uint8_t i_base = 0;
  size_t concrete_index = std::numeric_limits<size_t>::max();
  for(; i_base < header.n_bases; ++i_base) {
    const auto& entry = *reinterpret_cast<
      darma_runtime::detail::PolymorphicAbstractBasesTableEntry const*
    >(buffer);

    if (abstract_type_index == entry.abstract_index) {
      // We found the one we want to call, so break
      concrete_index = entry.concrete_index;
      break;
    }

    buffer += sizeof(darma_runtime::detail::PolymorphicAbstractBasesTableEntry);
  }
  DARMA_ASSERT_MESSAGE(
    concrete_index != std::numeric_limits<size_t>::max(),
    "No registered unpacker found to unpack a concrete type as abstract type "
      << darma_runtime::utility::try_demangle<AbstractType>::name()
  );

  // Make sure we advance the buffer over all of the other base class entries
  buffer += sizeof(darma_runtime::detail::PolymorphicAbstractBasesTableEntry) * (header.n_bases - i_base);

  // get a reference to the static registry
  auto& reg = darma_runtime::detail::get_polymorphic_unpack_registry<AbstractType>();

  // execute the unpack callable on the buffer
  return reg[concrete_index](buffer);
}

} // end namespace frontend
} // end namespace abstract

} // end namespace darma_runtime


#endif //DARMA_IMPL_POLYMORPHIC_SERIALIZATION_H
