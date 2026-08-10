#ifndef _SLKC_AST_GLOBAL_H_
#define _SLKC_AST_GLOBAL_H_

#include "node.h"
#include <atomic>
#include <memory>

namespace slkc {
	namespace ast {
		struct NodeRegistry final {
			NodeRegistry *next_zero_ref = nullptr;
			size_t ref_count = 0, pin_count = 0;
			std::unique_ptr<Node, peff::DeallocableDeleter<Node>> in_memory;
			NodeIndex self_index;

			SLAKE_FORCEINLINE NodeRegistry() {
			}

			SLAKE_FORCEINLINE NodeRegistry(NodeRegistry &&rhs) : in_memory(std::move(rhs.in_memory)), ref_count(+rhs.ref_count), pin_count(+rhs.pin_count), self_index(rhs.self_index) {
				rhs.ref_count = 0;
				rhs.pin_count = 0;
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
			IOError
		};

		class Global final {
		private:
			peff::RcObjectPtr<peff::Alloc> resource_allocator;
			peff::Map<NodeIndex, NodeRegistry> _node_registries;
			// TODO: Use HashMap instead after the alt version of functions are done.
			peff::Set<GlobalSharedString, std::less<std::string_view>> _shared_strings;
			std::mutex _shared_strings_mutex;
			NodeRegistry *_zero_ref_node_registry_list = nullptr;
			std::recursive_mutex _node_registries_mutex;
			NodeIndex _min_free_node_index = 0;
			NodeIndex _root_module = INVALID_NODE_INDEX;

			SLKC_API void _clear_zero_ref_node_registry_list() noexcept;

			///
			/// @brief Allocate a node index.
			/// @note This function requires the @c _node_registries_mutex to be locked.
			///
			/// @return Allocated node index, @c INVALID_NODE_INDEX if there is no slot.
			///
			[[nodiscard]] SLKC_API NodeIndex _alloc_node_index() noexcept;

		public:
			SLAKE_FORCEINLINE peff::Alloc *get_allocator() noexcept {
				return resource_allocator.get();
			}

			SLAKE_FORCEINLINE void ref_node(NodeIndex index) noexcept {
				_clear_zero_ref_node_registry_list();
				++_node_registries.at(index).ref_count;
			}

			SLKC_API void unref_node(NodeIndex index) noexcept;

			SLKC_API peff::Result<Node *, PinFailReason> pin_node(NodeIndex index) noexcept;

			SLKC_API void unpin_node(NodeIndex index) noexcept;

			///
			/// @brief Allocate a node index and map a node object.
			///
			/// @param node
			/// @return @c peff::NULLOPT if out of memory, @c INVALID_NODE_INDEX if no slot.
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

			SLKC_API peff::Result<NodeIndex, DuplicationError> duplicate_node(NodeIndex node_index) noexcept;

			SLKC_API peff::Result<wandjson::Value *, DumpResult> shallow_dump_node(peff::Alloc *allocator, NodeIndex node_index) noexcept;
			SLKC_API peff::Result<wandjson::Value *, DumpResult> deep_dump_node(peff::Alloc *allocator, NodeIndex node_index) noexcept;

			SLKC_API GlobalSharedString *register_shared_string(std::string_view sv) noexcept;
			SLKC_API void unregister_shared_string(std::string_view s) noexcept;

			SLKC_API bool init_root_module() noexcept;

			SLAKE_FORCEINLINE NodeIndex get_root_module_node_index() noexcept {
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
				++_string->_ref_count;
			}
			SLAKE_FORCEINLINE GlobalSharedStringRef(const GlobalSharedStringRef &rhs) noexcept : _string(rhs._string) {
				++_string->_ref_count;
			}
			SLAKE_FORCEINLINE GlobalSharedStringRef(GlobalSharedStringRef &&rhs) noexcept : _string(rhs._string) {
				rhs._string = nullptr;
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

			SLAKE_FORCEINLINE std::string_view get() const noexcept {
				return *_string;
			}

			SLAKE_FORCEINLINE operator std::string_view() const noexcept {
				return *_string;
			}

			SLAKE_FORCEINLINE std::strong_ordering operator<=>(const GlobalSharedStringRef &rhs) const noexcept {
				return _string <=> rhs._string;
			}
			
			SLAKE_FORCEINLINE std::strong_ordering operator<=>(const std::string_view &rhs) const noexcept {
				return std::string_view(_string->_ptr, _string->_length) <=> rhs;
			}
			
			SLAKE_FORCEINLINE bool operator==(const std::string_view &rhs) const noexcept {
				return std::string_view(_string->_ptr, _string->_length) == rhs;
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
			return _impl(rhs.get());
		}

		SLAKE_FORCEINLINE size_t operator()(const std::string_view &rhs) const {
			return _impl(rhs);
		}
	};
}

#endif
