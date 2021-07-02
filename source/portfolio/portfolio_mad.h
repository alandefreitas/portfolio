//
// Created by eduardo on 15/04/2021.
//

#ifndef PORTFOLIO_PORTFOLIO_MAD_H
#define PORTFOLIO_PORTFOLIO_MAD_H

#include "market_data.h"

namespace portfolio {
    class portfolio_mad {
      public:
        /// @brief Class constructor
        /// \param data Market_data used for calculating MAD.
        /// \param interval Interval of the last price record for which you want
        /// to calculate the MAD. \param n_periods Number of periods used for
        /// calculating MAD.
        portfolio_mad(const market_data &data);

        /// @brief Gets interval used for calculating MAD.
        /// \return Interval_points of portfolio_mad.
        [[nodiscard]] interval_points interval() const;

        /// @brief Gets number of periods used for calculating MAD.
        /// \return The number of periods of portfolio_mad.
        [[nodiscard]] int n_periods() const;

        /// @brief Gets calculated risk of an asset.
        /// \param asset Asset code.
        /// \return The risk of adjustment using MAD.
        [[nodiscard]] double risk(std::string_view asset) const;

        /// @brief Gets the expected return on the asset based on the average of
        /// past returns. \param asset Asset code. \return The expected return
        /// or mean of past returns.
        [[nodiscard]] double expected_return(std::string_view asset) const;

      private:
        interval_points interval_;
        int n_periods_;
        std::map<std::string, std::pair<double, double>> assets_risk_return_;
    };
} // namespace portfolio
#endif // PORTFOLIO_PORTFOLIO_MAD_H
