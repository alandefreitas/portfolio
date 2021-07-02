//
// Created by eduardo on 15/04/2021.
//

#include "portfolio_mad.h"
#include "portfolio/common/algorithm.h"
#include <iostream>
#include <numeric>
#include <random>
#include <range/v3/core.hpp>
#include <range/v3/numeric/accumulate.hpp>
namespace portfolio {
    portfolio_mad::portfolio_mad(const market_data &data) {
        this->interval_= data.interval();
        this->n_periods_= data.n_periods();
        for (auto a = data.assets_map_begin(); a != data.assets_map_end();
             ++a) {
            data_feed_result df = a->second;
            auto price_it = df.find_prices_from(interval_);
            if (price_it == df.end()) {
                throw std::runtime_error("MAD_PORTFOLIO constructor error: "
                                         "interval not found for asset " +
                                         a->first);
            }
            for (size_t i = 0; i < n_periods_; ++i) {
                if (price_it != df.begin())
                    price_it--;
                else
                    throw std::runtime_error(
                        "MAD_PORTFOLIO constructor error: "
                        "n_periods out of market_data for asset: " +
                        a->first);
            }
            std::vector<double> asset_returns;
            for (size_t i = 0; i < n_periods_; ++i) {
                double price_0 = price_it->second.close();
                price_it++;
                double price_1 = price_it->second.close();
                asset_returns.push_back((price_1 - price_0) / price_0);
            }
            double mean =
                ranges::accumulate(asset_returns, 0.0) / asset_returns.size();
            double mad = ranges::accumulate(asset_returns, 0.0,
                                            [mean](double mad, double d) {
                                                return mad + std::abs(d - mean);
                                            }) /
                         asset_returns.size();
            assets_risk_return_[a->first] = std::make_pair(mad, mean);
        }
    }
    interval_points portfolio_mad::interval() const { return interval_; }
    int portfolio_mad::n_periods() const { return n_periods_; }
    double portfolio_mad::risk(std::string_view asset) const {
        return assets_risk_return_.at(std::string(asset)).first;
    }
    double portfolio_mad::expected_return(std::string_view asset) const {
        return assets_risk_return_.at(std::string(asset)).second;
    }

} // namespace portfolio