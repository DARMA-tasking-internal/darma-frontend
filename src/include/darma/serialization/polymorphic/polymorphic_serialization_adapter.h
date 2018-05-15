/*
//@HEADER
// ************************************************************************
//
//                      polymorphic_serialization_adapter.h
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

#ifndef DARMAFRONTEND_POLYMORPHIC_SERIALIZATION_ADAPTER_H
#define DARMAFRONTEND_POLYMORPHIC_SERIALIZATION_ADAPTER_H

#include <darma/serialization/polymorphic/registry.h>
#include <darma/serialization/simple_handler.h>

#include <tinympl/variadic/find.hpp>

namespace darma {
namespace serialization {


//==============================================================================
// <editor-fold desc="PolymorphicSerializationAdapter"> {{{1

namespace detail {

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

} // end namespace detail

// Adapter for single abstract base
template <typename ConcreteT, typename AbstractT, typename BaseT = AbstractT,
  typename SerializationHandler =
    darma::serialization::PointerReferenceSerializationHandler<
      darma::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
    >
>
struct PolymorphicSerializationAdapter
  : detail::_polymorphic_serialization_adapter_impl<
      typename detail::polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<AbstractT>,
      BaseT,
      SerializationHandler
    >
{
  private:
    using serialization_handler_t = SerializationHandler;
    using impl_t = detail::_polymorphic_serialization_adapter_impl<
      typename detail::polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<AbstractT>,
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

      // TODO ask the abstract object for an allocator? Or allocate some other way?
      using allocator_t = std::allocator<ConcreteT>;
      using allocator_traits_t = std::allocator_traits<allocator_t>;
      allocator_t alloc;
      void* allocated_spot = std::allocator_traits<allocator_t>::allocate(alloc, 1);

      auto ar = serialization_handler_t::make_unpacking_archive(buffer);

      // call the customization point, allow ADL
      darma_unpack(
        darma::serialization::allocated_buffer_for<ConcreteT>(allocated_spot), ar
      );

      std::unique_ptr<ConcreteT> rv(reinterpret_cast<ConcreteT*>(allocated_spot));

      return std::move(rv);
    }
};

// Adapter for multiple abstract bases
template <typename ConcreteT, typename BaseT, typename... AbstractTypes>
struct PolymorphicSerializationAdapter<ConcreteT, tinympl::vector<AbstractTypes...>, BaseT,
  darma::serialization::PointerReferenceSerializationHandler<
    darma::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
  >
> : detail::_polymorphic_serialization_adapter_impl<
      typename detail::polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<AbstractTypes...>,
      BaseT,
      darma::serialization::PointerReferenceSerializationHandler<
        darma::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
      >
    >
{
  private:
    using serialization_handler_t = darma::serialization::PointerReferenceSerializationHandler<
      darma::serialization::SimpleSerializationHandler<std::allocator<ConcreteT>>
    >;
    using impl_t = detail::_polymorphic_serialization_adapter_impl<
      typename detail::polymorphic_serialization_details<ConcreteT>::template with_abstract_bases<
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

      // TODO ask the abstract object for an allocator? Or allocate some other way?
      using allocator_t = std::allocator<ConcreteT>;
      using allocator_traits_t = std::allocator_traits<allocator_t>;
      allocator_t alloc;
      void* allocated_spot = std::allocator_traits<allocator_t>::allocate(alloc, 1);

      auto ar = serialization_handler_t::make_unpacking_archive(buffer);

      darma_unpack(
        darma::serialization::allocated_buffer_for<ConcreteT>(allocated_spot), ar
      );

      std::unique_ptr<ConcreteT> rv(reinterpret_cast<ConcreteT*>(allocated_spot));

      return std::move(rv);
    }
};

// </editor-fold> end PolymorphicSerializationAdapter }}}1
//==============================================================================

} // end namespace serialization
} // end namespace darma

#endif //DARMAFRONTEND_POLYMORPHIC_SERIALIZATION_ADAPTER_H
