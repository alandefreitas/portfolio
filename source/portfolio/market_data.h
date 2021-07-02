//
// Created by Alan Freitas on 12/7/20.
//

#ifndef PORTFOLIO_MARKET_DATA_H
#define PORTFOLIO_MARKET_DATA_H
#include "portfolio/data_feed/data_feed.h"
#include "portfolio/data_feed/data_feed_result.h"
#include <map>
#include <ostream>
#include <string_view>
#include <vector>
namespace portfolio {
    class market_data {
      public:
        market_data(const std::vector<std::string> &asset_list, data_feed &df,
                    minute_point start_period, minute_point end_period,
                    timeframe tf, interval_points interval, size_t n_periods);
        [[nodiscard]] std::map<std::string,
                               portfolio::data_feed_result>::const_iterator
        assets_map_begin() const;
        [[nodiscard]] std::map<std::string,
                               portfolio::data_feed_result>::const_iterator
        assets_map_end() const;
        [[nodiscard]] bool contains(std::string_view asset) const;
        friend std::ostream &operator<<(std::ostream &os,
                                        const market_data &data);
        [[nodiscard]] size_t size() const;
        const interval_points &interval() const;
        void interval(const interval_points &interval);
        size_t n_periods() const;
        void n_periods(size_t nPeriods);

      private:
        std::map<std::string, portfolio::data_feed_result> assets_map_;
        data_feed &data_feed_;
        interval_points interval_;
        size_t n_periods_;
    };
} // namespace portfolio

#endif // PORTFOLIO_MARKET_DATA_H
