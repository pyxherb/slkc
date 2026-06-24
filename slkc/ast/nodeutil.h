#ifndef _SLKC_AST_NODEUTIL_H_
#define _SLKC_AST_NODEUTIL_H_

#include "global.h"

namespace slkc {
	namespace ast {
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

			PEFF_FORCEINLINE NodeIndex get_global() const noexcept {
				return _global;
			}

			PEFF_FORCEINLINE NodeIndex get_index() const noexcept {
				return _node_index;
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

#endif
