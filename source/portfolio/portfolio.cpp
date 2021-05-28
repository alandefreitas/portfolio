//
// Created by Alan Freitas on 12/7/20.
//

#include "portfolio.h"
#include "portfolio/common/algorithm.h"
#include <random>
#include <range/v3/core.hpp>
#include <range/v3/numeric/accumulate.hpp>

namespace portfolio {

    bool portfolio::invariants() const {
        double total = total_allocation();
        return almost_equal(total, 1.0);
    }
    portfolio::portfolio(const market_data &data) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> d_real(0.0, 1.0);
        std::binomial_distribution<int> d_binomial(1, 0.5);
        int count_assets = 0;
        while (count_assets == 0) {
            for (auto a = data.assets_map_begin(); a != data.assets_map_end();
                 ++a) {
                if (1 == d_binomial(generator)) { // asset is selected or not
                    assets_proportions_[a->first] = d_real(generator);
                    count_assets++;
                } else {
                    assets_proportions_[a->first] = 0.0;
                }
            }
        }

        normalize_allocation();
    }
    void portfolio::normalize_allocation() {
        double total = total_allocation();
        if (almost_equal(total, 1.0, 5)) {
            return;
        } else if (total > 1.0) {
            std::transform(
                assets_proportions_.begin(), assets_proportions_.end(),
                std::inserter(assets_proportions_, assets_proportions_.begin()),
                [&total](auto &p) {
                    p.second = p.second / total;
                    return p;
                });
            return;
        } else {
            for (auto &[key, value] : assets_proportions_) {
                if (!almost_equal(value, 0.0)) {
                    assets_proportions_[key] = value / total;
                }
            }
            return;
        }
    }
    double portfolio::total_allocation() const {
        double total;
        total = ranges::accumulate(
            std::begin(assets_proportions_), std::end(assets_proportions_), 0.0,
            [](double value,
               const std::map<std::string, double>::value_type &p) {
                return value + p.second;
            });
        return total;
    }
    std::pair<double, double> portfolio::evaluate_mad(const market_data &data,
                                                      interval_points interval,
                                                      int n_periods) {
        if (mad_) {
            if (mad_->n_periods() != n_periods ||
                mad_->interval() != interval) {
                mad_.reset();
                mad_.emplace(data, interval, n_periods);
            }
        } else {
            mad_.emplace(data, interval, n_periods);
        }
        double total_risk = 0.0;
        double total_return = 0.0;
        for (auto &a : assets_proportions_) {
            total_risk += a.second * mad_->risk(a.first);
            total_return += a.second * mad_->expected_return(a.first);
        }
        return std::make_pair(total_risk, total_return);
    }
    std::ostream &operator<<(std::ostream &os, const portfolio &portfolio1) {
        os << "Assets allocations:\n";
        for (auto &a : portfolio1.assets_proportions_) {
            if (!almost_equal(a.second, 0.0)) {
                os << "Asset: " << a.first << " - Allocation " << a.second;
                if (portfolio1.mad_) {
                    os << " - Expect return: "
                       << portfolio1.mad_->expected_return(a.first);
                    os << " - Risk: " << portfolio1.mad_->risk(a.first);
                }
                os << "\n";
            }
        }
        return os;
    }
    void portfolio::mutation(portfolio &p, double mutation_strength) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::normal_distribution<double> d(0, mutation_strength);
        for (auto &[key, _] : assets_proportions_) {
            this->assets_proportions_.at(key) += d(generator);
            normalize_allocation();
        }
    }
    portfolio portfolio::crossover(const market_data &data, portfolio &rhs) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> d(0., 1.);
        double alpha = d(generator);
        portfolio child(data);
        for (auto &[key, _] : assets_proportions_) {
            child.assets_proportions_[key] =
                alpha * this->assets_proportions_.at(key) +
                (1 - alpha) * rhs.assets_proportions_.at(key);
        }
        return child;
    }
    double portfolio::distance(market_data &data, portfolio &rhs,
                               double max_dist) {
        double total = 0.0;
        for (auto &[key, _] : assets_proportions_) {
            total += std::abs(rhs.assets_proportions_.at(key) -
                              assets_proportions_.at(key));
            if (total > max_dist) {
                return total;
            }
        }
        return total;
    }
} // namespace portfolio