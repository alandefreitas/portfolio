//
// Created by Alan Freitas on 12/7/20.
//

#ifndef PORTFOLIO_DATA_FEED_RESULT_H
#define PORTFOLIO_DATA_FEED_RESULT_H

#include "portfolio/core/ohlc_prices.h"
#include <chrono>
#include <date/date.h>
#include <map>
namespace portfolio {
    using minute_point = std::chrono::time_point<std::chrono::system_clock,
                                                 std::chrono::minutes>;
    using interval_points = std::pair<minute_point, minute_point>;
    using price_map = std::map<interval_points, ohlc_prices>;
    using price_iterator = price_map::iterator;
    class data_feed_result {
      public /* constructors */:
        bool operator==(const data_feed_result &rhs) const;
        bool operator!=(const data_feed_result &rhs) const;
        bool operator<(const data_feed_result &rhs) const;
        bool operator>(const data_feed_result &rhs) const;
        bool operator<=(const data_feed_result &rhs) const;
        bool operator>=(const data_feed_result &rhs) const;
        data_feed_result &operator=(const data_feed_result &rhs);
        data_feed_result(const data_feed_result &rhs);
        /// \brief Class constructor
        /// \param historical_data Asset data to be stored.
        explicit data_feed_result(price_map historical_data);

      public /* getters and setters */:
        /// \brief Get latest prices stored.
        /// \return Latest ohlc_prices stored.
        [[nodiscard]] ohlc_prices latest_prices() const;

        /// \brief Find ohlc_prices of a specific interval point.
        /// \param interval Interval point for searching.
        /// \return A iterator for price of interval or returns end() if not
        /// founded.
        price_iterator find_prices_from(interval_points interval);

        /// \brief Get a iterator for begin of price_map.
        /// \return A iterator for begin of price_map.
        price_iterator begin();

        /// \brief Get a iterator for end of price_map.
        /// \return A iterator for end of price_map.
        price_iterator end();

        /// \brief Find ohlc_prices of a closest minute_point.
        /// \param date_time Minute point for searching.
        /// \return A ohlc_price for closest price of date_time.
        [[nodiscard]] ohlc_prices closest_prices(minute_point date_time) const;

        /// \brief Check if data feed result is empty.
        /// \return If the data feed result is empty returns true or false
        /// otherwise.
        bool empty();

        minute_point start_point() const;

        minute_point end_point() const;

      private:
        price_map historical_data_;
    };
} // namespace portfolio

#endif // PORTFOLIO_DATA_FEED_RESULT_H
