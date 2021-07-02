//
// Created by Alan Freitas on 12/7/20.
//

#include "market_data.h"
#include "portfolio/common/algorithm.h"
#include "portfolio/data_feed/alphavantage_data_feed.h"
#include <ranges>
#include <utility>
namespace portfolio {

    market_data::market_data(const std::vector<std::string> &asset_list,
                             data_feed &df, minute_point start_period,
                             minute_point end_period, timeframe tf,
                             interval_points interval, size_t n_periods)
        : data_feed_(df), interval_(interval), n_periods_(n_periods) {
        for (auto &str : asset_list) {
            data_feed_result data =
                data_feed_.fetch(str, start_period, end_period, tf);
            assets_map_.emplace(str, std::move(data));
        }
    }
    std::map<std::string, portfolio::data_feed_result>::const_iterator
    market_data::assets_map_begin() const {
        return assets_map_.cbegin();
    }
    std::map<std::string, portfolio::data_feed_result>::const_iterator
    market_data::assets_map_end() const {
        return assets_map_.cend();
    }
    bool market_data::contains(std::string_view asset) const {
        return assets_map_.find(std::string(asset)) != assets_map_.end();
    }
    std::ostream &operator<<(std::ostream &os, const market_data &data) {
        os << "List of assets: \n";
        for (auto &[key, _] : data.assets_map_) {
            os << key << "\n";
        }
        using namespace date;
        os << "Start point: "
           << portfolio::minute_point_to_string(
                  data.assets_map_.begin()->second.start_point())
           << "\n";
        os << "End point: "
           << portfolio::minute_point_to_string(
                  data.assets_map_.begin()->second.end_point())
           << "\n";
        return os;
    }
    size_t market_data::size() const { return assets_map_.size(); }
    const interval_points &market_data::interval() const { return interval_; }
    void market_data::interval(const interval_points &interval) {
        interval_ = interval;
    }
    size_t market_data::n_periods() const { return n_periods_; }
    void market_data::n_periods(size_t nPeriods) { n_periods_ = nPeriods; }

} // namespace portfolio