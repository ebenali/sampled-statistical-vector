#pragma once

template<typename T>
requires std::is_floating_point_v<T> || std::is_integral_v<T>
struct SampledSeries {
    SampledSeries() = delete;
    SampledSeries(SampledSeries const &) = default;
    SampledSeries(SampledSeries &&)  noexcept = default;
    SampledSeries &operator=(SampledSeries const &) = default;

    explicit SampledSeries(size_t max_values): max_values_{max_values ? max_values : 1} {
        samples_.reserve(max_values ? max_values : 1);
    }

    explicit operator bool() const { return !samples_.empty(); }

    [[nodiscard]] T last() const {
        try {
            sanity_check();
            return *(samples_.end() - 1);
        } catch (...) { return SNAN; }
    }
    [[nodiscard]] T first() const { try { sanity_check(); return samples_[0]; } catch(...) { return SNAN; } }
    [[nodiscard]] T mean() const { try { sanity_check(); return cma_; } catch(...) { return SNAN; } }
    [[nodiscard]] T median() const { try { sanity_check(); return *(samples_.begin() + std::distance(samples_.begin(), samples_.end()) / 2); } catch(...) { return SNAN; } }
    [[nodiscard]] size_t size() const { return samples_.size(); }
    [[nodiscard]] size_t total_processed() const { return n_processed_; }

    SampledSeries &operator<<(T value) {
        ++n_processed_;
        if (value < min_)
            min_ = value;
        if (value > max_)
            max_ = value;

        if (cma_n_ == 0)
            cma_ = value;
        else
            cma_ = cma_ + (value - cma_) /  (cma_n_ + 1);
        ++cma_n_;


        if (samples_.size() < max_values_) {
            if (samples_.empty())
                samples_.push_back(value);
            else if (value == max_) {
                samples_.insert(samples_.end()-1, value);
            } else if (value == min_) {
                samples_.insert(samples_.begin(), value);
            } else {
                const auto insert_position = std::equal_range(samples_.begin(), samples_.end(), value);
                samples_.insert(insert_position.first - 1, value);
            }
        } else {
            if (max_values_ == 1) {
                samples_[0] = value;
                return *this;
            }
            if (value == max_) {
                *(samples_.end()-1)= value;
            } else if (value == min_) {
                samples_[0] = value;
            } else {
                const auto replace_position = std::equal_range(samples_.begin(), samples_.end(), value);
                *replace_position.first = value;
            }
        }

        return *this;
    }

private:
    void sanity_check() const {
        if (!*this)
            throw std::logic_error{"SampledSeries: attempting to query statistics of invalid series"};
    }
    size_t max_values_;
    T min_ = std::numeric_limits<std::decay_t<T>>::max();
    T max_ = std::numeric_limits<std::decay_t<T>>::min();
    T cma_ = std::numeric_limits<std::decay_t<T>>::quiet_NaN();
    size_t cma_n_ = 0;

    size_t n_processed_ = 0;

    std::vector<T> samples_;
};
