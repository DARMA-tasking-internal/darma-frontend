
#include <memory>
namespace darma_runtime { namespace types {
  template <typename... Ts>
  using shared_ptr_template = std::shared_ptr<Ts...>;
  template <typename... Ts>
  using unique_ptr_template = std::unique_ptr<Ts...>;
}} // end namespace darma_runtime::types

