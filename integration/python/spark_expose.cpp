#include "spark_expose.hpp"

#include <iostream>

namespace otterbrix {
    spark_expose::spark_expose() {

    }

    void spark_expose::output_message(std::string msg) {
        std::cout << "otterbrix->spark_expose->output_message:" << std::endl;
        std::cout << "the message is: " << msg << std::endl;
    }

    int spark_expose::two_sum(int a, int b) {
        return a + b;
    }

    int spark_expose::get_42() {
        return 42;
    }
}