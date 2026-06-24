#ifndef _SLKC_AST_NODE_H_
#define _SLKC_AST_NODE_H_

#include <slkc/basedefs.h>
#include <cstdint>
#include <peff/utils/result.h>
#include <peff/advutils/shared_ptr.h>
#include <peff/containers/map.h>
#include <peff/containers/hashmap.h>
#include <peff/base/deallocable.h>

namespace slkc {
	namespace ast {
		enum class NodeType : uint8_t {
			Bad = 0,

			Class,

			Struct,

			ConstEnum,
			ScopedEnum,
			UnionEnum,

			EnumItem,
			UnionEnumItem,

			Attribute,

			Except,
			Interface,

			AppliedAttribute,

			Fn,
			FnOverloading,

			Stmt,
			Expr,

			Var,

			GenericParam,

			Module,

			Import,

			This,

			TypeNameDef
		};

		using NodeIndex = uint32_t;
		using TokenIndex = size_t;

		constexpr NodeIndex INVALID_NODE_INDEX = std::numeric_limits<NodeIndex>::max();

		class Global;

		struct DuplicationTask {
			NodeIndex dest, src;
		};

		enum class DuplicationResult : uint8_t {
			Success = 0,
			NoSlot,
			OutOfMemory,
			PinningFailed
		};

		struct DuplicationContext {
			Global *global;
			peff::List<DuplicationTask> task_list;

			SLKC_API DuplicationContext(Global *global);
			SLKC_API peff::Option<NodeIndex> push_task(NodeIndex node_index) noexcept;
		};

		struct TokenRange {
			NodeIndex source_node;
			TokenIndex begin, end;
		};

		class Node : public peff::SharedFromThis<Node> {
		private:
			NodeType _ast_node_type;
			peff::Alloc *_self_allocator;

		protected:
			virtual peff::Result<Node *, DuplicationResult> do_duplicate(DuplicationContext &duplication_context) = 0;

			friend Global;

		public:
			const peff::RcObjectPtr<peff::Alloc> self_allocator;
			TokenRange token_range;

			SLKC_API Node(NodeType ast_node_type, peff::Alloc *self_allocator, const peff::SharedPtr<Global> &document);
			SLKC_API Node(const Node &other, peff::Alloc *new_allocator, DuplicationContext &context);
			SLKC_API virtual ~Node();

			virtual void dealloc() noexcept = 0;

#if SLKC_WITH_AST_DUMPING
			SLAKE_API wandjson::Value *dump(peff::Alloc *allocator) const noexcept;
#endif

			PEFF_FORCEINLINE NodeType get_ast_node_type() const noexcept {
				return _ast_node_type;
			}
		};

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
		};

		template <typename T>
		class NodePin {
		private:
			using ThisType = NodePin<T>;
			Global *_global;
			NodeIndex _node_index;
			T *_ptr;

			PEFF_FORCEINLINE void _set_and_inc_ref(Global *global, NodeIndex node_index) {
				_global = global;
				_node_index = node_index;
				global->pin_node(node_index);
			}

		public:
			PEFF_FORCEINLINE void reset() noexcept {
				if (_ptr)
					_global->unpin_node(_node_index);
			}

			PEFF_FORCEINLINE NodePin() : _global(nullptr), _node_index(INVALID_NODE_INDEX), _ptr(nullptr) {}
			PEFF_FORCEINLINE explicit NodePin(Global *global, NodeIndex node_index, T *ptr) : _global(global), _node_index(node_index), _ptr(ptr) {
				// The pinning may fail, we pin outside of the constructor.
			}
			PEFF_FORCEINLINE ~NodePin() {
				reset();
			}

			PEFF_FORCEINLINE NodePin(const ThisType &rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				_global->pin_node(_node_index);
			}
			PEFF_FORCEINLINE NodePin(ThisType &&rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				rhs._node_index = INVALID_NODE_INDEX;
			}

			PEFF_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._node_index);

				return *this;
			}
			PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_node_index = rhs._node_index;
				rhs._node_index = INVALID_NODE_INDEX;

				return *this;
			}

			PEFF_FORCEINLINE T *get() const noexcept {
				return _ptr;
			}

			PEFF_FORCEINLINE T *operator->() const noexcept {
				return _ptr;
			}

			PEFF_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_node_index > rhs._node_index)
					return 1;
				if (_node_index < rhs._node_index)
					return -1;
				return 0;
			}

			PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index < rhs._node_index;
			}

			PEFF_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index > rhs._node_index;
			}

			PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index == rhs._node_index;
			}

			PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index != rhs._node_index;
			}

			PEFF_FORCEINLINE operator bool() const noexcept {
				return _ptr;
			}

			template <typename T1>
			PEFF_FORCEINLINE NodePin<T1> cast_to() const noexcept {
				static_assert(std::is_convertible_v<T *, T1 *>);

				_global->pin_node(_node_index);
				return NodePin<T1>(_global, _node_index, static_cast<T1>(_ptr));
			}
		};

		template <typename T>
		class NodePtr {
		private:
			using ThisType = NodePtr<T>;
			Global *_global;
			NodeIndex _node_index;

			PEFF_FORCEINLINE void _set_and_inc_ref(Global *global, NodeIndex node_index) {
				_global = global;
				_node_index = node_index;
				global->ref_node(node_index);
			}

		public:
			PEFF_FORCEINLINE void reset() noexcept {
				if (_node_index != INVALID_NODE_INDEX)
					_global->unref_node(_node_index);
			}

			PEFF_FORCEINLINE NodePtr() : _global(nullptr), _node_index(INVALID_NODE_INDEX) {}
			PEFF_FORCEINLINE explicit NodePtr(Global *global, NodeIndex node_index) : _global(global), _node_index(node_index) {
				global->ref_node(node_index);
			}
			PEFF_FORCEINLINE ~NodePtr() {
				reset();
			}

			PEFF_FORCEINLINE NodePtr(const ThisType &rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				_global->ref_node(_node_index);
			}
			PEFF_FORCEINLINE NodePtr(ThisType &&rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				rhs._node_index = INVALID_NODE_INDEX;
			}

			PEFF_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._node_index);

				return *this;
			}
			PEFF_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_node_index = rhs._node_index;
				rhs._node_index = INVALID_NODE_INDEX;

				return *this;
			}

			///
			/// @brief Pin the pointer.
			///
			/// @return A nonnull pointer to the pinned object if success, or a null pointer indicating that the pinning fails.
			///
			PEFF_FORCEINLINE NodePin<T> pin() const noexcept {
				return NodePin<T>(_global, _node_index, _global->pin_node(_node_index));
			}

			PEFF_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_node_index > rhs._node_index)
					return 1;
				if (_node_index < rhs._node_index)
					return -1;
				return 0;
			}

			PEFF_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index < rhs._node_index;
			}

			PEFF_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index > rhs._node_index;
			}

			PEFF_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index == rhs._node_index;
			}

			PEFF_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index != rhs._node_index;
			}

			PEFF_FORCEINLINE operator bool() const noexcept {
				return _node_index != INVALID_NODE_INDEX;
			}

			template <typename T1>
			PEFF_FORCEINLINE NodePtr<T1> cast_to() const noexcept {
				static_assert(std::is_convertible_v<T *, T1 *>);

				return NodePtr<T1>(_global, _node_index);
			}
		};
	}
}

#define SLKC_SIMPLE_AST_DEALLOC_FN_DECL(name) \
	SLKC_API void dealloc() noexcpet override;

#define SLKC_SIMPLE_AST_DEALLOC_FN_DEF(name)                                               \
	SLKC_API void name::dealloc() noexcpet {                                               \
		peff::destroy_and_release<name>(this->_self_allocator.get(), this, alignof(name)); \
	}

#endif
