//
// Created by eduar on 10/06/2021.
//

#include "random_search.h"
#include <pareto/common/default_allocator.h>
#include <pareto/matplot/front.h>

namespace portfolio {

    random_search::random_search(market_data &m) : market_(m) {
        pareto_front_ =
            pareto::front<double, 2, portfolio>({pareto::min, pareto::max});
        n_random_ = 1000;
    }
    void random_search::run() {
        for (size_t i = 0; i < n_random_; ++i) {
            portfolio p(market_);
            std::pair<double, double> risk_return =
                p.evaluate_mad(this->market_);
            pareto_front_.insert(
                std::make_pair(pareto::front<double, 2, portfolio>::key_type(
                                   {risk_return.first, risk_return.second}),
                               p));
        }
    }
    std::ostream &operator<<(std::ostream &os, const random_search &search) {
        for (const auto &[point, value] : search.pareto_front_) {
            os << point << " -> " << value << "\n";
        }
        return os;
    }
    void random_search::plot() {
        auto now = std::chrono::system_clock::now();
        std::string str_now = date::format("%Y_%m_%d_%H_%M", now);
        std::string file_name;
        file_name.append("pareto_");
        file_name.append(std::to_string(n_random_));
        file_name.append("_");
        file_name.append(str_now);
        file_name.append(".jpg");
        std::cout << file_name << std::endl;
        pareto::plot_front(pareto_front_);
        matplot::save(file_name);
    }
    void random_search::run(size_t n_random) {
        this->n_random_ = n_random;
        this->run();
    }
} // namespace portfolio
