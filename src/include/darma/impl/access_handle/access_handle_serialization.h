/*
//@HEADER
// ************************************************************************
//
//                      access_handle_serialization.h
//                         DARMA
//              Copyright (C) 2017 Sandia Corporation
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

#ifndef DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H
#define DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H

#include <darma/impl/feature_testing_macros.h>

#include <darma/interface/app/access_handle.h>
#include <darma/serialization/pointer_reference_archive.h>
#include <darma/serialization/pointer_reference_handler.h>

#include <darma/impl/compatibility.h>

namespace darma_runtime {

//==============================================================================
// <editor-fold desc="Serialization"> {{{1

#if _darma_has_feature(task_migration)
namespace serialization {

template <>
struct Serializer<types::flow_t> {

  using serialization_handler_t = SimpleSerializationHandler<std::allocator<types::flow_t>>;

  static void compute_size(types::flow_t const& obj, SimpleSizingArchive& ar) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    auto size = backend_runtime->get_packed_flow_size(obj);
    ar.add_to_size_raw(size);
  }

  template <typename SerializationBuffer>
  static void pack(
    types::flow_t const& obj, PointerReferencePackingArchive<SerializationBuffer>& ar
  ) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    backend_runtime->pack_flow(
      const_cast<types::flow_t&>(obj),
      ar.data_pointer_reference()
    );
  }

  template <typename Allocator>
  static void unpack(void* allocated, PointerReferenceUnpackingArchive<Allocator>& ar) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    new (allocated) types::flow_t{
      backend_runtime->make_unpacked_flow(ar.data_pointer_reference())
    };
  }
};

template <>
struct Serializer<types::anti_flow_t> {

  using serialization_handler_t = SimpleSerializationHandler<std::allocator<types::anti_flow_t>>;

  static void compute_size(types::anti_flow_t const& obj, SimpleSizingArchive& ar) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    auto size = backend_runtime->get_packed_anti_flow_size(obj);
    ar.add_to_size_raw(size);
  }

  template <typename SerializationBuffer>
  static void pack(types::anti_flow_t const& obj, PointerReferencePackingArchive<SerializationBuffer>& ar) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    backend_runtime->pack_anti_flow(
      const_cast<types::anti_flow_t&>(obj),
      ar.data_pointer_reference()
    );
  }

  template <typename Allocator>
  static void unpack(void* allocated, PointerReferenceUnpackingArchive<Allocator>& ar) {
    auto* backend_runtime = abstract::backend::get_backend_runtime();
    new (allocated) types::anti_flow_t{
      backend_runtime->make_unpacked_anti_flow(ar.data_pointer_reference())
    };
  }
};


// TODO if we add operator==() to the requirements of flow_t, we don't have to pack the outflow when it's the same as the inflow
template <typename... Args>
struct Serializer<AccessHandle<Args...>>
{
    // TODO update this for all of the special members now present in AccessHandle
  private:
    using AccessHandleT = AccessHandle<Args...>;

    static bool handle_is_serializable_assertions(AccessHandleT const& val)
    {
      // The handle has to be set up and valid
      assert(val.var_handle_base_.get() != nullptr);
      // Suspended out flow shouldn't exist
      assert(val.get_current_use()->use()->suspended_out_flow_ == nullptr);
      return true;
    }

  public:
    template <typename ArchiveT>
    static void compute_size(AccessHandleT const& val, ArchiveT& ar) {
      ar | val.var_handle_base_->get_key();
      ar.add_to_size_raw(val.current_use_base_->use_base->get_packed_size());
    }

    template <typename ArchiveT>
    static void pack(
      AccessHandleT const& val,
      ArchiveT& ar,
      std::enable_if_t<
        is_packable_with_archive<types::flow_t, ArchiveT>::value
        and is_packable_with_archive<types::anti_flow_t, ArchiveT>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    ) {
      ar | val.var_handle_base_->get_key();
      auto ptr_ar = PointerReferenceSerializationHandler<>::make_packing_archive_referencing(ar);
      val.current_use_base_->use_base->pack(*reinterpret_cast<char**>(&ptr_ar.data_pointer_reference()));
    }

    template <typename ConvertiblePackingArchive>
    _darma_requires(requires(ConvertiblePackingArchive a) {
      PointerReferenceSerializationHandler<>::make_packing_archive(ar);
    })
    static void pack(
      AccessHandleT const& val,
      ConvertiblePackingArchive& ar,
      std::enable_if_t<
        not is_packable_with_archive<types::flow_t, ConvertiblePackingArchive>::value
          or not is_packable_with_archive<types::anti_flow_t, ConvertiblePackingArchive>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    )
    {
      // Packing flows uses pointer references, so we need to convert to an archive
      // that exposes the pointer (unless the archive type can serialize flows directly
      auto ptr_ar = PointerReferenceSerializationHandler<>::make_unpacking_archive_referencing(ar);
      // need to assert this to avoid infinite recursion
      static_assert(serialization::is_packable_with_archive<
        types::flow_t, std::decay_t<decltype(ptr_ar)>
      >::value, "Don't know how to pack flows");
      static_assert(serialization::is_packable_with_archive<
        types::anti_flow_t, std::decay_t<decltype(ptr_ar)>
      >::value, "Don't know how to pack anti-flows");
      ptr_ar << val;
    }

    template <
      typename ArchiveT,
      typename=std::enable_if_t<
        is_unpackable_with_archive<types::flow_t, ArchiveT>::value
          and is_unpackable_with_archive<types::anti_flow_t, ArchiveT>::value
      >
    >
    static void unpack(void* allocated, ArchiveT& ar)
    {
      // Call an unpacking constructor
      auto ah_ptr = new (allocated) AccessHandleT{};
      ah_ptr->unpack_from_archive(ar);
    }

    template <
      typename ConvertibleUnpackingArchive
    >
    _darma_requires(requires(ConvertibleUnpackingArchive a) {
      PointerReferenceSerializationHandler<>::make_unpacking_archive(ar);
    })
    static void unpack(
      void* allocated,
      ConvertibleUnpackingArchive& ar,
      std::enable_if_t<
        not is_unpackable_with_archive<types::flow_t, ConvertibleUnpackingArchive>::value
          or not is_unpackable_with_archive<types::anti_flow_t, ConvertibleUnpackingArchive>::value,
        darma_runtime::detail::_not_a_type
      > = { }
    )
    {
      // Unpacking flows uses pointer references, so we need to convert to an archive
      // that exposes the pointer (unless the archive type can serialize flows directly
      auto ptr_ar = PointerReferenceSerializationHandler<>::make_unpacking_archive_referencing(ar);
      static_assert(serialization::is_unpackable_with_archive<
        types::flow_t, std::decay_t<decltype(ptr_ar)>
      >::value, "Don't know how to unpack flows");
      static_assert(serialization::is_unpackable_with_archive<
        types::anti_flow_t, std::decay_t<decltype(ptr_ar)>
      >::value, "Don't know how to unpack anti-flows");
      darma_unpack(allocated_buffer_for<AccessHandleT>(allocated), ptr_ar);
    }
};


} // end namespace serialization

template <typename T, typename Traits>
template <typename Archive>
void
AccessHandle<T, Traits>::unpack_from_archive(Archive& ar) {

  using serialization_handler_t =
    darma_runtime::serialization::PointerReferenceSerializationHandler<
      darma_runtime::serialization::SimpleSerializationHandler<std::allocator<char>>
    >;

  key_t k = ar.template unpack_next_item_as<key_t>();

  var_handle_base_ = detail::make_shared<detail::VariableHandle<T>>(k);

  auto use_base = abstract::frontend::PolymorphicSerializableObject<detail::HandleUseBase>
    ::unpack(*reinterpret_cast<char const**>(&ar.data_pointer_reference()));
  use_base->set_handle(var_handle_base_);

  set_current_use(base_t::use_holder_t::recreate_migrated(
    std::move(*use_base.get())
  ));

}
#endif // _darma_has_feature(task_migration)

// </editor-fold> end Serialization }}}1
//==============================================================================

} // end namespace darma_runtime

#endif //DARMAFRONTEND_IMPL_ACCESS_HANDLE_ACCESS_HANDLE_SERIALIZATION_H
