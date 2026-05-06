#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <barrier>

using namespace std;

struct Node {
    Node* next;
    Node() : next(nullptr) {}
};

class ArmStack {
    Node* m_head{nullptr};
public:
    void push(Node* blk) {
        uint32_t res;
        do {
            asm volatile("ldxr %0, %1" : "=r"(blk->next) : "Q"(m_head) : "memory");
            asm volatile("stxr %w0, %2, %1" : "=&r"(res), "=Q"(m_head) : "r"(blk) : "memory");
        } while (res != 0);
    }

    Node* pop() {
        Node *blk, *next;
        uint32_t res;
        do {
            asm volatile("ldxr %0, %1" : "=r"(blk) : "Q"(m_head) : "memory");
            if (!blk) return nullptr; 
            
            next = blk->next;
            asm volatile("stxr %w0, %2, %1" : "=&r"(res), "=Q"(m_head) : "r"(next) : "memory");
        } while (res != 0); 
        
        return blk;
    }
};

void run_benchmark(int num_threads) {
    ArmStack stack;
    int ops = 500000; 

    // Rozgrzewka
    for(int i = 0; i < num_threads * 2; ++i) stack.push(new Node());

    vector<thread> threads;
    
    // Tworzymy barierę dla: N wątków roboczych + 1 (wątek główny)
    barrier sync_barrier(num_threads + 1);

    auto start_time = chrono::high_resolution_clock::now(); 

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            Node* local_node = new Node();
            
            sync_barrier.arrive_and_wait();

            for (int j = 0; j < ops; ++j) {
                stack.push(local_node);
                local_node = stack.pop();
            }
        });
    }

    sync_barrier.arrive_and_wait();
    start_time = chrono::high_resolution_clock::now();

    for (auto& t : threads) t.join();
    auto end_time = chrono::high_resolution_clock::now();

    chrono::duration<double> diff = end_time - start_time;
    double mops = (num_threads * ops * 2.0) / 1000000.0 / diff.count();
    
    cout << "Watki: " << num_threads << " | LL/SC STACK -> " << mops << " MOPS\n";
}

int main() {
    cout << "TEST WYDAJNOSCI LL/SC (ARM)\n";
    vector<int> t_counts = {1, 2, 4, 8};
    for(int t : t_counts) {
        run_benchmark(t);
    }
    return 0;
}