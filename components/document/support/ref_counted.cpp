#include "ref_counted.hpp"

#include <stdexcept>
#include <cstdio>
#include <cstdlib>

#include <string>

#ifdef _MSC_VER
#include "asprintf.h"
#endif

namespace document {

void release(const ref_counted_t *r) noexcept {
    if (r) r->release_();
}

void assign_ref(ref_counted_t* &dst, ref_counted_t *src) noexcept {
    ref_counted_t *old_value = dst;
    if (_usually_true(src != old_value)) {
        if (src) src->retain_();
        dst = src;
        if (old_value) old_value->release_();
    }
}

static void fail(const ref_counted_t *obj, const char *what, int ref_count, bool and_throw =true)
{
    char *message;
    asprintf(&message,
             "ref_counted_t object <%s @ %p> %s while it had an invalid ref_count of %d (0x%x)",
             typeid(*obj).name(), static_cast<const void*>(obj), what, ref_count, ref_count);
#ifdef WarnError
    WarnError("%s", message);
#else
    fprintf(stderr, "WARNING: %s\n", message);
#endif
    if (and_throw) {
        std::string str(message);
        free(message);
        throw std::runtime_error(str);
    }
    free(message);
}


#if !DEBUG
void ref_counted_t::release_() const noexcept {
    if (--_ref_count <= 0)
        delete this;
}
#endif

int ref_counted_t::ref_count() const {
    return _ref_count;
}

ref_counted_t::ref_counted_t(const ref_counted_t &) {}

ref_counted_t::~ref_counted_t() {
    int32_t old_ref = _ref_count.exchange(-9999999);
    if (_usually_false(old_ref != 0)) {
#if DEBUG
        if (old_ref != careful_initial_ref_count)
#endif
        {
            fail(this, "destructed", old_ref, false);
        }
    }
}

void ref_counted_t::_careful_retain() const noexcept {
    auto old_ref = _ref_count++;
    if (old_ref == careful_initial_ref_count) {
        _ref_count = 1;
        return;
    }
    if (old_ref <= 0 || old_ref >= 10000000)
        fail(this, "retained", old_ref);
}

void ref_counted_t::_careful_release() const noexcept {
    auto old_ref = _ref_count--;
    if (old_ref <= 0 || old_ref >= 10000000)
        fail(this, "released", old_ref);
    if (old_ref == 1) delete this;
}

}
