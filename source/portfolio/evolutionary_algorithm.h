//
// Created by eduar on 04/06/2021. Adapted from Alan de Freitas
//

#ifndef PORTFOLIO_EVOLUTIONARY_ALGORITHM_H
#define PORTFOLIO_EVOLUTIONARY_ALGORITHM_H
#include "market_data.h"
#include "portfolio.h"
#include <ostream>
#include <pareto/front.h>

namespace portfolio {

    class evolutionary_algorithm {
      public:
        // Auxiliary class for individuals
        class individual : public portfolio {
          public:
            /// Initialize a new portfolio making a copy of the portfolio
            /// received as a parameter. \param rhs portfolio used to copy.
            individual(portfolio &rhs) : portfolio(rhs) {}
            /// Initialize a new random portfolio using market_data.
            /// \param rhs market_data used to initialize a portfolio.
            individual(market_data &rhs) : portfolio(rhs) {}
            /// Initialize a new portfolio making a copy of the portfolio
            /// received as a parameter. \param rhs portfolio used to copy.
            individual(portfolio rhs) : portfolio(rhs) {}
            void mutation(market_data &market, double mutation_strength);
            individual crossover(market_data &p, individual &rhs);
            double second_order_mutation_strength_ = 0.1;
            double crossover_probability_ = 0.8;
            double mutation_strength_ = 0.1;
            double risk;
            double expected_return;
            double fx;
            double fitness;

          private:
            void reflect(double &v, double lb, double ub);
        };
        using solution_ptr = std::shared_ptr<portfolio>;
        using individual_ptr =
            std::shared_ptr<evolutionary_algorithm::individual>;
        using population_type = std::vector<individual_ptr>;

        // how solutions can be selected for reproduction or survival
        enum class selection_strategy {
            uniform,       // same probability to all solutions
            truncate,      // only the solutions with best fitness
            tournament,    // winners of tournaments between groups of solutions
            roulette,      // selection probability proportional to fitness
            sus,           // stochastic probability proportional to fitness
            overselection, // 80% of operations on the x% best solutions
            roundrobin_tournament // each individual evaluated against other q
                                  // individuals
        };

        // how fitness is determined from the objective functions
        enum class scaling_strategy {
            window, // fitness <- fx - worst_fx
            sigma,  // fitness <- max(bias + (fx - avg_fx)/(c * std_fx)), 0.0)
            linear_rank, // fitness <- ((2-pressure_constant)/n_ind) + ((2 *
                         // rank(fx) * (pressure_constant - 1))/n_ind * (n_ind -
                         // 1))
            exponential_rank // fitness <- (1-e^(-i))/n_ind
        };

        // the structure of the islands
        enum class island_structure {
            ring,  // each island is connected to the next
            random // each island is connected to another random island
        };

        // which individuals migrate from islands
        enum class island_migration_policy {
            best,         // the best individual moves to another island
            fittest_half, // the best individual moves to another island
            random        // a random individual moves to another island
        };

        // which individuals migrate from islands
        enum class island_replacement_policy {
            worst_swap,
            worst_overwrite,
            random_swap,
            random_overwrite,
        };

        enum class algorithm {
            DEFAULT, // Default configuration
            GA,      // Genetic Algorithm
            EE       // Evolutionary Strategy
        };

      public:
        explicit evolutionary_algorithm(market_data &m);

        void algorithm(enum algorithm alg);

        void run();

        void run(size_t iterations);

        [[nodiscard]] double best_fx() const;

        individual_ptr best_solution();

        typename population_type::iterator best_solutions_begin();

        typename population_type::iterator best_solutions_end();

        // Setters and getters
        // Population management
        size_t population_size();
        void population_size(size_t value);
        size_t number_of_islands();
        void number_of_islands(size_t value);
        island_structure island_structure();
        void island_structure(enum island_structure value);
        island_migration_policy island_migration_policy();
        void island_migration_policy(enum island_migration_policy value);
        island_replacement_policy island_replacement_policy();
        void island_replacement_policy(enum island_replacement_policy value);
        size_t migration_epoch();
        void migration_epoch(size_t value);
        size_t migration_size();
        void migration_size(size_t value);
        double fitness_sharing_niche_size();
        void fitness_sharing_niche_size(double value);
        // Stopping criteria
        size_t max_generations();
        void max_generations(size_t value);
        // Reproduction
        double children_proportion();
        void children_proportion(double value);
        double crossover_probability();
        void crossover_probability(double value);
        double mutation_strength();
        void mutation_strength(double value);
        // Selection
        bool competition_between_parents_and_children();
        void competition_between_parents_and_children(bool value);
        unsigned int tournament_size();
        void tournament_size(unsigned int value);
        double overselection_proportion();
        void overselection_proportion(double value);
        size_t roundrobin_tournament_size();
        void roundrobin_tournament_size(size_t value);
        double elitism_proportion();
        void elitism_proportion(double value);
        // scaling
        enum scaling_strategy reproduction_scaling_strategy();
        void reproduction_scaling_strategy(enum scaling_strategy value);
        enum selection_strategy reproduction_selection_strategy();
        void reproduction_selection_strategy(enum selection_strategy value);
        enum scaling_strategy survival_scaling_strategy();
        void survival_scaling_strategy(enum scaling_strategy value);
        enum selection_strategy survival_selection_strategy();
        void survival_selection_strategy(enum selection_strategy value);
        double sigma_bias();
        void sigma_bias(double value);
        double sigma_constant();
        void sigma_constant(double value);
        double linear_rank_selective_pressure();
        void linear_rank_selective_pressure(double value);

      private /* methods */:
        // main cycle
        // void evolutionary_cycle();
        void evolutionary_cycle();

        // helpers
        size_t n_of_selection_candidates();
        size_t size_of_elite_set();
        void display_status();

        // evaluation algorithms
        // void evaluate(population_type &population);
        void evaluate(population_type &population);

        // scaling algorithms
        void scaling(population_type &population, scaling_strategy s);

        void window_scaling(population_type &population);

        void sigma_scaling(population_type &population);

        void linear_rank_scaling(population_type &population);

        void exponential_rank_scaling(population_type &population);

        // fitness sharing
        void fitness_sharing(population_type &population);

        // selection algorithms
        population_type selection(population_type &population,
                                  size_t n_of_candidates, selection_strategy s);

        population_type uniform_selection(population_type &population,
                                          size_t n_of_candidates);

        population_type truncate_selection(population_type &population,
                                           size_t n_of_candidates);

        population_type tournament_selection(population_type &population,
                                             size_t n_of_candidates);

        population_type roulette_selection(population_type &population,
                                           size_t n_of_candidates);

        population_type sus_selection(population_type &population,
                                      size_t n_of_candidates);

        population_type overselection_selection(population_type &population,
                                                size_t n_of_candidates);

        population_type roundrobin_selection(population_type &population,
                                             size_t n_of_candidates);

        // reproduction algorithms
        population_type reproduction(population_type &parents);

        population_type merge(population_type &parents,
                              population_type &children);

        // update
        population_type insert_elite_set(population_type &individuals,
                                         population_type &elite_source,
                                         size_t size_of_elite_set);

        // void population_update_step(population_type &children);

        void initialize_population();

        void attribute_fitness_from_rank(population_type &population);

        population_type get_island(population_type &population, int idx);

        void try_to_update_best(individual_ptr &candidate);

        void migration_step();

      private /* members */:
        // Search parameters
        // Population management
        size_t population_size_ = 300;
        size_t number_of_islands_ = 5;
        enum island_structure island_structure_ = island_structure::random;
        enum island_migration_policy island_migration_policy_ =
            island_migration_policy::random;
        enum island_replacement_policy island_replacement_policy_ =
            island_replacement_policy::worst_swap;
        size_t migration_epoch_ = 25;
        size_t migration_size_ = 2;
        double fitness_sharing_niche_size_ = 5.0;
        // Stopping criteria
        size_t max_generations_ = 150;
        // Reproduction
        const size_t parents_per_children_ = 2;
        double children_proportion_ = 7.0;
        double crossover_probability_ = 0.9;
        double mutation_strength_ = 0.1;
        // Selection
        bool competition_between_parents_and_children_ = false;
        unsigned int tournament_size_ = 2;
        double overselection_proportion_ = 0.1;
        size_t roundrobin_tournament_size_ = 10;
        double elitism_proportion_ = 0.01;
        // scaling
        enum scaling_strategy reproduction_scaling_strategy_ =
            scaling_strategy::window;
        enum selection_strategy reproduction_selection_strategy_ =
            selection_strategy::tournament;
        enum scaling_strategy survival_scaling_strategy_ =
            scaling_strategy::window;
        enum selection_strategy survival_selection_strategy_ =
            selection_strategy::truncate;
        double sigma_bias_ = 1.0;
        double sigma_constant_ = 2.0;
        double linear_rank_selective_pressure_ = 1.5;

        // Data
        population_type population_;
        market_data problem_;
        size_t current_generation_ = 0;

        // Solution comparing
        std::function<bool(individual_ptr &, individual_ptr &)> comp_;
        std::function<bool(individual_ptr &, individual_ptr &)> not_comp_;
        std::function<bool(individual_ptr &, individual_ptr &)> comp_fitness_;
        std::function<bool(individual_ptr &, individual_ptr &)>
            not_comp_fitness_;

        // Get statistics
        population_type best_solutions_;
        size_t number_of_best_solutions_ = 10;

        // meus
      private:
        // std::vector<portfolio> result_;
        // portfolio best_portfolio_;
        // std::pair<double, double> best_risk_return_;
        double lambda_step_;
        double lambda_value_;
        pareto::front<double, 2, portfolio> pareto_front_;
        void display_status(double lambda_value);

      public:
        pareto::front<double, 2, portfolio> &pareto_front();

        friend std::ostream &
        operator<<(std::ostream &os, const evolutionary_algorithm &algorithm);
        double lambda_value() const;
        void lambda_value(double value);
        void plot();
    };

} // namespace portfolio
#endif // PORTFOLIO_EVOLUTIONARY_ALGORITHM_H
