//
// Created by eduar on 04/06/2021.
//

#include "evolutionary_algorithm.h"
#include <iostream>
#include <matplot/matplot.h>
#include <numeric>
#include <pareto/matplot/front.h>
#include <random>

namespace portfolio {

    evolutionary_algorithm::evolutionary_algorithm(market_data &m)
        : problem_(m), mutation_strength_(1.0 / this->problem_.size()),
          lambda_step_(0.2), fitness_sharing_niche_size_(0.05 * m.size()) {
        this->algorithm(algorithm::DEFAULT);
        pareto_front_ =
            pareto::front<double, 2, portfolio>({pareto::min, pareto::max});
        this->lambda_value_ = 0.0;
        this->comp_ = [](individual_ptr &a, individual_ptr &b) {
            return a->fx < b->fx;
        };
        this->not_comp_ = [](individual_ptr &a, individual_ptr &b) {
            return a->fx > b->fx;
        };
        this->comp_fitness_ = [](individual_ptr &a, individual_ptr &b) {
            return a->fitness > b->fitness;
        };
        this->not_comp_fitness_ = [](individual_ptr &a, individual_ptr &b) {
            return a->fitness < b->fitness;
        };
    }
    void evolutionary_algorithm::run() {
        initialize_population();
        while (lambda_value_ < 1.0) {
            best_solutions_.clear();
            for (int i = 0; i < this->max_generations_; ++i) {
                evolutionary_cycle();
            }
            for (auto &solution : best_solutions_) {
                pareto_front_.insert(std::make_pair(
                    pareto::front<double, 2, portfolio>::key_type(
                        {solution->risk, solution->expected_return}),
                    *solution));
            }
            this->lambda_value_ += lambda_step_;
        }
    }
    void evolutionary_algorithm::run(size_t iterations) {
        initialize_population();
        double lambda_value = 0.0;
        while (lambda_value <= 1.0) {
            for (int i = 0; i < iterations; ++i) {
                evolutionary_cycle();
            }
            this->lambda_value_ += this->lambda_step_;
        }
    }
    void evolutionary_algorithm::evolutionary_cycle() {
        // display_status(lambda_value);
        evaluate(population_);
        scaling(this->population_, this->reproduction_scaling_strategy_);
        fitness_sharing(this->population_);
        population_type parents =
            selection(this->population_, n_of_selection_candidates(),
                      this->reproduction_selection_strategy_);
        population_type children = reproduction(parents);

        evaluate(children);

        const size_t size_of_elite_set = this->size_of_elite_set();
        population_type parents_and_children =
            this->merge(this->population_, children);
        population_type next_population_candidates =
            (this->competition_between_parents_and_children_ ||
             this->children_proportion_ < 1.0)
                ? parents_and_children
                : children;
        scaling(next_population_candidates, this->survival_scaling_strategy_);
        population_type survivors =
            selection(next_population_candidates,
                      this->population_size_ - size_of_elite_set,
                      this->survival_selection_strategy_);
        this->population_ = insert_elite_set(survivors, parents_and_children,
                                             size_of_elite_set);
        migration_step();
    }
    void evolutionary_algorithm::evaluate(population_type &population) {
        for (individual_ptr &item : population) {
            auto risk_return = item->evaluate_mad(this->problem_);
            item->risk = risk_return.first;
            item->expected_return = risk_return.second;
            item->fx = (this->lambda_value_ * item->risk) -
                       ((1.0 - this->lambda_value_) * item->expected_return);
            this->try_to_update_best(item);
        }
        //        for (individual_ptr &item : population) {
        //            // item.fx = item.evaluate(problem_,
        //            interval_,n_periods_); std::pair<double, double>
        //            risk_return =
        //                item.evaluate(problem_);
        //            item->risk = risk_return.first;
        //            item->expected_return = risk_return.second;
        //            item->fx = (lambda_value * item.risk) -
        //                      ((1.0 - lambda_value) * item->expected_return);
        //            if (item->fx < best_fx_) {
        //                pareto_front_.insert(std::make_pair(
        //                    pareto::front<double, 2, portfolio>::key_type(
        //                        {item->risk, item->expected_return}),
        //                    item->s));
        //                best_risk_return_ =
        //                    std::make_pair(item->risk, item->expected_return);
        //                best_fx_ = item->fx;
        //            }
        //        }
    }
    evolutionary_algorithm::population_type evolutionary_algorithm::selection(
        population_type &population, size_t n_of_candidates,
        evolutionary_algorithm::selection_strategy s) {
        population_type result;
        for (int i = 0; i < this->number_of_islands_; ++i) {
            population_type island = get_island(population, i);
            population_type island_result;
            size_t n_of_candidates_per_island =
                n_of_candidates / this->number_of_islands_;
            switch (s) {
            case selection_strategy::uniform:
                island_result =
                    uniform_selection(island, n_of_candidates_per_island);
                break;
            case selection_strategy::truncate:
                island_result =
                    truncate_selection(island, n_of_candidates_per_island);
                break;
            case selection_strategy::tournament:
                island_result =
                    tournament_selection(island, n_of_candidates_per_island);
                break;
            case selection_strategy::roulette:
                island_result =
                    roulette_selection(island, n_of_candidates_per_island);
                break;
            case selection_strategy::sus:
                island_result =
                    sus_selection(island, n_of_candidates_per_island);
                break;
            case selection_strategy::overselection:
                island_result =
                    overselection_selection(island, n_of_candidates_per_island);
                break;
            case selection_strategy::roundrobin_tournament:
                island_result =
                    roundrobin_selection(island, n_of_candidates_per_island);
                break;
            }
            result.insert(result.end(), island_result.begin(),
                          island_result.end());
        }
        return result;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::reproduction(population_type &parents) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> r(0.0, 1.0);
        population_type children;
        children.reserve(parents.size() / this->parents_per_children_);
        for (int j = 0; j < parents.size(); j += this->parents_per_children_) {
            double crossover_probability =
                (parents[j]->crossover_probability_ +
                 parents[j + 1]->crossover_probability_) /
                2;
            if (r(generator) < crossover_probability) {
                children.emplace_back(std::make_shared<individual>(
                    parents[j]->crossover(this->problem_, *parents[j + 1])));
            } else {
                children.emplace_back(
                    std::make_shared<individual>(*parents[j]));
                children.back()->mutation(this->problem_,
                                          children.back()->mutation_strength_);
            }
        }
        return children;
    }

    void evolutionary_algorithm::display_status(double lambda_value) {
        std::cout << "Generation #" << ++current_generation_;
        std::cout << " - Lambda value: " << lambda_value;
        std::cout << " - Best_fx: " << this->best_fx() << std::endl;
    }
    double evolutionary_algorithm::best_fx() const {
        if (!this->best_solutions_.empty()) {
            return this->best_solutions_[0]->fx;
        } else {
            return std::numeric_limits<double>::max();
        }
    }

    std::ostream &operator<<(std::ostream &os,
                             const evolutionary_algorithm &algorithm) {
        for (const auto &[point, value] : algorithm.pareto_front_) {
            os << point << " -> " << value << "\n";
        }
        return os;
    }
    void evolutionary_algorithm::plot() {
        auto now = std::chrono::system_clock::now();
        std::string str_now = date::format("%Y_%m_%d_%H_%M", now);
        std::string file_name;
        file_name.append("pareto_");
        file_name.append(std::to_string(max_generations_));
        file_name.append("_");
        file_name.append(str_now);
        file_name.append(".jpg");
        std::cout << file_name << std::endl;
        pareto::plot_front(pareto_front_);
        matplot::save(file_name);
    }
    void evolutionary_algorithm::initialize_population() {
        this->population_.resize(0);
        this->population_.reserve(this->population_size_);
        for (int i = this->population_.size(); i < this->population_size_;
             ++i) {
            this->population_.emplace_back(
                std::make_shared<individual>(this->problem_));
        }
    }
    void evolutionary_algorithm::algorithm(enum algorithm alg) {
        switch (alg) {
        case algorithm::DEFAULT: {
            // this->population_size_ = 300;
            this->population_size_ = 300;
            this->number_of_islands_ = 5;
            this->fitness_sharing_niche_size_ = 3.0;
            this->children_proportion_ = 7.0;
            this->crossover_probability_ = 0.9;
            this->mutation_strength_ = 1 / this->problem_.size();
            this->competition_between_parents_and_children_ = false;
            this->elitism_proportion_ = 0.01;
            this->reproduction_scaling_strategy_ = scaling_strategy::window;
            this->reproduction_selection_strategy_ =
                selection_strategy::tournament;
            this->survival_scaling_strategy_ = scaling_strategy::window;
            this->survival_selection_strategy_ = selection_strategy::truncate;
            break;
        }
        case algorithm::GA: {
            this->population_size_ = 100;
            this->number_of_islands_ = 1;
            this->fitness_sharing_niche_size_ = 0.0;
            this->children_proportion_ = 1.0;
            this->crossover_probability_ = 0.8;
            this->mutation_strength_ = 1 / this->problem_.size();
            this->competition_between_parents_and_children_ = false;
            this->elitism_proportion_ = 1.0;
            this->reproduction_scaling_strategy_ = scaling_strategy::window;
            this->reproduction_selection_strategy_ =
                selection_strategy::roulette;
            this->survival_scaling_strategy_ = scaling_strategy::window;
            this->survival_selection_strategy_ = selection_strategy::truncate;
            break;
        }
        case algorithm::EE: {
            this->population_size_ = 40;
            this->number_of_islands_ = 1;
            this->fitness_sharing_niche_size_ = 0.0;
            this->children_proportion_ = 7.0;
            this->crossover_probability_ = 0.1;
            this->mutation_strength_ = 1 / this->problem_.size();
            this->competition_between_parents_and_children_ = false;
            this->elitism_proportion_ = 1.0;
            this->reproduction_scaling_strategy_ = scaling_strategy::window;
            this->reproduction_selection_strategy_ =
                selection_strategy::uniform;
            this->survival_scaling_strategy_ = scaling_strategy::window;
            this->survival_selection_strategy_ = selection_strategy::truncate;
            break;
        }
        }
    }
    evolutionary_algorithm::individual_ptr
    evolutionary_algorithm::best_solution() {
        if (!this->best_solutions_.empty()) {
            return this->best_solutions_[0];
        } else {
            return nullptr;
        }
    }
    typename evolutionary_algorithm::population_type::iterator
    evolutionary_algorithm::best_solutions_begin() {
        return this->best_solutions_.begin();
    }
    typename evolutionary_algorithm::population_type::iterator
    evolutionary_algorithm::best_solutions_end() {
        return this->best_solutions_.end();
    }
    size_t evolutionary_algorithm::population_size() {
        return this->population_size_;
    }
    void evolutionary_algorithm::population_size(size_t value) {
        this->population_size_ = value;
    }
    size_t evolutionary_algorithm::number_of_islands() {
        return this->number_of_islands_;
    }
    void evolutionary_algorithm::number_of_islands(size_t value) {
        this->number_of_islands_ = value;
    }
    enum evolutionary_algorithm::island_structure
    evolutionary_algorithm::island_structure() {
        return this->island_structure_;
    }
    void evolutionary_algorithm::island_structure(
        enum evolutionary_algorithm::island_structure value) {
        this->island_structure_ = value;
    }
    enum evolutionary_algorithm::island_migration_policy
    evolutionary_algorithm::island_migration_policy() {
        return this->island_migration_policy_;
    }
    void evolutionary_algorithm::island_migration_policy(
        enum evolutionary_algorithm::island_migration_policy value) {
        this->island_migration_policy_ = value;
    }
    enum evolutionary_algorithm::island_replacement_policy
    evolutionary_algorithm::island_replacement_policy() {
        return this->island_replacement_policy_;
    }
    void evolutionary_algorithm::island_replacement_policy(
        enum evolutionary_algorithm::island_replacement_policy value) {
        this->island_replacement_policy_ = value;
    }
    size_t evolutionary_algorithm::migration_epoch() {
        return this->migration_epoch_;
    }
    void evolutionary_algorithm::migration_epoch(size_t value) {
        this->migration_epoch_ = value;
    }
    size_t evolutionary_algorithm::migration_size() {
        return this->migration_size_;
    }
    void evolutionary_algorithm::migration_size(size_t value) {
        this->migration_size_ = value;
    }
    double evolutionary_algorithm::fitness_sharing_niche_size() {
        return fitness_sharing_niche_size_;
    }
    void evolutionary_algorithm::fitness_sharing_niche_size(double value) {
        this->fitness_sharing_niche_size_ = value;
    }
    size_t evolutionary_algorithm::max_generations() {
        return this->max_generations_;
    }
    void evolutionary_algorithm::max_generations(size_t value) {
        this->max_generations_ = value;
    }
    double evolutionary_algorithm::children_proportion() {
        return this->children_proportion_;
    }
    void evolutionary_algorithm::children_proportion(double value) {
        this->children_proportion_ = value;
    }
    double evolutionary_algorithm::crossover_probability() {
        return this->crossover_probability_;
    }
    void evolutionary_algorithm::crossover_probability(double value) {
        this->crossover_probability_ = value;
    }
    double evolutionary_algorithm::mutation_strength() {
        return this->mutation_strength_;
    }
    void evolutionary_algorithm::mutation_strength(double value) {
        this->mutation_strength_ = value;
    }
    bool evolutionary_algorithm::competition_between_parents_and_children() {
        return this->competition_between_parents_and_children_;
    }
    void evolutionary_algorithm::competition_between_parents_and_children(
        bool value) {
        this->competition_between_parents_and_children_ = value;
    }
    unsigned int evolutionary_algorithm::tournament_size() {
        return this->tournament_size_;
    }
    void evolutionary_algorithm::tournament_size(unsigned int value) {
        this->tournament_size_ = value;
    }
    double evolutionary_algorithm::overselection_proportion() {
        return this->overselection_proportion_;
    }
    void evolutionary_algorithm::overselection_proportion(double value) {
        this->overselection_proportion_ = value;
    }
    size_t evolutionary_algorithm::roundrobin_tournament_size() {
        return roundrobin_tournament_size_;
    }
    void evolutionary_algorithm::roundrobin_tournament_size(size_t value) {
        this->roundrobin_tournament_size_ = value;
    }
    double evolutionary_algorithm::elitism_proportion() {
        return this->elitism_proportion_;
    }
    void evolutionary_algorithm::elitism_proportion(double value) {
        this->elitism_proportion_ = value;
    }
    enum evolutionary_algorithm::scaling_strategy
    evolutionary_algorithm::reproduction_scaling_strategy() {
        return this->reproduction_scaling_strategy_;
    }
    void evolutionary_algorithm::reproduction_scaling_strategy(
        enum evolutionary_algorithm::scaling_strategy value) {
        this->reproduction_scaling_strategy_ = value;
    }
    enum evolutionary_algorithm::selection_strategy
    evolutionary_algorithm::reproduction_selection_strategy() {
        return this->reproduction_selection_strategy_;
    }
    void evolutionary_algorithm::reproduction_selection_strategy(
        enum evolutionary_algorithm::selection_strategy value) {
        this->reproduction_selection_strategy_ = value;
    }
    enum evolutionary_algorithm::scaling_strategy
    evolutionary_algorithm::survival_scaling_strategy() {
        return this->survival_scaling_strategy_;
    }
    void evolutionary_algorithm::survival_scaling_strategy(
        enum evolutionary_algorithm::scaling_strategy value) {
        this->survival_scaling_strategy_ = value;
    }
    enum evolutionary_algorithm::selection_strategy
    evolutionary_algorithm::survival_selection_strategy() {
        return this->survival_selection_strategy_;
    }
    void evolutionary_algorithm::survival_selection_strategy(
        enum evolutionary_algorithm::selection_strategy value) {
        this->survival_selection_strategy_ = value;
    }
    double evolutionary_algorithm::sigma_bias() { return this->sigma_bias_; }
    void evolutionary_algorithm::sigma_bias(double value) {
        this->sigma_bias_ = value;
    }
    double evolutionary_algorithm::sigma_constant() {
        return this->sigma_constant_;
    }
    void evolutionary_algorithm::sigma_constant(double value) {
        this->sigma_constant_ = value;
    }
    double evolutionary_algorithm::linear_rank_selective_pressure() {
        return this->linear_rank_selective_pressure_;
    }
    void evolutionary_algorithm::linear_rank_selective_pressure(double value) {
        this->linear_rank_selective_pressure_ = value;
    }
    size_t evolutionary_algorithm::n_of_selection_candidates() {
        return this->population_size_ * this->parents_per_children_ *
               this->children_proportion_;
    }
    size_t evolutionary_algorithm::size_of_elite_set() {
        return size_t(
            ceil(this->elitism_proportion_ * this->population_.size()));
    }
    void evolutionary_algorithm::display_status() {
        std::cout << "Generation #" << ++current_generation_;
        if (this->current_generation_ > 1) {
            std::cout << " Best_fx: " << this->best_fx() << std::endl;
            double min_fx = std::numeric_limits<double>::max();
            double avg_fx = 0.0;
            double max_fx = -std::numeric_limits<double>::max();
            for (individual_ptr &ind : this->population_) {
                min_fx = std::min(min_fx, ind->fx);
                max_fx = std::max(max_fx, ind->fx);
                avg_fx += ind->fx;
            }
            avg_fx /= this->population_.size();
            std::cout << "| Min: " << min_fx << " | Avg: " << avg_fx
                      << " | Max: " << max_fx << " | "
                      << ((min_fx == max_fx) ? ("(Population Converged)")
                                             : (""))
                      << std::endl;
        } else {
            std::cout << std::endl;
        }
    }
    void evolutionary_algorithm::scaling(
        evolutionary_algorithm::population_type &population,
        evolutionary_algorithm::scaling_strategy s) {
        switch (s) {
        case scaling_strategy::window: {
            window_scaling(population);
            break;
        }
        case scaling_strategy::sigma: {
            sigma_scaling(population);
            break;
        }
        case scaling_strategy::linear_rank: {
            linear_rank_scaling(population);
            break;
        }
        case scaling_strategy::exponential_rank: {
            exponential_rank_scaling(population);
            break;
        }
        }
    }
    void evolutionary_algorithm::window_scaling(
        evolutionary_algorithm::population_type &population) {
        for (individual_ptr &ind : population) {
            ind->fitness = -ind->fx;
        }
        const auto iter_to_min_fitness = std::min_element(
            population.begin(), population.end(), this->not_comp_fitness_);
        double min_fitness = (*iter_to_min_fitness)->fitness;
        for (individual_ptr &ind : population) {
            ind->fitness -= min_fitness + 1;
        }
    }
    void evolutionary_algorithm::sigma_scaling(
        evolutionary_algorithm::population_type &population) {
        for (individual_ptr &ind : population) {
            ind->fitness = -ind->fx;
        }
        double avg_fitness = 0.0;
        for (individual_ptr &ind : population) {
            avg_fitness += ind->fitness;
        }
        double std_fitness = 0.0;
        for (individual_ptr &ind : population) {
            std_fitness += pow(ind->fitness - avg_fitness, 2.0);
        }
        std_fitness /= population.size() - 1;
        std_fitness = sqrt(std_fitness);
        for (individual_ptr &ind : population) {
            ind->fitness = std::max(
                this->sigma_bias_ + (ind->fitness - avg_fitness) /
                                        (this->sigma_constant_ * std_fitness),
                0.0);
        }
    }
    void evolutionary_algorithm::linear_rank_scaling(
        evolutionary_algorithm::population_type &population) {
        attribute_fitness_from_rank(population);
        const double bias =
            ((2 - linear_rank_selective_pressure_) / population.size());
        const double denominator = population.size() * (population.size() - 1);
        for (individual_ptr &ind : population) {
            ind->fitness = bias + (2 * ind->fitness *
                                   (linear_rank_selective_pressure_ - 1)) /
                                      denominator;
        }
    }
    void evolutionary_algorithm::exponential_rank_scaling(
        evolutionary_algorithm::population_type &population) {
        for (individual_ptr &ind : population) {
            ind->fitness = (1 - exp(-ind->fitness)) / population.size();
        }
    }
    void evolutionary_algorithm::fitness_sharing(
        evolutionary_algorithm::population_type &population) {
        for (int i = 0; i < population.size(); ++i) {
            double sum = 0.0;
            for (int j = 0; j < population.size(); ++j) {
                const double d =
                    population[i]->distance(this->problem_, *population[j],
                                            this->fitness_sharing_niche_size_);
                const double sh =
                    (d <= this->fitness_sharing_niche_size_)
                        ? (1 - d / this->fitness_sharing_niche_size_)
                        : 0.0;
                sum += sh;
            }
            population[i]->fitness /= sum + 1.0;
        }
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::uniform_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        population_type parents(n_of_candidates);
        std::uniform_int_distribution<size_t> pos_d(0, population.size() - 1);
        for (individual_ptr &parent : parents) {
            parent = population[pos_d(generator)];
        }
        return parents;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::truncate_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        population_type parents;
        parents.reserve(n_of_candidates);
        std::partial_sort(population.begin(),
                          population.begin() +
                              std::min(n_of_candidates, population.size()),
                          population.end(), this->comp_fitness_);
        std::copy(population.begin(),
                  population.begin() +
                      std::min(n_of_candidates, population.size()),
                  std::back_inserter(parents));
        int i = 0;
        while (parents.size() < n_of_candidates) {
            parents.push_back(parents[i % population.size()]);
            i++;
        }
        std::shuffle(parents.begin(), parents.end(), generator);
        return parents;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::tournament_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        population_type parents;
        std::uniform_int_distribution<size_t> pos_d(0, population.size() - 1);
        for (int i = 0; i < n_of_candidates; ++i) {
            size_t position = pos_d(generator);
            for (int j = 1; j < this->tournament_size_; ++j) {
                size_t position2 = pos_d(generator);
                if (population[position2]->fitness >
                    population[position]->fitness) {
                    position = position2;
                }
            }
            parents.push_back(population[position]);
        }
        return parents;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::roulette_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        population_type parents;
        parents.reserve(n_of_candidates);
        std::discrete_distribution<size_t> pos_d(
            population.size(), 0, population.size() - 1,
            [&population](size_t pos) { return population[pos]->fitness; });
        for (int i = 0; i < n_of_candidates; ++i) {
            parents.push_back(population[pos_d(generator)]);
        }
        return parents;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::sus_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        population_type parents;
        parents.reserve(n_of_candidates);
        double total_fit = 0.0;
        for (individual_ptr &ind : population) {
            total_fit += ind->fitness;
        }
        double gap = total_fit / n_of_candidates;
        std::uniform_real_distribution<double> dist_r(0.0, gap);
        double r = dist_r(generator);
        size_t current_ind = 0;
        double sum = population[current_ind]->fitness;
        for (int i = 0; i < n_of_candidates; ++i) {
            while (r > sum) {
                ++current_ind;
                sum += population[current_ind]->fitness;
            }
            parents.push_back(population[current_ind]);
            r += gap;
        }
        std::shuffle(parents.begin(), parents.end(), generator);
        return parents;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::overselection_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        population_type parents;
        parents.reserve(n_of_candidates);
        std::partial_sort(population.begin(),
                          population.begin() +
                              population.size() *
                                  this->overselection_proportion_,
                          population.end(), this->comp_fitness_);
        std::uniform_int_distribution<size_t> pos_best(
            0, population.size() * this->overselection_proportion_ - 1);
        for (int i = 0; i < n_of_candidates * 0.8; ++i) {
            parents.push_back(population[pos_best(generator)]);
        }
        std::uniform_int_distribution<size_t> pos_worst(
            population.size() * this->overselection_proportion_,
            population.size() - 1);
        for (int i = 0; i < n_of_candidates - (n_of_candidates * 0.8); ++i) {
            parents.push_back(population[pos_worst(generator)]);
        }
        return parents;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::roundrobin_selection(
        evolutionary_algorithm::population_type &population,
        size_t n_of_candidates) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        struct solution_with_score {
            individual_ptr s;
            size_t score = 0;
        };
        std::vector<solution_with_score> scores(population.size());
        for (int i = 0; i < population.size(); ++i) {
            scores[i].s = population[i];
        }
        std::uniform_int_distribution<size_t> pos_d(0, scores.size() - 1);
        for (int i = 0; i < population.size(); ++i) {
            solution_with_score &player1 = scores[i];
            for (int j = 1; j < this->roundrobin_tournament_size_; ++j) {
                solution_with_score &player2 = scores[pos_d(generator)];
                if (player1.s->fitness > player2.s->fitness) {
                    player1.score++;
                } else {
                    player2.score++;
                }
            }
        }
        std::sort(scores.begin(), scores.end(),
                  [](solution_with_score &a, solution_with_score &b) {
                      return a.score > b.score;
                  });
        population_type parents;
        parents.reserve(n_of_candidates);
        for (int i = 0; i < std::min(n_of_candidates, population.size()); ++i) {
            parents.push_back(scores[i].s);
        }
        int i = 0;
        while (parents.size() < n_of_candidates) {
            parents.push_back(parents[i % population.size()]);
            i++;
        }
        return parents;
    }
    evolutionary_algorithm::population_type evolutionary_algorithm::merge(
        evolutionary_algorithm::population_type &parents,
        evolutionary_algorithm::population_type &children) {
        population_type result = parents;
        result.insert(result.end(), children.begin(), children.end());
        return result;
    }
    evolutionary_algorithm::population_type
    evolutionary_algorithm::insert_elite_set(
        evolutionary_algorithm::population_type &individuals,
        evolutionary_algorithm::population_type &elite_source,
        size_t size_of_elite_set) {
        std::partial_sort(elite_source.begin(),
                          elite_source.begin() + size_of_elite_set,
                          elite_source.end(), this->comp_fitness_);
        individuals.insert(individuals.end(), elite_source.begin(),
                           elite_source.begin() + size_of_elite_set);
        return individuals;
    }
    void evolutionary_algorithm::attribute_fitness_from_rank(
        evolutionary_algorithm::population_type &population) {
        std::sort(population.begin(), population.end(), this->comp_);
        for (int i = 0; i < population.size(); ++i) {
            population[i]->fitness = population.size() - i;
        }
    }
    evolutionary_algorithm::population_type evolutionary_algorithm::get_island(
        evolutionary_algorithm::population_type &population, int idx) {
        population_type island;
        const size_t island_size = population.size() / this->number_of_islands_;
        island.insert(island.end(), population.begin() + idx * island_size,
                      population.begin() + (idx + 1) * island_size);
        return island;
    }
    void evolutionary_algorithm::try_to_update_best(
        evolutionary_algorithm::individual_ptr &candidate) {
        const bool solution_is_new =
            std::find_if(this->best_solutions_.begin(),
                         this->best_solutions_.end(),
                         [this, &candidate](individual_ptr &a) {
                             return (a->distance(this->problem_, *candidate,
                                                 0.0) == 0.0);
                         }) == this->best_solutions_.end();
        if (solution_is_new) {
            if (this->best_solutions_.size() <
                this->number_of_best_solutions_) {
                this->best_solutions_.push_back(candidate);
            } else {
                if (this->comp_(candidate, this->best_solutions_.back())) {
                    this->best_solutions_.pop_back();
                    this->best_solutions_.push_back(candidate);
                }
            }
            std::sort(this->best_solutions_.begin(),
                      this->best_solutions_.end(), this->comp_);
        }
    }
    void evolutionary_algorithm::migration_step() {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        if (this->current_generation_ % this->migration_epoch_ == 0) {
            for (int i = 0; i < this->number_of_islands_; ++i) {
                population_type island1 = get_island(this->population_, i);
                population_type island2;
                switch (this->island_structure_) {
                case island_structure::random: {
                    std::uniform_int_distribution<int> island_idx(
                        0, this->number_of_islands_ - 1);
                    island2 =
                        get_island(this->population_, island_idx(generator));
                    break;
                }
                case island_structure::ring: {
                    island2 = get_island(this->population_,
                                         (i + 1) % this->number_of_islands_);
                    break;
                }
                }

                population_type individuals_emigrating;
                switch (this->island_migration_policy_) {
                case island_migration_policy::random: {
                    std::uniform_int_distribution<int> island_idx(
                        0, island1.size() - 1);
                    for (int j = 0; j < this->migration_size_; ++j) {
                        individuals_emigrating.push_back(
                            island1[island_idx(generator)]);
                    }
                    break;
                }
                case island_migration_policy::best: {
                    std::partial_sort(island1.begin(),
                                      island1.begin() + this->migration_size_,
                                      island1.end(), this->comp_);
                    individuals_emigrating.insert(
                        individuals_emigrating.end(), island1.begin(),
                        island1.begin() + this->migration_size_);
                    break;
                }
                case island_migration_policy::fittest_half: {
                    std::partial_sort(island1.begin(),
                                      island1.begin() + island1.size() / 2,
                                      island1.end(), this->comp_);
                    std::uniform_int_distribution<int> island_idx(
                        0, island1.size() / 2);
                    for (int j = 0; j < this->migration_size_; ++j) {
                        individuals_emigrating.push_back(
                            island1[island_idx(generator)]);
                    }
                    break;
                }
                }

                population_type individuals_immigrating;
                switch (this->island_replacement_policy_) {
                case island_replacement_policy::worst_swap: {
                    std::partial_sort(island2.begin(),
                                      island2.begin() + this->migration_size_,
                                      island2.end(), this->not_comp_);
                    individuals_immigrating.insert(
                        individuals_immigrating.end(), island2.begin(),
                        island2.begin() + migration_size_);
                    for (int j = 0; j < this->migration_size_; ++j) {
                        std::swap(*individuals_immigrating[j],
                                  *individuals_emigrating[j]);
                    }
                    break;
                }
                case island_replacement_policy::worst_overwrite: {
                    std::partial_sort(
                        island2.begin(),
                        island2.begin() + this->migration_size_, island2.end(),
                        [this](individual_ptr &a, individual_ptr &b) {
                            return !this->comp_(a, b);
                        });
                    individuals_immigrating.insert(
                        individuals_immigrating.end(), island2.begin(),
                        island2.begin() + this->migration_size_);
                    for (int j = 0; j < this->migration_size_; ++j) {
                        *individuals_immigrating[j] =
                            *individuals_emigrating[j];
                    }
                    break;
                }
                case island_replacement_policy::random_swap: {
                    std::uniform_int_distribution<int> island_idx(
                        0, island2.size() - 1);
                    for (int j = 0; j < this->migration_size_; ++j) {
                        individuals_immigrating.push_back(
                            island2[island_idx(generator)]);
                    }
                    for (int j = 0; j < this->migration_size_; ++j) {
                        std::swap(*individuals_immigrating[j],
                                  *individuals_emigrating[j]);
                    }
                    break;
                }
                case island_replacement_policy::random_overwrite: {
                    std::uniform_int_distribution<int> island_idx(
                        0, island2.size() - 1);
                    for (int j = 0; j < this->migration_size_; ++j) {
                        individuals_immigrating.push_back(
                            island2[island_idx(generator)]);
                    }
                    for (int j = 0; j < this->migration_size_; ++j) {
                        *individuals_immigrating[j] =
                            *individuals_emigrating[j];
                    }
                    break;
                }
                }
            }
        }
    }
    double evolutionary_algorithm::lambda_value() const {
        return lambda_value_;
    }
    void evolutionary_algorithm::lambda_value(double value) {
        lambda_value_ = value;
    }
    pareto::front<double, 2, portfolio> &
    evolutionary_algorithm::pareto_front() {
        return this->pareto_front_;
    }

    void
    evolutionary_algorithm::individual::mutation(market_data &market,
                                                 double mutation_strength) {
        // mutate search parameters before they influence mutation
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        std::normal_distribution<double> n(0.0, 1);

        mutation_strength_ =
            mutation_strength_ *
            exp(second_order_mutation_strength_ * n(generator));
        reflect(mutation_strength_, 0.001, 0.999);

        crossover_probability_ =
            crossover_probability_ *
            exp(second_order_mutation_strength_ * n(generator));
        reflect(crossover_probability_, 0.001, 0.999);

        // call the underlying mutation function
        portfolio::mutation(market, mutation_strength_);
    }
    void evolutionary_algorithm::individual::reflect(double &v, double lb,
                                                     double ub) {
        // reflect is just a better version of truncating
        while (v < lb || v > ub) {
            if (v < lb) {
                v += 2 * (lb - v);
            }
            if (v > ub) {
                v -= 2 * (v - ub);
            }
        }
        return;
    }
    evolutionary_algorithm::individual
    evolutionary_algorithm::individual::crossover(market_data &p,
                                                  individual &rhs) {
        static std::default_random_engine generator =
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count());
        // crossover search parameters that might influence crossover
        std::uniform_real_distribution<double> u(0.0, 1.0);

        double alpha = u(generator);
        double child_mutation_strength = (alpha * this->mutation_strength_) +
                                         ((1 - alpha) * rhs.mutation_strength_);

        alpha = u(generator);
        double child_crossover_probability =
            (alpha * this->crossover_probability_) +
            ((1 - alpha) * rhs.crossover_probability_);

        // call the underlying crossover function
        individual child = this->portfolio::crossover(p, rhs);

        // copy the values to the new born child
        child.mutation_strength_ = child_mutation_strength;
        child.crossover_probability_ = child_crossover_probability;

        return child;
    }
} // namespace portfolio
