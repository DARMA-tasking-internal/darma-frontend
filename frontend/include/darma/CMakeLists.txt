set (darma_abstract_backend_HEADERS
abstract/backend/data_block.h
abstract/backend/runtime.h
abstract/backend/types.h)

set (darma_abstract_frontend_HEADERS
abstract/frontend/aliasing_manager.h
abstract/frontend/containment_manager.h
abstract/frontend/dependency_handle.h
abstract/frontend/serialization_manager.h
abstract/frontend/task.h
abstract/frontend/types.h)

set (darma_abstract_defaults_HEADERS
abstract/defaults/key.h
abstract/defaults/key_fwd.h
abstract/defaults/pointers.h
abstract/defaults/version.h)

set (key_args_HEADERS
keyword_arguments/get_kwarg.h
keyword_arguments/keyword_argument_name.h
keyword_arguments/keyword_arguments.h
keyword_arguments/keyword_tag.h
keyword_arguments/kwarg_expression.h
keyword_arguments/kwarg_expression_fwd.h
keyword_arguments/macros.h)

set (meta_HEADERS
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

set (darma_HEADERS
create_work.h
darma.h
darma_assert.h
darma_main.h
handle.h
runtime.h
serialization.h
serialization_builtin.h
spmd.h
task.h
task_fwd.h
util.h)


install (FILES ${darma_HEADERS} DESTINATION include/darma)
install (FILES ${darma_abstract_defaults_HEADERS} DESTINATION include/darma/abstract/defaults)
install (FILES ${darma_abstract_backend_HEADERS} DESTINATION include/darma/abstract/backend)
install (FILES ${darma_abstract_frontend_HEADERS} DESTINATION include/darma/abstract/frontend)
install (FILES ${key_args_HEADERS} DESTINATION include/darma/keyword_arguments)
install (FILES ${meta_HEADERS} DESTINATION include/darma/meta)

