//
// Created by eduar on 10/06/2021.
//

#ifndef PORTFOLIO_RANDOM_SEARCH_H
#define PORTFOLIO_RANDOM_SEARCH_H
#include "portfolio.h"
#include <ostream>
#include <pareto/front.h>
namespace portfolio {
    class random_search {
      public:
        random_search(market_data &m);
        void run();
        void run(size_t n_randon);
        friend std::ostream &operator<<(std::ostream &os,
                                        const random_search &search);
        void plot();

      private:
        pareto::front<double, 2, portfolio> pareto_front_;
        market_data market_;
        size_t n_random_;
    };
} // namespace portfolio
#endif // PORTFOLIO_RANDOM_SEARCH_H
