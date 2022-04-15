#include <atomic>
#include <barrier>
#include <iostream>
#include <thread>

void maybe_fence() {
#ifdef FENCE
    std::atomic_thread_fence(std::memory_order::seq_cst);
#else
    // Prevent compiler reordering
    std::atomic_signal_fence(std::memory_order::seq_cst);
#endif
}

int main() {
    std::barrier barrier(2, []() noexcept {});

    std::atomic<int> x;
    std::atomic<int> y;

    int a;
    int b;

    auto prepare = [&] {
        x.store(0, std::memory_order::relaxed);
        y.store(0, std::memory_order::relaxed);
    };

    auto both_zero = [&] { return a == 0 && b == 0; };

    std::thread t1([&] {
        while (true) {
            prepare();
            barrier.arrive_and_wait();

            x.store(1, std::memory_order::relaxed);
            maybe_fence();
            a = y.load(std::memory_order::relaxed);

            barrier.arrive_and_wait();

            if (both_zero()) {
                break;
            }

            barrier.arrive_and_wait();
        }
    });

    std::thread t2([&] {
        while (true) {
            barrier.arrive_and_wait();

            y.store(1, std::memory_order::relaxed);
            maybe_fence();
            b = x.load(std::memory_order::relaxed);

            barrier.arrive_and_wait();

            if (both_zero()) {
                break;
            }

            barrier.arrive_and_wait();
        }
    });

    t1.join();
    t2.join();

    std::cout << "Wat\n";

    return 0;
}
