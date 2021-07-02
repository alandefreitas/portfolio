//
// Created by Alan Freitas on 12/7/20.
//

#include "portfolio.h"
#include "portfolio/common/algorithm.h"
#include <iostream>
#include <random>
#include <range/v3/core.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <vector>
namespace portfolio {

    bool portfolio::invariants() const {
        size_t k = 0;
        bool in_bounds = true;
        for (auto &[key, value] : this->assets_proportions_) {
            if (!almost_equal(value, 0.0)) {
                k++;
                if (value < this->lower_bound_ || value > this->upper_bound_) {
                    in_bounds = false;
                }
            }
        }
        return almost_equal(this->total_allocation(), 1.0) && this->k_ == k &&
               in_bounds;
    }
    portfolio::portfolio(const market_data &data)
        : lower_bound_(0.08), upper_bound_(0.15), k_(10) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> d_real(lower_bound_,
                                                      upper_bound_);
        std::uniform_int_distribution<int> d_int(1, int(data.size()));
        std::vector<std::string> selected_assets;
        int aux = 0;
        for (auto a = data.assets_map_begin(); a != data.assets_map_end();
             ++a) {
            this->assets_proportions_[a->first] = 0.0;
        }
        while (selected_assets.size() < k_) {
            for (auto a = data.assets_map_begin(); a != data.assets_map_end();
                 ++a) {
                if (d_int(generator) <=
                    data.size() / k_) { // asset is selected or not
                    if (std::find(selected_assets.begin(),
                                  selected_assets.end(),
                                  a->first) == selected_assets.end()) {
                        assets_proportions_[a->first] = d_real(generator);
                        selected_assets.push_back(a->first);
                        if (selected_assets.size() == k_) {
                            break;
                        }
                    }
                } else {
                    if (aux == 0) {
                        assets_proportions_[a->first] = 0.0;
                    }
                }
            }
            if (selected_assets.size() < k_) {
                aux++;
            }
        }
        normalize_allocation();
    }
    void portfolio::normalize_allocation() {
        for (auto &[key, value] : assets_proportions_) {
            if (!almost_equal(value, 0.0)) {
                while (value < lower_bound_ || value > upper_bound_) {
                    if (value < lower_bound_) {
                        value = value + 2 * (lower_bound_ - value);
                    } else if (value > upper_bound_) {
                        value = value - 2 * (value - upper_bound_);
                    }
                }
                assets_proportions_[key] = value;
            }
        }
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
        } else {
            for (auto &[key, value] : assets_proportions_) {
                if (!almost_equal(value, 0.0)) {
                    assets_proportions_[key] = value / total;
                }
            }
        }
        normalize_allocation();
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
    std::pair<double, double> portfolio::evaluate_mad(const market_data &data) {
        if (mad_) {
            if (mad_->n_periods() != data.n_periods() ||
                mad_->interval() != data.interval()) {
                mad_.reset();
                mad_.emplace(data);
            }
        } else {
            mad_.emplace(data);
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
    void portfolio::mutation(market_data &p, double mutation_strength) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::normal_distribution<double> d(0, mutation_strength);
        for (auto &[key, value] : assets_proportions_) {
            if (!almost_equal(value, 0.0)) {
                this->assets_proportions_[key] = value + d(generator);
            }
        }
        this->normalize_allocation();
    }
    portfolio portfolio::crossover(const market_data &data, portfolio &rhs) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        portfolio child(data);
        std::vector<std::string> assets_this;
        std::vector<std::string> assets_rhs;
        std::vector<std::string> assets_total;
        std::vector<std::string> chosen_assets;
        for (auto &[key, value] : this->assets_proportions_) {
            if (!almost_equal(value, 0.0)) {
                assets_this.push_back(key);
            }
            if (!almost_equal(rhs.assets_proportions_[key], 0.0)) {
                assets_rhs.push_back(key);
            }
            child.assets_proportions_[key] = 0.0;
        }
        std::set_union(assets_this.begin(), assets_this.end(),
                       assets_rhs.begin(), assets_rhs.end(),
                       std::back_inserter(assets_total));
        std::sample(assets_total.begin(), assets_total.end(),
                    std::back_inserter(chosen_assets), k_,
                    std::mt19937{std::random_device{}()});
        for (auto &a : chosen_assets) {
            child.assets_proportions_[a] =
                this->assets_proportions_.at(a) + rhs.assets_proportions_.at(a);
        }
        child.normalize_allocation();
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