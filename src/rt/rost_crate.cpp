
#include "rost_internal.h"

uintptr_t
rost_crate::get_image_base() const {
  return ((uintptr_t)this + image_base_off);
}

ptrdiff_t
rost_crate::get_relocation_diff() const {
  return ((uintptr_t)this - self_addr);
}

activate_glue_ty
rost_crate::get_activate_glue() const {
  return (activate_glue_ty) ((uintptr_t)this + activate_glue_off);
}

uintptr_t
rost_crate::get_exit_task_glue() const {
  return ((uintptr_t)this + exit_task_glue_off);
}

uintptr_t
rost_crate::get_unwind_glue() const {
  return ((uintptr_t)this + unwind_glue_off);
}

uintptr_t
rost_crate::get_yield_glue() const {
  return ((uintptr_t)this + yield_glue_off);
}

rost_crate::mem_area::mem_area(rost_dom *dom, uintptr_t pos, size_t sz)
  : dom(dom),
    base(pos),
    lim(pos + sz)
{
  dom->log(rost_log::MEM, "new mem_area [0x%" PRIxPTR ",0x%" PRIxPTR "]",
           base, lim);
}

rost_crate::mem_area
rost_crate::get_debug_info(rost_dom *dom) const {
  return mem_area(dom, ((uintptr_t)this + debug_info_off),
                  debug_info_sz);
}

rost_crate::mem_area
rost_crate::get_debug_abbrev(rost_dom *dom) const {
  return mem_area(dom, ((uintptr_t)this + debug_abbrev_off),
                  debug_abbrev_sz);
}

//
// Local Variables:
// mode: C++
// fill-column: 78;
// indent-tabs-mode: nil
// c-basic-offset: 4
// buffer-file-coding-system: utf-8-unix
// compile-command: "make -k -C .. 2>&1 | sed -e 's/\\/x\\//x:\\//g'";
// End: