# General Instructions of SLKC for Agents

This instruction will instruct you to maintain and understand the project more efficiently.

## Rules

Unlike the main VM project (pyxherb/slake, it uses C++17), we use C++20 for this project. Use `.h` for C/C++ header files and use `.cc` for C++ source files by default.

For C++, use `snake_case` for functions, variables, namespaces and file names, `UpperCamelCase` for types, add `_` prefix for private members (except types and namespaces):

```cpp
namespace my_namespace {
    class MyClass {
    private:
        struct MyType {
        };

        using PrivateAlias = uint32_t;

        int _private_member;

        void _test_impl();

    public:
        inline int get_private_member() const noexcept {
            return this->_private_member;
        }

        inline void set_private_member(int private_member) noexcept {
            this->_private_member = private_member;
        }
    }

    void public_fn() {
        int local_var = 123;
        // ...
    }
}
```

Always use Doxygen-styled documentations in C++, and use US English in documentations and comments.

DO NOT USE **ANY** exception in this project, unless you are interacting with libraries with exception used (such as FlatBuffers). Add `noexcept` function modifiers if possible (must not cause aborting).

**RTTI** is ABSOLUTELY PROHIBITED, you **MUST NOT** implement anything in this library with RTTI.

Usually, we make a header file corresponds to a source file with the same name (e.g. `type.h` vs `type.cc`), some header-only ones have no source file, find sources by this pattern first.

**All** OOM errors should be handled and reported to the caller elegantly, aborting or suppressing errors are **ABSOLUTELY UNACCEPTABLE**, also, the library chosen for this project must support OOM error reporting (no matter if using exceptions or error codes, for former we allow to catch exceptions and convert them into errors but spreading it out is unallowed). If you are designing an API, prefer error codes/error objects.

DO NOT USE standard library containers involving memory allocations such as `std::vector`, `std::string`, `std::map` or any other container using memory allocations. Always use containers from PEFF (such as `peff::DynArray`, `peff::String`, `peff::Map`) instead. `std::function` should be noticed and prohibited because it will allocate memory unexpectly in some implementations and you cannot control the details of allocation. Use interface + virtual method mode or use generics instead if you have to use `std::function`. Also notice the types introducing indirect allocation details you cannot control such as `std::default_deleter` and `std::shared_ptr`. Containers without dynamic memory allocation are allowed, e.g. `std::bitset`, `std::string_view`, `std::span` and `std::array`.

Provide interfaces allowing the user to specify allocator to be used in the APIs. If it's not viable, stop immediately and let me make decisions.

DO NOT use primitive `new` and `delete` directly unless you are implementing an allocator providing primitve allocating and releasing functions. Placement new is allowed because it does not allocate memory.

Coroutine frame allocations also must not violate the memory management rules (you can rewrite the `operator new` and `operator delete` of the `promise_type` to override the default allocating and deallocating behaviors).

For C++ libraries, find library paths by locating variables named `<Library name>_DIR` in `CMakeCache.txt` in the build directory. You should use tools like `grep` to perform it unless you were fail to find. Variable definitions are like `peff_DIR=/path/to/library`.

If something was fail to perform, find another way to perform it but avoid risky operations, or report to me if it's unviable or you need my decision.

If you found that something may recurse too many times, you should use the coroutine to convert it into a recursive coroutine to make it iterative in execution even if it is logically recursive. This technique can be used for any recursive function but requires an extra scheduler to eliminate it (see coroutine implementations in `slkc/ast/rgtree.h` and `slkc/comp/rg2ast.h` for examples).

If you found your design/next operations/external libraries will violate the rules, stop and ask me to make decisions or let me give you suggestions or instructions.

## Project Status

We have three kinds of IR for compilation, SGIR (Scope Graph IR), RGIR (Red-Green IR) and TCIR (Type Checker IR).

* SGIR is the most frequent used IR which is designed for organizating the sources, it includes scope member nodes (classes, functions, interfaces, etc) and almost all control flow structures (e.g. expressions and statements), itself is generated from RGIR and cacheable.
  * you may `.pin()` it back to memory while it is cached but you must check if the pinning was fail by using `.is_fail()` on returned pinned reference, use `.get_fail_reason()` to get the error code), `AstNodePin` contains a pinned strong reference with in-memory guaranteed during its lifetime.
  * `AstNodeWeakPtr` contains a weak reference to a node, you cannot convert it into a pin directly but can convert it into a strong reference first by `.reclaim()`
* RGIR is a red-green tree based IR which is generated by the parser, its main purpose is to support incremental building of the SGIR and serve the language server, the green nodes are cacheable while the red nodes are always in the memory to mitigate the burden of maintenance and improve the usefulness.
* TCIR is dedicated on type checking, most of the type operations are based on this kind of IR, it is cacheable.

The project status is currently like following:

* `slkc/` contains the entire compiler executable's implementations.
  * `slkc/ast/` contains the AST framework's definitions, which defines the RGIR and SGIR types.
    * `slkc/ast/nodedefs/` contains the definitions of the node types in the SGIR.
    * `slkc/ast/parser/` contains implementation codes of parser that generates RGIR from source codes. We use coroutines to eliminate recursions.
  * `slkc/comp` contains definitions and implementations of the compilation and semantic analysis framework.
* `cmake/` contains files related to CMake, such as CMake package definitions used by the project or CMake sources used during CMake installation.

## Building

Use `cmake -S . -B build` to configure the project and use `cmake --build build --target <Target you wanted> --config <Configuration type you wanted> ` to build a target with configuration type you wanted.

For GCC/Clang, you can set `ENABLE_ADDRESS_SANITIZER` cache variable of CMake to control if to enable AddressSanitizer. For MSVC, set `addressSanitizerEnabled` field of the configuration in `CMakeSettings.json` to control if the AddressSanitizer is enabled.

## Tricks

* When a C++ template error occurs, you must locate the line with the error marker such as `error:` rapidly and search around (forward or backward, only one direction is valid which is determined by the compiler, search with another direction if you found that the information is not related to the error) the line to confirm the type of error and the context.
