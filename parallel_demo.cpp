#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <numeric>
#include <chrono>
#include <algorithm>
#include <random>
#include <iomanip>


std::mutex console_mutex;

struct SharedData {
    long long value = 0;
    std::mutex mtx;
};

void thread_add(SharedData& data, long long amount, int iterations, const std::string& name) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(data.mtx);
        data.value += amount;
    }
    std::lock_guard<std::mutex> log(console_mutex);
    std::cout << "  [" << name << "] завершён, добавил " << amount * iterations << "\n";
}

void easy_demo() {
    std::cout << "=== EASY: Два потока, синхронизация ===\n";

    SharedData data;
    int iterations = 100000;

    std::thread t1(thread_add, std::ref(data), 1, iterations, "Thread-1");
    std::thread t2(thread_add, std::ref(data), -1, iterations, "Thread-2");

    t1.join();
    t2.join();

    std::cout << "  Ожидаемый результат: 0\n";
    std::cout << "  Фактический результат: " << data.value << "\n\n";
}

template<typename Iterator, typename T>
struct AccumulateBlock {
    void operator()(Iterator first, Iterator last, T& result) {
        result = std::accumulate(first, last, T{});
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    const unsigned long length = std::distance(first, last);
    const unsigned long min_per_thread = 1000;

    if (!length) return init;

    const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware = std::thread::hardware_concurrency();
    const unsigned long num_threads = std::min(hardware ? hardware : 2, max_threads);
    const unsigned long block_size = length / num_threads;

    std::vector<T>           results(num_threads, T{});
    std::vector<std::thread> threads(num_threads - 1);

    Iterator block_start = first;

    for (unsigned long i = 0; i < num_threads - 1; ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(
            AccumulateBlock<Iterator, T>(),
            block_start, block_end,
            std::ref(results[i])
        );
        block_start = block_end;
    }

    AccumulateBlock<Iterator, T>()(block_start, last, results[num_threads - 1]);

    for (auto& t : threads) t.join();

    return std::accumulate(results.begin(), results.end(), init);
}

void middle_demo() {
    std::cout << "=== MIDDLE: Параллельное суммирование вектора ===\n";

    const size_t SIZE = 10'000'000;
    std::vector<long long> data(SIZE, 1); // сумма должна быть равна SIZE

    long long seq_result = std::accumulate(data.begin(), data.end(), 0LL);
    long long par_result = parallel_accumulate(data.begin(), data.end(), 0LL);

    std::cout << "  Размер вектора:           " << SIZE << "\n";
    std::cout << "  Последовательная сумма:   " << seq_result << "\n";
    std::cout << "  Параллельная сумма:       " << par_result << "\n";
    std::cout << "  Результаты совпадают:     " << (seq_result == par_result ? "ДА" : "НЕТ") << "\n\n";
}


// Последовательная сортировка
template<typename T>
void sequential_sort(std::vector<T>& data) {
    std::sort(data.begin(), data.end());
}

// Параллельная сортировка(разбиваем на блоки, сортируем, merge)
template<typename T>
void parallel_sort(std::vector<T>& data) {
    const size_t length = data.size();
    const size_t num_threads = std::thread::hardware_concurrency();
    const size_t block_size = length / num_threads;

    std::vector<std::thread> threads;

    for (size_t i = 0; i < num_threads; ++i) {
        size_t start = i * block_size;
        size_t end = (i == num_threads - 1) ? length : start + block_size;

        threads.emplace_back([&data, start, end]() {
            std::sort(data.begin() + start, data.begin() + end);
            });
    }

    for (auto& t : threads) t.join();

    for (size_t i = 1; i < num_threads; ++i) {
        size_t mid = std::min(i * block_size, length);
        size_t end = std::min((i + 1) * block_size, length);
        if (mid < length) {
            std::inplace_merge(data.begin(), data.begin() + mid, data.begin() + end);
        }
    }
    std::inplace_merge(data.begin(), data.begin() + block_size * (num_threads - 1), data.end());
}

template<typename Func>
long long measure_time(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void hard_demo() {
    std::cout << "=== HARD: Сортировка | Однопоточная vs Многопоточная ===\n";

    const size_t SIZE = 5'000'000;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1'000'000);

    std::vector<int> original(SIZE);
    std::generate(original.begin(), original.end(), [&]() { return dist(rng); });

    // --- Последовательная ---
    std::vector<int> seq_data = original;
    long long seq_time = measure_time([&]() {
        sequential_sort(seq_data);
        });

    // --- Параллельная ---
    std::vector<int> par_data = original;
    long long par_time = measure_time([&]() {
        parallel_sort(par_data);
        });

    bool correct = (seq_data == par_data);

    double speedup = seq_time > 0 ? static_cast<double>(seq_time) / par_time : 0;

    std::cout << "  Размер массива:            " << SIZE << " элементов\n";
    std::cout << "  Доступно потоков:          " << std::thread::hardware_concurrency() << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Время (однопоточная):      " << seq_time << " мс\n";
    std::cout << "  Время (многопоточная):     " << par_time << " мс\n";
    std::cout << "  Результаты совпадают:      " << (correct ? "ДА" : "НЕТ") << "\n\n";
}

int main() {
    easy_demo();
    middle_demo();
    hard_demo();
    return 0;
}
