//
// Created by eduar on 15/03/2021.
//

#include "algorithm.h"
#include <iomanip>
#include <stdexcept>
#include <vector>

namespace portfolio {

    double string_to_double(const std::string &str) {
        try {
            std::string::size_type sz; // alias of size_t
            return std::stod(str, &sz);
        } catch (std::invalid_argument &e) {
            throw std::runtime_error("Fatal conversion error: " +
                                     std::string(e.what()));
        }
    }

    std::string minute_point_to_string(minute_point mp) {
        return date::format("%Y-%m-%d_%H-%M", mp);
    }

    std::string interval_points_to_string(interval_points interval) {
        std::string str_start = minute_point_to_string(interval.first);
        std::string str_end = minute_point_to_string(interval.second);
        return str_start + "|" + str_end;
    }

    minute_point string_to_minute_point(std::string &str_mp) {
        std::tm tm = {};
        std::stringstream ss(str_mp);
        ss >> std::get_time(&tm, "%Y-%m-%d_%H-%M");
        std::chrono::system_clock::time_point tp =
            std::chrono::system_clock::from_time_t(mktime(&tm));
        using namespace std::chrono_literals;
        return std::chrono::floor<std::chrono::minutes>(tp);
    }

    interval_points string_to_interval_points(std::string str_interval) {
        std::replace(str_interval.begin(), str_interval.end(), '|', ' ');
        std::istringstream iss(str_interval);
        std::vector<std::string> result(std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>());
        minute_point start = string_to_minute_point(result[0]);
        minute_point end = string_to_minute_point(result[1]);
        return std::make_pair(start, end);
    }
    std::string set_filename(std::string_view asset_code,
                             std::string_view start_period,
                             std::string_view end_period, timeframe tf) {
        std::string filename{asset_code};
        switch (tf) {
        case (timeframe::monthly):
            filename += ".MONTHLY";
            break;
        case (timeframe::weekly):
            filename += ".WEEKLY";
            break;
        case (timeframe::daily):
            filename += ".DAILY";
            break;
        case (timeframe::hourly):
            filename += ".HOURLY";
            break;
        case (timeframe::minutes_15):
            filename += "MINUTES15";
        }
        filename += ".";
        filename += std::string{start_period};
        filename += ".";
        filename += std::string{end_period};
        filename += ".data";
        return filename;
    }
    std::string start_filename(std::string_view asset_code, timeframe tf) {
        std::string start_filename{asset_code};
        switch (tf) {
        case (timeframe::monthly):
            start_filename += ".MONTHLY";
            break;
        case (timeframe::weekly):
            start_filename += ".WEEKLY";
            break;
        case (timeframe::daily):
            start_filename += ".DAILY";
            break;
        case (timeframe::hourly):
            start_filename += ".HOURLY";
            break;
        case (timeframe::minutes_15):
            start_filename += "MINUTES15";
        }
        return start_filename;
    }

} // namespace portfolio