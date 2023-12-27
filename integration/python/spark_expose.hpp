#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

namespace otterbrix {
    class PYBIND11_EXPORT spark_expose final {
    public:
        spark_expose();

        void output_message(std::string msg);
        int two_sum(int a, int b);
        int get_42();
    };
}