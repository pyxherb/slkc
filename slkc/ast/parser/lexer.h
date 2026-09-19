#ifndef _SLKC_AST_LEXER_H_
#define _SLKC_AST_LEXER_H_

#include <slkc/ast/global.h>
#include <slake/runtime.h>
#include <peff/base/deallocable.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>

namespace slkc {
	namespace ast {
		struct SourcePosition {
			size_t line, column;

			SLAKE_FORCEINLINE SourcePosition() : line(SIZE_MAX), column(SIZE_MAX) {}
			SLAKE_FORCEINLINE SourcePosition(size_t line, size_t column) : line(line), column(column) {}

			SLAKE_FORCEINLINE bool operator<(const SourcePosition &loc) const {
				if (line < loc.line)
					return true;
				if (line > loc.line)
					return false;
				return column < loc.column;
			}

			SLAKE_FORCEINLINE bool operator>(const SourcePosition &loc) const {
				if (line > loc.line)
					return true;
				if (line < loc.line)
					return false;
				return column > loc.column;
			}

			SLAKE_FORCEINLINE bool operator==(const SourcePosition &loc) const {
				return (line == loc.line) && (column == loc.column);
			}

			SLAKE_FORCEINLINE bool operator>=(const SourcePosition &loc) const {
				return ((*this) == loc) || ((*this) > loc);
			}

			SLAKE_FORCEINLINE bool operator<=(const SourcePosition &loc) const {
				return ((*this) == loc) || ((*this) < loc);
			}
		};

		class ModuleNode;

		struct SourceLocation {
			AstNodeIndex module_node;
			SourcePosition begin_position, end_position;
		};

		class Lexer;

		namespace TokenId {
			enum {
				End = 0,

				Unknown,

				Comma,
				Question,
				Colon,
				Semicolon,
				LBracket,
				RBracket,
				LDBracket,
				RDBracket,
				LBrace,
				RBrace,
				LParenthesis,
				RParenthesis,
				At,
				Dot,
				HashTag,
				VarArg,

				ScopeOp,
				ReturnTypeOp,
				MatchOp,
				LAndOp,
				LOrOp,
				AddOp,
				SubOp,
				MulOp,
				DivOp,
				ModOp,
				AndOp,
				OrOp,
				XorOp,
				LNotOp,
				NotOp,
				AssignOp,
				AddAssignOp,
				SubAssignOp,
				MulAssignOp,
				DivAssignOp,
				ModAssignOp,
				AndAssignOp,
				OrAssignOp,
				XorAssignOp,
				ShlAssignOp,
				ShrAssignOp,
				StrictEqOp,
				StrictNeqOp,
				EqOp,
				NeqOp,
				ShlOp,
				ShrOp,
				LtEqOp,
				GtEqOp,
				LtOp,
				GtOp,
				CmpOp,
				DollarOp,

				AbstractKeyword,
				StackallocKeyword,
				AttributeKeyword,
				AsKeyword,
				AsyncKeyword,
				AutoKeyword,
				AwaitKeyword,
				BaseKeyword,
				BreakKeyword,
				CaseKeyword,
				CatchKeyword,
				ClassKeyword,
				ConstKeyword,
				ContinueKeyword,
				DeleteKeyword,
				DefKeyword,
				DefaultKeyword,
				DoKeyword,
				ElseKeyword,
				EnumKeyword,
				ExceptKeyword,
				FalseKeyword,
				FnKeyword,
				ForKeyword,
				FinalKeyword,
				FriendKeyword,
				IfKeyword,
				ImportKeyword,
				InKeyword,
				InterfaceKeyword,
				IsKeyword,
				LetKeyword,
				LocalKeyword,
				MacroKeyword,
				MatchKeyword,
				ModuleKeyword,
				MultiKeyword,
				MutableKeyword,
				NativeKeyword,
				NewKeyword,
				NullKeyword,
				OperatorKeyword,
				OutKeyword,
				OverrideKeyword,
				PublicKeyword,
				PrivateKeyword,
				ProtectedKeyword,
				ReadonlyKeyword,
				RefKeyword,
				RestrictKeyword,
				ReturnKeyword,
				StaticKeyword,
				StructKeyword,
				SwitchKeyword,
				SynchronizedKeyword,
				ThisKeyword,
				ThrowKeyword,
				TypeofKeyword,
				TraitKeyword,
				TransientKeyword,
				TrueKeyword,
				TryKeyword,
				TypenameKeyword,
				UsingKeyword,
				UnionKeyword,
				UnsafeKeyword,
				VarKeyword,
				VirtualKeyword,
				WhereKeyword,
				WhileKeyword,
				WithKeyword,
				YieldKeyword,

				I8TypeName,
				I16TypeName,
				I32TypeName,
				I64TypeName,
				ISizeTypeName,
				U8TypeName,
				U16TypeName,
				U32TypeName,
				U64TypeName,
				USizeTypeName,
				F32TypeName,
				F64TypeName,
				StringTypeName,
				BoolTypeName,
				VoidTypeName,
				ObjectTypeName,
				AnyTypeName,
				SIMDTypeName,
				NeverTypeName,

				I8Literal,
				I16Literal,
				I32Literal,
				I64Literal,
				U8Literal,
				U16Literal,
				U32Literal,
				U64Literal,
				F32Literal,
				F64Literal,
				StringLiteral,
				RawStringLiteral,

				Id,

				Whitespace,
				NewLine,
				LineComment,
				BlockComment,
				DocumentationComment,

				MaxToken
			};
		}

		class TokenExtension {
		public:
			SLKC_API virtual ~TokenExtension();

			virtual void dealloc() = 0;
		};

		enum class IntTokenType {
			Decimal = 0,
			Hexadecimal,
			Octal,
			Binary,
		};

		class IntTokenExtension : public TokenExtension {
		public:
			IntTokenType token_type;
			peff::RcObjectPtr<peff::Alloc> allocator;

			SLKC_API IntTokenExtension(peff::Alloc *allocator, IntTokenType token_type);
			SLKC_API virtual ~IntTokenExtension();

			SLKC_API virtual void dealloc() override;
		};

		class StringTokenExtension : public TokenExtension {
		public:
			GlobalSharedStringRef data;
			peff::RcObjectPtr<peff::Alloc> allocator;

			SLKC_API StringTokenExtension(peff::Alloc *allocator, GlobalSharedStringRef data);
			SLKC_API virtual ~StringTokenExtension();

			SLKC_API virtual void dealloc() override;
		};

		class Global;

		class Token {
		private:
			std::atomic_size_t _ref_count = 0;

		public:
			uint32_t token_id;
			peff::RcObjectPtr<peff::Alloc> allocator;
			GlobalSharedStringRef source_text;
			Global *global;
			SourceLocation source_location;
			std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>> ex_data;
			TokenIndex index = INVALID_TOKEN_INDEX;

			SLKC_API Token(peff::Alloc *allocator, Global *global);
			SLKC_API ~Token();

			SLKC_API void dealloc();

			SLAKE_FORCEINLINE void inc_ref(size_t ignored) noexcept {
				++_ref_count;
			}

			SLAKE_FORCEINLINE void dec_ref(size_t ignored) noexcept {
				if (!--_ref_count) {
					dealloc();
				}
			}
		};

		using TokenPtr = peff::RcObjectPtr<Token>;
		using TokenList = peff::DynArray<TokenPtr>;

		enum class LexicalErrorKind {
			UnrecognizedToken = 0,
			UnexpectedEndOfLine,
			PrematuredEndOfFile,
			InvalidEscape,
			OutOfMemory
		};

		struct LexicalError {
			SourceLocation location;
			LexicalErrorKind kind;
		};

		class Lexer {
		public:
			TokenList token_list;
			peff::Option<LexicalError> lexical_error;

			SLAKE_FORCEINLINE Lexer(peff::Alloc *allocator) : token_list(allocator) {
			}
			[[nodiscard]] SLKC_API peff::Option<LexicalError> lex(Global *global, AstNodeIndex module_node, const std::string_view &src, peff::Alloc *allocator);
		};

		SLKC_API std::string_view get_token_name(uint32_t token_id);
	}
}

#endif
