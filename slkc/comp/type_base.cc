#include "../global.h"

using namespace slkc;
using namespace slkc::comp;

SLKC_API void TypeDef::on_ref_zero() noexcept {
	this->get_global()->_clear_zero_ref_type_def_registry_list();
	this->get_global()->_add_type_def_to_deferred_deleting_list(this);
}
