//
// Created by Alan Freitas on 12/7/20.
//

#include "data_feed_result.h"
#include <chrono>
#include <utility>
namespace portfolio {

    data_feed_result::data_feed_result(price_map historical_data)
        : historical_data_(std::move(historical_data)) {}

    price_iterator data_feed_result::end() { return historical_data_.end(); }

    bool data_feed_result::empty() { return historical_data_.empty(); }

    ohlc_prices data_feed_result::latest_prices() const {
        // return last price in historical data
        return historical_data_.rbegin()->second;
    }

    price_iterator
    data_feed_result::find_prices_from(interval_points interval) {
        return historical_data_.find(interval);
    }

    ohlc_prices data_feed_result::closest_prices(minute_point date_time) const {
        if (date_time <= historical_data_.begin()->first.first) {
            return historical_data_.begin()->second;
        } else if (date_time >= historical_data_.rbegin()->first.second) {
            return historical_data_.rbegin()->second;
        }
        auto it = historical_data_.begin();
        auto previous_it = it;
        while (date_time > it->first.second) {
            previous_it = it;
            it++;
        }
        if (date_time >= it->first.first && date_time <= it->first.second) {
            return it->second;
        } else {
            return previous_it->second;
        }
    }
    price_iterator data_feed_result::begin() {
        return historical_data_.begin();
    }
    bool data_feed_result::operator==(const data_feed_result &rhs) const {
        return historical_data_ == rhs.historical_data_;
    }
    bool data_feed_result::operator!=(const data_feed_result &rhs) const {
        return !(rhs == *this);
    }
    bool data_feed_result::operator<(const data_feed_result &rhs) const {
        return historical_data_.begin()->first.first <
               rhs.historical_data_.begin()->first.first;
    }
    bool data_feed_result::operator>(const data_feed_result &rhs) const {
        return rhs < *this;
    }
    bool data_feed_result::operator<=(const data_feed_result &rhs) const {
        return !(rhs < *this);
    }
    bool data_feed_result::operator>=(const data_feed_result &rhs) const {
        return !(*this < rhs);
    }
    data_feed_result &data_feed_result::operator=(const data_feed_result &rhs) {
        if (this == &rhs)
            return *this;
        this->historical_data_ = rhs.historical_data_;
        return *this;
    }
    data_feed_result::data_feed_result(const data_feed_result &rhs) {
        this->historical_data_ = rhs.historical_data_;
    }

    minute_point data_feed_result::start_point() const {
        return historical_data_.begin()->first.first;
    }
    minute_point data_feed_result::end_point() const {
        return historical_data_.rbegin()->first.second;
    }
} // namespace portfolio