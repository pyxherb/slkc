#ifndef _SLKC_AST_UTILS_H_
#define _SLKC_AST_UTILS_H_

#include "global.h"
#include "rgtree.h"

namespace slkc {
	namespace ast {
		template <typename T>
		class NodePin final {
		private:
			using ThisType = NodePin<T>;
			Global *_global;
			NodeIndex _node_index;
			union {
				T *_ptr;
				PinFailReason _fail_reason;
			};

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, NodeIndex node_index) {
				_global = global;
				_node_index = node_index;
				global->pin_node(node_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_ptr)
					_global->unpin_node(_node_index);
			}

			SLAKE_FORCEINLINE NodePin() : _global(nullptr), _node_index(INVALID_NODE_INDEX), _ptr(nullptr) {}
			SLAKE_FORCEINLINE explicit NodePin(Global *global, NodeIndex node_index, T *ptr) : _global(global), _node_index(node_index), _ptr(ptr) {
			}
			SLAKE_FORCEINLINE explicit NodePin(NodeIndex node_index, PinFailReason reason) : _global(nullptr), _node_index(node_index), _fail_reason(reason) {
			}
			SLAKE_FORCEINLINE ~NodePin() {
				reset();
			}

			SLAKE_FORCEINLINE NodePin(const ThisType &rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				_global->pin_node(_node_index);
			}
			SLAKE_FORCEINLINE NodePin(ThisType &&rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				rhs._node_index = INVALID_NODE_INDEX;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._node_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_node_index = rhs._node_index;
				rhs._node_index = INVALID_NODE_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE T *get() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE NodeIndex get_index() const noexcept {
				return _node_index;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE T *operator->() const noexcept {
				return _ptr;
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_node_index > rhs._node_index)
					return 1;
				if (_node_index < rhs._node_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index < rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index > rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index == rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index != rhs._node_index;
			}

			SLAKE_FORCEINLINE operator bool() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE bool is_fail() const noexcept {
				return !_global;
			}

			SLAKE_FORCEINLINE PinFailReason get_fail_reason() const noexcept {
				return _fail_reason;
			}

			template <typename T1>
			SLAKE_FORCEINLINE NodePin<T1> cast_to() const noexcept {
				_global->pin_node(_node_index);
				return NodePin<T1>(_global, _node_index, static_cast<T1 *>(_ptr));
			}
		};

		template <typename T>
		class NodePtr final {
		private:
			using ThisType = NodePtr<T>;
			Global *_global;
			NodeIndex _node_index;

			SLAKE_FORCEINLINE void _set_and_inc_ref(Global *global, NodeIndex node_index) {
				_global = global;
				_node_index = node_index;
				global->ref_node(node_index);
			}

		public:
			SLAKE_FORCEINLINE void reset() noexcept {
				if (_node_index != INVALID_NODE_INDEX)
					_global->unref_node(_node_index);
			}

			SLAKE_FORCEINLINE NodePtr() : _global(nullptr), _node_index(INVALID_NODE_INDEX) {}
			SLAKE_FORCEINLINE NodePtr(const NodePin<T> &pin) : _global(pin.get_global()), _node_index(pin.get_index()) {}
			SLAKE_FORCEINLINE explicit NodePtr(Global *global, NodeIndex node_index) : _global(global), _node_index(node_index) {
				global->ref_node(node_index);
			}
			SLAKE_FORCEINLINE ~NodePtr() {
				reset();
			}

			SLAKE_FORCEINLINE NodePtr(const ThisType &rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				_global->ref_node(_node_index);
			}
			SLAKE_FORCEINLINE NodePtr(ThisType &&rhs) noexcept : _global(rhs._global), _node_index(rhs._node_index) {
				rhs._node_index = INVALID_NODE_INDEX;
			}

			SLAKE_FORCEINLINE ThisType &operator=(const ThisType &rhs) noexcept {
				reset();
				_set_and_inc_ref(rhs._global, rhs._node_index);

				return *this;
			}
			SLAKE_FORCEINLINE ThisType &operator=(ThisType &&rhs) noexcept {
				reset();
				_global = rhs._global;
				_node_index = rhs._node_index;
				rhs._node_index = INVALID_NODE_INDEX;

				return *this;
			}

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return _global;
			}

			SLAKE_FORCEINLINE NodeIndex get_index() const noexcept {
				return _node_index;
			}

			///
			/// @brief Pin the pointer.
			///
			/// @return A nonnull pointer to the pinned object if success, or a null pointer indicating that the pinning fails.
			///
			SLAKE_FORCEINLINE NodePin<T> pin() const noexcept {
				auto result = _global->pin_node(_node_index);
				if (result.has_error())
					return NodePin<T>(_node_index, std::move(result).error());
				return NodePin<T>(_global, _node_index, static_cast<T*>(std::move(result).value()));
			}

			SLAKE_FORCEINLINE static NodePtr<T> from_pin(NodePin<T> pin) noexcept {
				return NodePtr<T>(pin._global, pin._node_index);
			}

			SLAKE_FORCEINLINE int compares_to(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);

				if (_node_index > rhs._node_index)
					return 1;
				if (_node_index < rhs._node_index)
					return -1;
				return 0;
			}

			SLAKE_FORCEINLINE bool operator<(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index < rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator>(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index > rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator==(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index == rhs._node_index;
			}

			SLAKE_FORCEINLINE bool operator!=(const ThisType &rhs) const noexcept {
				assert(_global == rhs._global);
				return _node_index != rhs._node_index;
			}

			SLAKE_FORCEINLINE operator bool() const noexcept {
				return _node_index != INVALID_NODE_INDEX;
			}

			template <typename T1>
			SLAKE_FORCEINLINE NodePtr<T1> cast_to() const noexcept {
				static_assert(std::is_convertible_v<T *, T1 *>);

				return NodePtr<T1>(_global, _node_index);
			}
		};

		template <typename T>
		class LambdaDuplicationContextHook : public DuplicationContextHook {
		public:
			Global *global;
			T impl;

			using This = LambdaDuplicationContextHook;

			SLAKE_FORCEINLINE LambdaDuplicationContextHook(Global *global, T &&impl) : global(global), impl(impl) {}
			virtual inline DuplicationError run() override {
				impl();
			}
			virtual inline void dealloc() noexcept override {
				peff::destroy_and_release<This>(global->get_allocator(), this, alignof(This));
			}
		};

		template <typename T>
		LambdaDuplicationContextHook<T> *alloc_lambda_duplication_context_hook(DuplicationContext &context, T &&impl) {
			return peff::alloc_and_construct<LambdaDuplicationContextHook<T>>(context.get_global()->get_allocator(), alignof(LambdaDuplicationContextHook<T>), std::move(impl));
		}

		///
		/// @brief Create a new node object.
		/// @note The newly created node object will be pinned once by default.
		///
		/// @tparam T Type of the node object to be created.
		/// @tparam Args Arguments to be passed to the constructor.
		///
		template <typename T, typename... Args>
		SLAKE_FORCEINLINE NodePin<T> make_node(Global *global, Args &&...args)
			PEFF_REQUIRES_CONCEPT(std::constructible_from<T, Global *, Args...>) {
			T *node = peff::alloc_and_construct<T>(global->get_allocator(), alignof(T), global, std::forward<Args>(args)...);
			if (!node)
				return NodePin<T>();
			peff::ScopeGuard sg([global, node]() noexcept {
				peff::destroy_and_release<T>(global->get_allocator(), node, alignof(T));
			});
			if (!global->map_node(node).has_value())
				return NodePin<T>();
			sg.release();

			global->pin_node(node->get_node_index());

			return NodePin<T>(global, node->get_node_index(), node);
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
		SLAKE_FORCEINLINE T *make_node_dup(Global *global, Args &&...args)
			PEFF_REQUIRES_CONCEPT(std::constructible_from<T, Args...>) {
			return peff::alloc_and_construct<T>(global->get_allocator(), alignof(T), std::forward<Args>(args)...);
		}
	}
}

/// @brief Macro used for declaring a simple instance of the deallocation method for an AST node class.
#define SLKC_SIMPLE_AST_DEALLOC_FN_DECL() \
	SLKC_API void dealloc() noexcept override

/// @brief Macro used for defining a simple instance of the deallocation method for an AST node class.
#define SLKC_SIMPLE_AST_DEALLOC_FN_DEF(name)                                                       \
	SLKC_API void name::dealloc() noexcept {                                                       \
		peff::destroy_and_release<name>(this->get_global()->get_allocator(), this, alignof(name)); \
	}

/// @brief Macro used for declaring a simple instance of the duplication method for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DECL() \
	[[nodiscard]] SLKC_API virtual peff::Result<Node *, DuplicationError> do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept override

/// @brief Macro used for defining a simple instance of the duplication method for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DEF(name)                                                                                                                               \
	SLKC_API peff::Result<Node *, DuplicationError> name::do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept {                       \
		std::unique_ptr<name, peff::DeallocableDeleter<name>> ptr(slkc::ast::make_node_dup<name>(duplication_context.get_global(), *this, duplication_context, node_index)); \
                                                                                                                                                                             \
		if (!ptr)                                                                                                                                                            \
			return slkc::ast::DuplicationError::OutOfMemory;                                                                                                                 \
                                                                                                                                                                             \
		return ptr.release();                                                                                                                                                \
	}

/// @brief Macro used for defining a null instance of the duplication method for an AST node class.
#define SLKC_NULL_AST_DUPLICATE_FN_DEF(name)                                                                                                           \
	SLKC_API peff::Result<Node *, DuplicationError> name::do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept { \
		peff::panic("The class " #name " cannot be duplicated");                                                                                       \
		PEFF_UNREACHABLE();                                                                                                                            \
	}

/// @brief Macro used for defining a simple instance of the duplication method with a result output for an AST node class.
#define SLKC_SIMPLE_AST_DUPLICATE_FN_DEF_WITH_RESULT(name)                                                                                                                          \
	SLKC_API peff::Result<Node *, DuplicationError> name::do_duplicate(DuplicationContext &duplication_context, NodeIndex node_index) const noexcept {                              \
		peff::Option<DuplicationError> error;                                                                                                                                       \
		std::unique_ptr<name, peff::DeallocableDeleter<name>> ptr(slkc::ast::make_node_dup<name>(duplication_context.get_global(), *this, duplication_context, node_index, error)); \
                                                                                                                                                                                    \
		if (!ptr)                                                                                                                                                                   \
			return slkc::ast::DuplicationError::OutOfMemory;                                                                                                                        \
                                                                                                                                                                                    \
		if (error.has_value())                                                                                                                                                      \
			return std::move(error).value();                                                                                                                                        \
                                                                                                                                                                                    \
		return static_cast<Node *>(ptr.release());                                                                                                                                  \
	}

#endif
