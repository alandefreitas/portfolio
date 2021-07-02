//
// Created by Alan Freitas on 12/7/20.
//

#ifndef PORTFOLIO_PORTFOLIO_H
#define PORTFOLIO_PORTFOLIO_H

#include "market_data.h"
#include "portfolio_mad.h"
#include <map>
#include <optional>
#include <ostream>
#include <string>
namespace portfolio {
    class portfolio {
      public:
        explicit portfolio(const market_data &data);
        /// @brief Evaluate portfolio using MAD as risk measure.
        /// \param data Market data of assets.
        /// \return Risk and expected return of the portfolio.
        std::pair<double, double> evaluate_mad(const market_data &data);
        friend std::ostream &operator<<(std::ostream &os,
                                        const portfolio &portfolio1);

        void mutation(market_data &p, double mutation_strength);
        portfolio crossover(const market_data &data, portfolio &rhs);
        double distance(market_data &data, portfolio &rhs,
                        double max_dist = std::numeric_limits<double>::max());
        [[nodiscard]] bool invariants() const;

      private:
        void normalize_allocation();
        [[nodiscard]] double total_allocation() const;

        std::optional<portfolio_mad> mad_;
        std::map<std::string, double> assets_proportions_;
        double lower_bound_;
        double upper_bound_;
        size_t k_;
    };
} // namespace portfolio
#endif // PORTFOLIO_PORTFOLIO_H
