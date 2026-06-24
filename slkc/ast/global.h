#ifndef _SLKC_AST_GLOBAL_H_
#define _SLKC_AST_GLOBAL_H_

#include "node.h"
#include <atomic>
#include <memory>

namespace slkc {
	namespace ast {
		struct NodeRegistry {
			NodeRegistry *next_zero_ref = nullptr;
			std::atomic_size_t ref_count = 0, pin_count = 0;
			std::unique_ptr<Node, peff::DeallocableDeleter<Node>> in_memory;
			NodeIndex self_index;

			PEFF_FORCEINLINE NodeRegistry() {
			}

			PEFF_FORCEINLINE NodeRegistry(NodeRegistry &&rhs) : in_memory(std::move(rhs.in_memory)), ref_count(+rhs.ref_count), pin_count(+rhs.pin_count), self_index(rhs.self_index) {
				rhs.ref_count = 0;
				rhs.pin_count = 0;
			}
		};

		class Global {
		private:
			peff::RcObjectPtr<peff::Alloc> resource_allocator;
			peff::Map<NodeIndex, NodeRegistry> _node_registries;
			NodeRegistry *_zero_ref_node_registry_list = nullptr;
			std::recursive_mutex _node_registries_mutex;
			NodeIndex _min_free_node_index = 0;

			SLKC_API void _clear_zero_ref_node_registry_list() noexcept;

			///
			/// @brief Allocate a node index.
			/// @note This function requires the @c _node_registries_mutex to be locked.
			///
			/// @return Allocated node index, @c INVALID_NODE_INDEX if there is no slot.
			///
			[[nodiscard]] SLKC_API NodeIndex _alloc_node_index() noexcept;

		public:
			PEFF_FORCEINLINE peff::Alloc *get_allocator() noexcept {
				return resource_allocator.get();
			}

			PEFF_FORCEINLINE void ref_node(NodeIndex index) noexcept {
				_clear_zero_ref_node_registry_list();
				++_node_registries.at(index).ref_count;
			}

			SLKC_API void unref_node(NodeIndex index) noexcept;

			SLKC_API Node *pin_node(NodeIndex index) noexcept;

			SLKC_API void unpin_node(NodeIndex index) noexcept;

			///
			/// @brief Allocate a node index and map a node object.
			///
			/// @param node
			/// @return @c peff::NULL_OPTION if out of memory, @c INVALID_NODE_INDEX if no slot.
			///
			[[nodiscard]] SLKC_API peff::Option<NodeIndex> map_node(Node *node) noexcept;
			///
			/// @brief Map a node to a new node index.
			///
			/// @param node_index Node index to be mapped.
			/// @param node Node to be mapped.
			/// @return @c true if success, @c false if out of memory.
			///
			[[nodiscard]] SLKC_API bool map_node(NodeIndex node_index, Node *node) noexcept;

			///
			/// @brief Remap an existed node index.
			///
			/// @param node_index Node index to be remapped.
			/// @param node Node to be mapped to the memory.
			///
			SLKC_API void remap_node(NodeIndex node_index, Node *node) noexcept;

			SLKC_API void unmap_node(NodeIndex node_index) noexcept;

			SLKC_API peff::Result<NodeIndex, DuplicationResult> duplicate_node(NodeIndex node_index) noexcept;

			SLKC_API peff::Result<wandjson::Value *, DumpResult> shallow_dump_node(NodeIndex node_index) noexcept;
			SLKC_API peff::Result<wandjson::Value *, DumpResult> deep_dump_node(NodeIndex node_index) noexcept;
		};

		template <typename T, typename... Args>
		PEFF_FORCEINLINE T *make_node(Global *global, Args &&...args)
			PEFF_REQUIRES_CONCEPT(std::constructible_from<T, Global *, Args...>) {
			return peff::alloc_and_construct<T>(global->get_allocator(), alignof(T), global, std::forward<Args>(args)...);
		}

		///
		/// @brief Duplication operation version of @c make_node.
		///
		/// @tparam T Type of node to be made.
		/// @tparam Args Argument types to be passed to the constructor.
		///
		/// @param global Global used for making the node.
		/// @param args Arguments to be passed to the constructor.
		///
		template <typename T, typename... Args>
		PEFF_FORCEINLINE T *make_node_dup(Global *global, Args &&...args)
			PEFF_REQUIRES_CONCEPT(std::constructible_from<T, Args...>) {
			return peff::alloc_and_construct<T>(global->get_allocator(), alignof(T), std::forward<Args>(args)...);
		}
	}
}

#endif
