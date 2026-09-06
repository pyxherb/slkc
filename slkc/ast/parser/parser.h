#ifndef _SLKC_AST_PARSER_PARSER_H_
#define _SLKC_AST_PARSER_PARSER_H_

#include "lexer.h"
#include <slkc/ast/nodedefs.h>
#include <coroutine>

namespace slkc {
	namespace ast {
		class ParseCoroutineScheduler;

		class Parser;

		enum class SyntaxErrorKind : int {
			OutOfMemory = 0,
			ExpectingMoreTokens,
			UnexpectedToken,
			ExpectingSingleToken,
			ExpectingTokens,
			ExpectingId,
			ExpectingOperatorName,
			ExpectingExpr,
			ExpectingStmt,
			ExpectingDecl,
			InvalidMetaTypeName,
			NoMatchingTokensFound,
			ConflictingDefinitions,
			LiteralOverflowed,
		};

		struct ExpectingSingleTokenErrorExData {
			uint32_t expecting_token_id;
		};

		struct ExpectingTokensErrorExData {
			peff::Set<uint32_t> expecting_token_ids;

			SLAKE_FORCEINLINE ExpectingTokensErrorExData(peff::Alloc *allocator) : expecting_token_ids(allocator) {
			}
		};

		struct NoMatchingTokensFoundErrorExData {
			peff::Set<uint32_t> expecting_token_ids;

			SLAKE_FORCEINLINE NoMatchingTokensFoundErrorExData(peff::Alloc *allocator) : expecting_token_ids(allocator) {
			}
		};

		struct ConflictingDefinitionsErrorExData {
			peff::String member_name;

			SLAKE_FORCEINLINE ConflictingDefinitionsErrorExData(peff::String &&name) : member_name(std::move(name)) {
			}
		};

		struct SyntaxError {
			TokenRange token_range;
			SyntaxErrorKind error_kind;
			std::variant<std::monostate, ExpectingTokensErrorExData, NoMatchingTokensFoundErrorExData, ExpectingSingleTokenErrorExData, ConflictingDefinitionsErrorExData> ex_data;

			SLAKE_FORCEINLINE SyntaxError(
				const TokenRange &token_range,
				SyntaxErrorKind error_kind)
				: token_range(token_range),
				  error_kind(error_kind) {
			}

			SLAKE_FORCEINLINE SyntaxError(
				const TokenRange &token_range,
				ExpectingTokensErrorExData &&ex_data)
				: token_range(token_range),
				  error_kind(SyntaxErrorKind::ExpectingTokens),
				  ex_data(std::move(ex_data)) {
			}

			SLAKE_FORCEINLINE SyntaxError(
				const TokenRange &token_range,
				ExpectingSingleTokenErrorExData &&ex_data)
				: token_range(token_range),
				  error_kind(SyntaxErrorKind::ExpectingSingleToken),
				  ex_data(std::move(ex_data)) {
			}

			SLAKE_FORCEINLINE SyntaxError(
				const TokenRange &token_range,
				NoMatchingTokensFoundErrorExData &&ex_data)
				: token_range(token_range),
				  error_kind(SyntaxErrorKind::NoMatchingTokensFound),
				  ex_data(std::move(ex_data)) {
			}

			SLAKE_FORCEINLINE SyntaxError(
				const TokenRange &token_range,
				ConflictingDefinitionsErrorExData &&ex_data)
				: token_range(token_range),
				  error_kind(SyntaxErrorKind::ConflictingDefinitions),
				  ex_data(std::move(ex_data)) {
			}

			SLAKE_FORCEINLINE ExpectingTokensErrorExData &get_expecting_tokens_error_ex_data() {
				return std::get<ExpectingTokensErrorExData>(ex_data);
			}

			SLAKE_FORCEINLINE const ExpectingTokensErrorExData &get_expecting_tokens_error_ex_data() const {
				return std::get<ExpectingTokensErrorExData>(ex_data);
			}

			SLAKE_FORCEINLINE const NoMatchingTokensFoundErrorExData &get_no_matching_tokens_found_error_ex_data() const {
				return std::get<NoMatchingTokensFoundErrorExData>(ex_data);
			}
		};

		enum class SyntaxWarningKind : int {
			ScopeOpIsOmittableInIdRef = 0,
		};

		struct SyntaxWarning {
			TokenRange token_range;
			SyntaxWarningKind warning_kind;
			std::variant<std::monostate> ex_data;

			SLAKE_FORCEINLINE SyntaxWarning(
				const TokenRange &token_range,
				SyntaxWarningKind warning_kind)
				: token_range(token_range),
				  warning_kind(warning_kind) {
			}
		};

		struct ParseCoroutine {
			struct promise_type;

			using Handle = std::coroutine_handle<promise_type>;

			struct promise_type {
				peff::Option<SyntaxError> result;

				SLAKE_FORCEINLINE static ParseCoroutine get_return_object_on_allocation_failure() noexcept {
					return ParseCoroutine({});
				}

				SLAKE_FORCEINLINE ParseCoroutine get_return_object() noexcept {
					return ParseCoroutine(Handle::from_promise(*this));
				}

				SLAKE_FORCEINLINE std::suspend_always initial_suspend() noexcept {
					return {};
				}

				SLAKE_FORCEINLINE std::suspend_always final_suspend() noexcept {
					return {};
				}

				SLAKE_FORCEINLINE std::suspend_always yield_value(peff::Option<SyntaxError> &&value) noexcept {
					result = std::move(value);
					return {};
				}

				SLAKE_FORCEINLINE void return_value(peff::Option<SyntaxError> &&value) noexcept {
					result = std::move(value);
				}

				SLAKE_FORCEINLINE void unhandled_exception() { std::terminate(); }

				struct AllocatorInfo {
					peff::Alloc *allocator;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					size_t c;
#endif
				};

				template <typename First, typename... Args>
				SLAKE_FORCEINLINE static void *operator new(size_t size, First &&first, peff::Alloc *allocator, Args &&...args) noexcept {
					char *p = (char *)allocator->alloc(size + sizeof(AllocatorInfo), alignof(std::max_align_t));

					if (!p)
						return nullptr;

					memset(p, 0, size);

#if PEFF_ENABLE_RCOBJ_DEBUGGING
					auto ref_count = peff::acquire_global_rcobj_ptr_counter();
#endif

					AllocatorInfo allocator_info = {
						allocator
#if PEFF_ENABLE_RCOBJ_DEBUGGING
						,
						ref_count
#endif
					};

					memcpy(p + size, &allocator_info, sizeof(allocator_info));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					allocator->inc_ref(ref_count);
#endif

					return p;
				}

				SLAKE_FORCEINLINE static void operator delete(void *p, size_t size) noexcept {
					AllocatorInfo allocator_info;

					memcpy(&allocator_info, (char *)p + size, sizeof(allocator_info));

					peff::RcObjectPtr<peff::Alloc> allocator_holder = allocator_info.allocator;

					allocator_holder->release(p, size + sizeof(AllocatorInfo), alignof(std::max_align_t));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					allocator_holder->dec_ref(allocator_info.c);
#endif
				}
			};

			Handle coro_handle;

			static inline bool recursed = false;

			ParseCoroutine(Handle coro_handle) : coro_handle(coro_handle) {}
			~ParseCoroutine() {
				// assert(!recursed);
				// recursed = true;
				if (coro_handle)
					coro_handle.destroy();
				// recursed = false;
			}

			SLAKE_FORCEINLINE bool done() {
				return coro_handle.done();
			}

			SLAKE_API peff::Option<SyntaxError> resume(Parser *parser);

			struct Awaitable {
				ParseCoroutine &co;
				Parser *parser;
				ParseCoroutineScheduler *scheduler;
				Handle handle;

				SLKC_API Awaitable(ParseCoroutine &co, Parser *parser, ParseCoroutineScheduler *scheduler, Handle handle);
				SLKC_API bool await_ready();
				SLKC_API void await_suspend(Handle h);
				[[nodiscard]] SLKC_API peff::Option<SyntaxError> await_resume();
			};

			SLKC_API Awaitable operator()(Parser *parser);
		};

		class ParseCoroutineScheduler {
		public:
			peff::DynArray<std::coroutine_handle<ParseCoroutine::promise_type>> task_list;

			SLKC_API ParseCoroutineScheduler(peff::Alloc *allocator);
		};

		class Parser {
		public:
			ParseCoroutineScheduler parse_coro_scheduler;

			Global *global;

			AstNodePtr<AstNodeIndex> module_node;
			TokenList token_list;

			struct ParseContext {
				TokenIndex idx_prev_token = 0, idx_current_token = 0;
			};
			ParseContext parse_context;

			peff::DynArray<SyntaxError> syntax_errors;
			peff::DynArray<SyntaxWarning> syntax_warnings;

			SLKC_API Parser(Global *global, TokenList &&token_list, peff::Alloc *resource_allocator);
			SLKC_API virtual ~Parser();

			SLAKE_FORCEINLINE Global *get_global() const noexcept {
				return global;
			}

			SLAKE_FORCEINLINE SyntaxError gen_oom_syntax_error() const noexcept {
				return SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), 0 }, SyntaxErrorKind::OutOfMemory);
			}

			enum class TokenIgnoringPolicy : uint8_t {
				Ignore = 0,
				Keep
			};

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> to_next_token(const GreenNodePin &parent_node, TokenIgnoringPolicy keep_new_line = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_whitespace = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_comment = TokenIgnoringPolicy::Ignore);
			// SLKC_API void next_token();
			SLKC_API peff::Option<SyntaxError> collect_token(const GreenNodePin &parent_node);
			SLKC_API Token *peek_token(TokenIgnoringPolicy keep_new_line = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_whitespace = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_comment = TokenIgnoringPolicy::Ignore);
			SLKC_API peff::Option<SyntaxError> collect_and_next_token(const GreenNodePin &parent_node, TokenIgnoringPolicy keep_new_line = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_whitespace = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_comment = TokenIgnoringPolicy::Ignore);
			SLKC_API peff::Option<SyntaxError> collect_and_expect_token(const GreenNodePin &parent_node, TokenKind token_kind, TokenIgnoringPolicy keep_new_line = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_whitespace = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_comment = TokenIgnoringPolicy::Ignore);
			SLKC_API peff::Option<SyntaxError> collect_to_cur_token(const GreenNodePin &parent_node, TokenIgnoringPolicy keep_new_line = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_whitespace = TokenIgnoringPolicy::Ignore, TokenIgnoringPolicy keep_comment = TokenIgnoringPolicy::Ignore);

			[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expect_token(Token *token, TokenKind token_kind) {
				if (token->token_id != token_kind) {
					ExpectingSingleTokenErrorExData ex_data = { token_kind };

					return SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), token->index }, std::move(ex_data));
				}

				return peff::NULLOPT;
			}

			[[nodiscard]] SLAKE_FORCEINLINE peff::Option<SyntaxError> expect_token(Token *token) {
				if (token->token_id == TokenId::End) {
					ExpectingTokensErrorExData ex_data(global->get_allocator());

					return SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), token->index }, std::move(ex_data));
				}

				return peff::NULLOPT;
			}

			SLAKE_FORCEINLINE peff::Option<SyntaxError> push_literal_overflowed_error(Token *token) noexcept {
				if (!syntax_errors.push_back(SyntaxError(TokenRange{ get_global()->get_root_module_node_index(), token->index }, SyntaxErrorKind::LiteralOverflowed)))
					return gen_oom_syntax_error();
				return peff::NULLOPT;
			}

			[[nodiscard]] SLKC_API peff::Option<SyntaxError> split_shr_op_token();
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> split_rdbrackets_token();

		private:
			[[nodiscard]] SLKC_API ParseCoroutine parse_var_binding(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out, bool allow_mutability);
			[[nodiscard]] SLKC_API ParseCoroutine parse_var_binding_list(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out, bool allow_mutability);

			[[nodiscard]] SLKC_API ParseCoroutine parse_args(peff::Alloc *allocator, const GreenNodePin &args_node_out, TokenKind terminal_token, TokenKind separator_token);
			[[nodiscard]] SLKC_API ParseCoroutine parse_subscript_args(peff::Alloc *allocator, const GreenNodePin &args_node_out);

			[[nodiscard]] SLKC_API ParseCoroutine parse_type_name(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);
			[[nodiscard]] SLKC_API ParseCoroutine parse_id_ref_entry(peff::Alloc *allocator, const GreenNodePin &id_ref_entry_node_out, bool requires_generic_distinguisher);
			[[nodiscard]] SLKC_API ParseCoroutine parse_id_ref(peff::Alloc *allocator, const GreenNodePin &id_ref_node_out, bool requires_generic_distinguisher);

			[[nodiscard]] SLKC_API ParseCoroutine parse_expr(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out, int precedence);
			[[nodiscard]] SLKC_API ParseCoroutine parse_stmt(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);

			[[nodiscard]] SLKC_API ParseCoroutine parse_inheritance_slot(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);
			[[nodiscard]] SLKC_API ParseCoroutine parse_impl_item(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);
			[[nodiscard]] SLKC_API ParseCoroutine parse_impl_list(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);

			[[nodiscard]] SLKC_API ParseCoroutine parse_operator_name(peff::Alloc *allocator, const GreenNodePin &parent_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_fn(peff::Alloc *allocator, const GreenNodePin &fn_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_class(peff::Alloc *allocator, const GreenNodePin &cls_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_interface(peff::Alloc *allocator, const GreenNodePin &interface_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_trait(peff::Alloc *allocator, const GreenNodePin &trait_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_except(peff::Alloc *allocator, const GreenNodePin &except_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_struct(peff::Alloc *allocator, const GreenNodePin &struct_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_const_enum(peff::Alloc *allocator, const GreenNodePin &enum_node);
			[[nodiscard]] SLKC_API ParseCoroutine parse_scoped_enum(peff::Alloc *allocator, const GreenNodePin &enum_node);
			[[nodiscard]] SLKC_API ParseCoroutine parse_const_and_scoped_enum_item(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);
			[[nodiscard]] SLKC_API ParseCoroutine parse_union_enum(peff::Alloc *allocator, const GreenNodePin &enum_node);
			[[nodiscard]] SLKC_API ParseCoroutine parse_union_enum_case(peff::Alloc *allocator, GreenNodePin parent, GreenNodePin *node_pin_out);

			[[nodiscard]] SLKC_API ParseCoroutine parse_program_stmt(peff::Alloc *allocator, const GreenNodePin &module_node);

			[[nodiscard]] SLKC_API ParseCoroutine parse_program(peff::Alloc *allocator, const GreenNodePin &module_node);

		public:
			[[nodiscard]] SLKC_API peff::Option<SyntaxError> parse(const GreenNodePin &module_node);
		};
	}
}

#define SLKC_CO_RETURN_IF_PARSE_ERROR(expr)          \
	do {                                             \
		if (peff::Option<SyntaxError> _ = (expr); _) \
			co_return std::move(_).value();          \
	} while (0)

#define SLKC_PUSH_IF_PARSE_ERROR(list, expr)             \
	do {                                                 \
		if (peff::Option<SyntaxError> _ = (expr); _) {   \
			if (!(list).push_back(std::move(_).value())) \
				co_return gen_oom_syntax_error();        \
		}                                                \
	} while (0)

#define SLKC_CO_RETURN_IF_CO_AWAIT_ERROR(expr)                \
	do {                                                      \
		if (peff::Option<SyntaxError> _ = co_await (expr); _) \
			co_return std::move(_).value();                   \
	} while (0)

#define SLKC_RETURN_IF_PARSE_ERROR(expr)             \
	do {                                             \
		if (peff::Option<SyntaxError> _ = (expr); _) \
			return std::move(_).value();             \
	} while (0)

#define SLKC_CO_RETURN_IF_PUSH_RGNODE_FAILED(dest, subnode, ...) \
	if (!(dest)->push_child(subnode))                            \
		co_return gen_oom_syntax_error();

#endif
