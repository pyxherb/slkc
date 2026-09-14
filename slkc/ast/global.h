#ifndef _SLKC_AST_GLOBAL_H_
#define _SLKC_AST_GLOBAL_H_

#include "astnode.h"
#include <atomic>
#include <peff/containers/hashmap.h>

namespace slkc {
	namespace ast {
		struct NodeRegistry final {
			size_t ref_count = 0, weak_ref_count = 0, pin_count = 0;
			AstNode *in_memory;
			AstNodeIndex self_index;

			SLAKE_FORCEINLINE NodeRegistry() {
			}

			SLAKE_FORCEINLINE NodeRegistry(NodeRegistry &&rhs) : in_memory(std::move(rhs.in_memory)), ref_count(+rhs.ref_count), weak_ref_count(+rhs.weak_ref_count), pin_count(+rhs.pin_count), self_index(rhs.self_index) {
				rhs.ref_count = 0;
				rhs.weak_ref_count = 0;
				rhs.pin_count = 0;
			}

			SLKC_API ~NodeRegistry();
		};

		struct GreenNode;

		struct GreenNodeRegistry final {
			size_t ref_count = 0, weak_ref_count = 0, pin_count = 0;
			GreenNode *in_memory = nullptr;
			GreenNodeIndex self_index;

			SLAKE_FORCEINLINE GreenNodeRegistry() {
			}

			SLKC_API ~GreenNodeRegistry();

			SLAKE_FORCEINLINE GreenNodeRegistry(GreenNodeRegistry &&rhs) : in_memory(rhs.in_memory), ref_count(+rhs.ref_count), weak_ref_count(+rhs.weak_ref_count), pin_count(+rhs.pin_count), self_index(rhs.self_index) {
				rhs.in_memory = nullptr;
				rhs.ref_count = 0;
				rhs.weak_ref_count = 0;
				rhs.pin_count = 0;
				rhs.self_index = INVALID_GREEN_NODE_INDEX;
			}
		};

		struct GlobalSharedString {
		private:
			Global *_global;
			char *_ptr;
			size_t _length;
			std::atomic_size_t _ref_count = 0;

			friend class Global;
			friend struct GlobalSharedStringRef;

		public:
			SLKC_API GlobalSharedString() noexcept;
			SLAKE_FORCEINLINE GlobalSharedString(GlobalSharedString &&rhs) noexcept
				: _global(rhs._global),
				  _ptr(rhs._ptr),
				  _length(rhs._length),
				  _ref_count(+rhs._ref_count) {
				rhs._ptr = nullptr;
			}
			SLKC_API ~GlobalSharedString();

			SLAKE_FORCEINLINE operator std::string_view() const noexcept {
				return std::string_view(_ptr, _length);
			}
		};

		enum class PinFailReason : uint8_t {
			OutOfMemory = 0,
			IOError,
			OutOfNodeIndex,
		};

		class Global final {
		private:
			peff::RcObjectPtr<peff::Alloc> resource_allocator;
			// TODO: Use HashMap instead after the alt version of functions are done.
			peff::HashSet<GlobalSharedString, std::equal_to<std::string_view>, peff::Hasher<std::string_view>> _shared_strings;
			std::mutex _shared_strings_mutex;

			peff::Map<AstNodeIndex, NodeRegistry> _ast_node_registries;
			AstNode *_zero_ref_ast_node_registry_list = nullptr;
			std::recursive_mutex _ast_node_registries_mutex;
			AstNodeIndex _min_free_ast_node_index = 0;

			peff::Map<GreenNodeIndex, GreenNodeRegistry> _green_node_registries;
			GreenNode *_zero_ref_green_node_registry_list = nullptr;
			std::recursive_mutex _green_node_registries_mutex;
			GreenNodeIndex _min_free_green_node_index = 0;

			AstNodeIndex _root_module = INVALID_AST_NODE_INDEX;

			SLKC_API void _clear_zero_ref_ast_node_registry_list() noexcept;
			SLKC_API void _clear_zero_ref_green_node_registry_list() noexcept;

			///
			/// @brief Allocate a node index.
			/// @note This function requires the @c _node_registries_mutex to be locked.
			///
			/// @return Allocated node index, @c INVALID_AST_NODE_INDEX if there is no slot.
			///
			[[nodiscard]] SLKC_API AstNodeIndex _alloc_ast_node_index() noexcept;
			[[nodiscard]] SLKC_API GreenNodeIndex _alloc_green_node_index() noexcept;

			SLKC_API void _add_ast_node_to_deferred_deleting_list(AstNode *node_registry) noexcept;
			SLKC_API void _add_green_node_to_deferred_deleting_list(GreenNode *green_node_registry) noexcept;

			friend struct GreenNodeRegistry;

		public:
			SLKC_API Global(peff::Alloc *allocator) noexcept;
			SLKC_API ~Global() noexcept;

			SLAKE_FORCEINLINE peff::Alloc *get_allocator() noexcept {
				return resource_allocator.get();
			}

			SLAKE_FORCEINLINE void ref_ast_node(AstNodeIndex index) noexcept {
				_clear_zero_ref_ast_node_registry_list();
				++_ast_node_registries.at(index).ref_count;
			}
			SLKC_API bool try_ref_ast_node(AstNodeIndex index) noexcept;
			SLAKE_FORCEINLINE void ref_ast_node_weak(AstNodeIndex index) noexcept {
				_clear_zero_ref_ast_node_registry_list();
				++_ast_node_registries.at(index).weak_ref_count;
			}
			SLKC_API void unref_ast_node(AstNodeIndex index) noexcept;
			SLKC_API void unref_ast_node_weak(AstNodeIndex index) noexcept;
			SLKC_API peff::Result<AstNode *, PinFailReason> pin_ast_node(AstNodeIndex index) noexcept;
			SLKC_API void unpin_ast_node(AstNodeIndex index) noexcept;
			///
			/// @brief Allocate a node index and map a node object.
			///
			/// @param node
			/// @return @c peff::NULLOPT if out of memory, @c INVALID_AST_NODE_INDEX if no slot.
			///
			[[nodiscard]] SLKC_API peff::Option<AstNodeIndex> map_ast_node(AstNode *node, AstNodeIndex node_index = INVALID_AST_NODE_INDEX) noexcept;
			///
			/// @brief Remap an existed node index.
			///
			/// @param node_index Node index to be remapped.
			/// @param node Node to be mapped to the memory.
			///
			SLKC_API void remap_ast_node(AstNodeIndex node_index, AstNode *node) noexcept;
			SLKC_API void unmap_ast_node(AstNodeIndex node_index) noexcept;
			SLKC_API peff::Result<AstNodeIndex, DuplicationError> duplicate_ast_node(AstNodeIndex node_index) noexcept;
			SLKC_API peff::Result<wandjson::Value *, DumpResult> shallow_dump_ast_node(peff::Alloc *allocator, AstNodeIndex node_index) noexcept;
			SLKC_API peff::Result<wandjson::Value *, DumpResult> deep_dump_ast_node(peff::Alloc *allocator, AstNodeIndex node_index) noexcept;

			SLAKE_FORCEINLINE void ref_green_node(GreenNodeIndex index) noexcept {
				_clear_zero_ref_green_node_registry_list();
				++_green_node_registries.at(index).ref_count;
			}
			SLKC_API bool try_ref_green_node(AstNodeIndex index) noexcept;
			SLAKE_FORCEINLINE void ref_green_node_weak(GreenNodeIndex index) noexcept {
				_clear_zero_ref_green_node_registry_list();
				++_green_node_registries.at(index).weak_ref_count;
			}
			SLKC_API void unref_green_node(GreenNodeIndex index) noexcept;
			SLKC_API void unref_green_node_weak(GreenNodeIndex index) noexcept;
			SLKC_API peff::Result<GreenNode *, PinFailReason> pin_green_node(GreenNodeIndex index) noexcept;
			SLKC_API void unpin_green_node(GreenNodeIndex index) noexcept;
			///
			/// @brief Allocate a node index and map a node object.
			///
			/// @param node
			/// @return @c peff::NULLOPT if out of memory, @c INVALID_AST_NODE_INDEX if no slot.
			///
			[[nodiscard]] SLKC_API peff::Option<GreenNodeIndex> map_green_node(GreenNode *node, GreenNodeIndex node_index = INVALID_GREEN_NODE_INDEX) noexcept;
			///
			/// @brief Remap an existed node index.
			///
			/// @param node_index Node index to be remapped.
			/// @param node Node to be mapped to the memory.
			///
			SLKC_API void remap_green_node(GreenNodeIndex node_index, GreenNode *node) noexcept;
			SLKC_API void unmap_green_node(GreenNodeIndex node_index) noexcept;
			SLKC_API peff::Result<wandjson::Value *, DumpResult> shallow_dump_green_node(peff::Alloc *allocator, GreenNodeIndex node_index) noexcept;
			SLKC_API peff::Result<wandjson::Value *, DumpResult> deep_dump_green_node(peff::Alloc *allocator, GreenNodeIndex node_index) noexcept;

			SLKC_API GlobalSharedString *register_shared_string(std::string_view sv) noexcept;
			SLKC_API void unregister_shared_string(std::string_view s) noexcept;

			SLKC_API bool init_root_module() noexcept;

			SLAKE_FORCEINLINE AstNodeIndex get_root_module_node_index() noexcept {
				return _root_module;
			}
		};

		struct GlobalSharedStringRef {
		private:
			GlobalSharedString *_string;

			SLAKE_FORCEINLINE void _reset() {
				if (_string) {
					if (!--_string->_ref_count) {
						_string->_global->unregister_shared_string(*_string);
					}
					_string = nullptr;
				}
			}

		public:
			SLAKE_FORCEINLINE GlobalSharedStringRef() noexcept : _string(nullptr) {
			}
			SLAKE_FORCEINLINE GlobalSharedStringRef(GlobalSharedString *string) noexcept : _string(string) {
				if (string)
					++_string->_ref_count;
			}
			SLAKE_FORCEINLINE GlobalSharedStringRef(const GlobalSharedStringRef &rhs) noexcept : _string(rhs._string) {
				if (_string)
					++_string->_ref_count;
			}
			SLAKE_FORCEINLINE GlobalSharedStringRef(GlobalSharedStringRef &&rhs) noexcept : _string(rhs._string) {
				rhs._string = nullptr;
			}
			SLAKE_FORCEINLINE ~GlobalSharedStringRef() {
				_reset();
			}

			SLAKE_FORCEINLINE GlobalSharedStringRef &operator=(const GlobalSharedStringRef &rhs) noexcept {
				_reset();
				if (rhs._string) {
					_string = rhs._string;
					++_string->_ref_count;
				}
				return *this;
			}
			SLAKE_FORCEINLINE GlobalSharedStringRef &operator=(GlobalSharedStringRef &&rhs) noexcept {
				_reset();
				if (rhs._string) {
					_string = rhs._string;
					rhs._string = nullptr;
				}
				return *this;
			}

			SLAKE_FORCEINLINE std::string_view get_view() const noexcept {
				return std::string_view(_string->_ptr, _string->_length);
			}

			SLAKE_FORCEINLINE operator std::string_view() const noexcept {
				return std::string_view(_string->_ptr, _string->_length);
			}

			SLAKE_FORCEINLINE operator bool() const noexcept {
				return _string;
			}

			SLAKE_FORCEINLINE std::strong_ordering operator<=>(const GlobalSharedStringRef &rhs) const noexcept {
				return _string <=> rhs._string;
			}

			SLAKE_FORCEINLINE std::strong_ordering operator<=>(const std::string_view &rhs) const noexcept {
				return std::string_view(_string->_ptr, _string->_length) <=> rhs;
			}

			bool operator<(const GlobalSharedStringRef &) const noexcept = default;
			bool operator>(const GlobalSharedStringRef &) const noexcept = default;
			bool operator==(const GlobalSharedStringRef &) const noexcept = default;
			bool operator!=(const GlobalSharedStringRef &) const noexcept = default;

			SLAKE_FORCEINLINE operator bool() {
				return _string;
			}
		};

		struct GlobalSharedStringRefEq {
			bool operator()(const GlobalSharedStringRef &lhs, const GlobalSharedStringRef &rhs) const noexcept {
				return lhs == rhs;
			}
			bool operator()(const GlobalSharedStringRef &lhs, const std::string_view &rhs) const noexcept {
				return lhs == rhs;
			}
			bool operator()(const std::string_view &lhs, const GlobalSharedStringRef &rhs) const noexcept {
				return rhs == lhs;
			}
		};
	}
}

namespace peff {
	template <>
	struct Hasher<slkc::ast::GlobalSharedStringRef> {
		peff::Hasher<std::string_view> _impl;

		SLAKE_FORCEINLINE size_t operator()(const slkc::ast::GlobalSharedStringRef &rhs) const {
			return _impl(rhs.get_view());
		}

		SLAKE_FORCEINLINE size_t operator()(const std::string_view &rhs) const {
			return _impl(rhs);
		}
	};
}

#endif
