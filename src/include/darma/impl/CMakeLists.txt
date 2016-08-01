
set (impl_HEADERS
access_handle_publish.impl.h
compatibility.h
config.h
create_condition.h
create_work.h
darma_assert.h
darma.h
functor_traits.h
handle_attorneys.h
handle_fwd.h
handle.h
key_concept.h
publication_details.h
runnable.h
runtime.h
spmd.h
task_do_capture.impl.h
task_fwd.h
task.h
use.h
util.h)

set (impl_key_HEADERS
key/bytes_convert.h
key/bytes_type_metadata.h
key/contiguous_key.h
key/key_fwd.h
key/raw_bytes.h
key/segmented_key.h
key/simple_key_fwd.h
key/simple_key.h)

set (impl_keyword_arguments_HEADERS
keyword_arguments/check_allowed_kwargs.h
keyword_arguments/get_kwarg.h
keyword_arguments/keyword_argument_name.h
keyword_arguments/keyword_arguments.h
keyword_arguments/keyword_tag.h
keyword_arguments/kwarg_expression_fwd.h
keyword_arguments/kwarg_expression.h
keyword_arguments/macros.h)

set (impl_meta_HEADERS
meta/any_convertible.h
meta/callable_traits.h
meta/detection_archetypes.h
meta/detection.h
meta/filter_types.h
meta/is_container.h
meta/is_streamable.h
meta/largest_aligned.h
meta/member_detector.h
meta/sentinal_type.h
meta/splat_tuple.h
meta/tuple_for_each.h
meta/tuple_pop_back.h
meta/tuple_zip.h
meta/void_t.h
meta/wrap_with_template.h)

set (impl_oo_HEADERS
oo/class.h
oo/constructor.h
oo/errors.h
oo/field.h
oo/macros.h
oo/method_runnable.h
oo/method.h
oo/oo_fwd.h
oo/util.h)

set (impl_serialization_HEADERS
serialization/allocation.h
serialization/archive_mixins.h
serialization/archive_passthrough.h
serialization/archive.h
serialization/array.h
serialization/as_pod.h
serialization/builtin.h
serialization/intrusive.h
serialization/nonintrusive.h
serialization/range.h
serialization/serialization_fwd.h
serialization/serializer_attorneys.h
serialization/stl_containers.h
serialization/stl_pair.h
serialization/stl_vector.h
serialization/traits.h
serialization/tuple.h
serialization/unpack_contructor_access.h)


set (impl_util_HEADERS
util/macros.h
util/smart_pointers.h)

install (FILES ${impl_HEADERS} DESTINATION include/darma/impl)
install (FILES ${impl_key_HEADERS} DESTINATION include/darma/impl/key)
install (FILES ${impl_keyword_arguments_HEADERS} DESTINATION include/darma/impl/keyword_arguments)
install (FILES ${impl_meta_HEADERS} DESTINATION include/darma/impl/meta)
install (FILES ${impl_oo_HEADERS} DESTINATION include/darma/impl/oo)
install (FILES ${impl_serialization_HEADERS} DESTINATION include/darma/impl/serialization)
install (FILES ${impl_util_HEADERS} DESTINATION include/darma/impl/util)

include (CheckCXXSourceCompiles)
check_cxx_source_compiles("
extern void* my_memcpy(void *dest, const void *src)
    __attribute__((nonnull (1, 2)));
int main() { }
" DARMA_HAVE_ATTRIBUTE_NONNULL)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-generated.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-generated.h)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/config-generated.h DESTINATION include/darma/impl)
