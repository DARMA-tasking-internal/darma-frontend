
set (impl_HEADERS
compatibility.h
create_work.h
darma.h
darma_assert.h
handle.h
key_concept.h
runtime.h
serialization.h
serialization_builtin.h
spmd.h
task.h
task_fwd.h
util.h)

set (impl_meta_HEADERS
meta/detection.h
meta/filter_types.h
meta/indices_where.h
meta/largest_aligned.h
meta/member_detector.h
meta/metaprogramming.h
meta/move_if.h
meta/sentinal_type.h
meta/splat_tuple.h
meta/tuple_for_each.h
meta/void_t.h
meta/wrap_with_template.h)

set (impl_keyword_arguments_HEADERS
keyword_arguments/get_kwarg.h
keyword_arguments/keyword_argument_name.h
keyword_arguments/keyword_arguments.h
keyword_arguments/keyword_tag.h
keyword_arguments/kwarg_expression.h
keyword_arguments/kwarg_expression_fwd.h
keyword_arguments/macros.h)


install (FILES ${impl_HEADERS} DESTINATION include/darma/impl)
install (FILES ${impl_meta_HEADERS} DESTINATION include/darma/impl/meta)
install (FILES ${impl_keyword_arguments_HEADERS} DESTINATION include/darma/impl/keyword_arguments)

