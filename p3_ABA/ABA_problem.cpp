//compilation: g++ -std=c++20 -O0 -mcx16 ./ABA_problem.cpp -o ./ABA_problem -lpthread -latomic
//usage: ./ABA_problem

#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <string>
#include <barrier>
using namespace std;

struct Node {
    string id;
    Node* next;
    Node(string identifier) : id(identifier), next(nullptr) {}
};

void dummy_work(int cycles) {
    volatile int sum = 0;
    for (int i = 0; i < cycles; ++i) {
        sum += i;
    }
}

bool ENABLE_LOGS = true;

void log_state(const string& thread_name, const string& action, Node* top) {
    if (!ENABLE_LOGS) return;
    
    string msg = "[" + thread_name + "] " + action + " | Stos: ";
    Node* curr = top;
    int count = 0;
    while (curr != nullptr && count < 5) {
        msg += "[" + curr->id + "]->";
        curr = curr->next;
        count++;
    }
    msg += "null\n";
    
    cout << msg;
}

// 1. MUTEX
class MutexStack {
    Node* top = nullptr;
    mutex mtx;
public:
    void push(Node* node) {
        lock_guard<mutex> lock(mtx);
        node->next = top;
        top = node;
    }

    Node* pop(bool simulate_preemption = false) {
        lock_guard<mutex> lock(mtx); 
        if (top == nullptr) return nullptr;
        
        Node* old_top = top;
        if (simulate_preemption) {
            log_state("T1", "Blokada uzyskana. Wywlaszczenie OS.", top);
            this_thread::sleep_for(chrono::milliseconds(100));
            log_state("T1", "Wznowienie. POP zakonczony.", top);
        }

        top = top->next;
        return old_top;
    }
    Node* get_top() { return top; }
};

// 2. NAIWNY CAS
class NaiveStack {
public:
    atomic<Node*> top{nullptr};
    atomic<bool> thread1_paused{false};
    atomic<bool> thread2_finished{false};

    void push(Node* node, const string& tid = "SYS") {
        Node* current_top = top.load();
        do {
            node->next = current_top;
            if (top.compare_exchange_weak(current_top, node)) {
                log_state(tid, "PUSH [" + node->id + "] CAS OK.", top.load());
                break;
            }
        } while (true);
    }

    Node* pop(bool simulate_preemption = false, const string& tid = "SYS") {
        Node* old_top = top.load();
        while (old_top != nullptr) {
            Node* new_top = old_top->next;

            if (simulate_preemption) {
                log_state(tid, "Odczyt Top=[" + old_top->id + "]. Wywlaszczenie.", top.load());
                thread1_paused = true;
                while (!thread2_finished) { this_thread::yield(); }
                simulate_preemption = false;
                log_state(tid, "Wznowienie. Wykonuje stary CAS.", top.load());
            }

            if (top.compare_exchange_weak(old_top, new_top)) {
                log_state(tid, "POP [" + old_top->id + "] CAS OK.", top.load());
                return old_top;
            } else {
                log_state(tid, "POP CAS BLAD. Retry.", top.load());
            }
        }
        return nullptr;
    }
    Node* get_top() { return top.load(); }
};

// 3. TAGGED CAS (128-bit)
struct alignas(16) TaggedPointer {
    Node* ptr = nullptr;
    uint64_t tag = 0;
};

class TaggedStack {
public:
    atomic<TaggedPointer> top{};
    atomic<bool> thread1_paused{false};
    atomic<bool> thread2_finished{false};

    void push(Node* node) {
        TaggedPointer old_top = top.load();
        TaggedPointer new_top;
        do {
            node->next = old_top.ptr;
            new_top.ptr = node;
            new_top.tag = old_top.tag + 1; 
        } while (!top.compare_exchange_weak(old_top, new_top));
    }

    Node* pop(bool simulate_preemption = false) {
        TaggedPointer old_top = top.load();
        TaggedPointer new_top;
        while (old_top.ptr != nullptr) {
            new_top.ptr = old_top.ptr->next;
            new_top.tag = old_top.tag + 1; 

            if (simulate_preemption) {
                log_state("T1", "Odczyt Tag=" + to_string(old_top.tag) + ". Wywlaszczenie.", top.load().ptr);
                thread1_paused = true;
                while (!thread2_finished) { this_thread::yield(); }
                simulate_preemption = false; 
            }

            if (top.compare_exchange_weak(old_top, new_top)) {
                return old_top.ptr;
            } else if (!simulate_preemption && thread1_paused) {
                 log_state("T1", "CAS BLAD (Zly Tag). ABA zablokowane. Retry.", top.load().ptr);
                 thread1_paused = false;
            }
        }
        return nullptr;
    }
    Node* get_top() { return top.load().ptr; }
};

// TESTY LOGICZNE
void test_mutex() {
    cout << "\nTEST 1: MUTEX\n";
    MutexStack stack;
    stack.push(new Node("C")); 
    stack.push(new Node("A"));
    Node* node_B = new Node("B");

    log_state("SYS", "Stan startowy", stack.get_top());

    thread t1([&]() { 
        Node* popped = stack.pop(true); 
        if (popped) {
            log_state("T1", "Zakończono POP. Usunięto węzeł: [" + popped->id + "]", stack.get_top());
        }
    });
    
    this_thread::sleep_for(chrono::milliseconds(20)); 
    
    thread t2([&]() {
        log_state("T2", "Oczekuje na Mutex...", stack.get_top());
        
        Node* popped = stack.pop(); 
        
        if (popped) {
            log_state("T2", "Mutex zwolniony. POP pobrał: [" + popped->id + "]", stack.get_top());
            
            stack.push(node_B);
            stack.push(popped);
            log_state("T2", "Zakończono odkładanie.", stack.get_top());
        } else {
            log_state("T2", "Stos był pusty!", stack.get_top());
        }
    });

    t1.join(); 
    t2.join();
    log_state("SYS", "Stan końcowy (Sekwencyjna spójność)", stack.get_top());
}

void test_naive_cas() {
    cout << "\nTEST 2: NAIWNY CAS\n";
    NaiveStack stack;
    stack.top.store(new Node("C"));
    stack.push(new Node("A")); 
    Node* node_B = new Node("B");

    log_state("SYS", "Stan startowy", stack.get_top());

    thread t1([&]() { stack.pop(true, "T1"); });
    
    thread t2([&]() {
        while (!stack.thread1_paused) { this_thread::yield(); }
        log_state("T2", "Start sekwencji ABA.", stack.get_top());
        Node* a = stack.pop(false, "T2");
        stack.push(node_B, "T2");
        stack.push(a, "T2");
        log_state("T2", "Koniec sekwencji ABA.", stack.get_top());
        stack.thread2_finished = true;
    });

    t1.join(); t2.join();
    log_state("SYS", "Utrata wezla B", stack.get_top());
}

void test_tagged_cas() {
    cout << "\nTEST 3: TAGGED CAS\n";
    TaggedStack stack;
    stack.push(new Node("C")); stack.push(new Node("A"));
    Node* node_B = new Node("B");

    log_state("SYS", "Stan startowy", stack.get_top());

    thread t1([&]() { stack.pop(true); });
    thread t2([&]() {
        while (!stack.thread1_paused) { this_thread::yield(); }
        log_state("T2", "Start sekwencji ABA.", stack.get_top());
        Node* a = stack.pop();
        stack.push(node_B);
        stack.push(a);
        stack.thread2_finished = true;
    });

    t1.join(); t2.join();
    log_state("SYS", "Wynik: B ocalone", stack.get_top());
}

// BENCHMARK
template<typename StackType>
void run_realistic_benchmark(string name, int num_threads) {
    StackType stack;
    vector<thread> threads;
    
    int ops = 20000; 
    int work_cycles = 500;

    for(int i = 0; i < num_threads * ops; ++i) {
        stack.push(new Node("INIT"));
    }

    std::barrier sync_barrier(num_threads + 1);

    vector<vector<Node*>> all_pop_pools(num_threads, vector<Node*>(ops, nullptr));

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            vector<Node*> local_push_pool(ops);
            for(int j = 0; j < ops; ++j) {
                local_push_pool[j] = new Node("W"); 
            }
            
            sync_barrier.arrive_and_wait();

            for (int j = 0; j < ops; ++j) {
                stack.push(local_push_pool[j]);
                dummy_work(work_cycles); 
                
                all_pop_pools[i][j] = stack.pop();
                dummy_work(work_cycles); 
            }
        });
    }

    sync_barrier.arrive_and_wait();

    auto start_time = chrono::high_resolution_clock::now(); 

    for (auto& t : threads) t.join();
    
    auto end_time = chrono::high_resolution_clock::now();

    // Faza zwalniania pamięci 
    for (int i = 0; i < num_threads; ++i) {
        for(int j = 0; j < ops; ++j) {
            if (all_pop_pools[i][j] != nullptr) {
                delete all_pop_pools[i][j];
            }
        }
    }

    Node* remaining = stack.pop();
    while(remaining != nullptr) {
        delete remaining;
        remaining = stack.pop();
    }

    chrono::duration<double> diff = end_time - start_time;
    double mops = (num_threads * ops * 2.0) / 1000000.0 / diff.count();
    
    cout << "Watki: " << num_threads << " | " << name << " -> " << mops << " MOPS\n";
}

int main() {    

    run_realistic_benchmark<MutexStack>("MutexStack", 10);

    test_mutex();
    test_naive_cas();
    test_tagged_cas();
    
    ENABLE_LOGS = false;

    cout << "\nPRZEPUSTOWOSC (MOPS)\n";
    vector<int> real_t_counts = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20}; 
    //vector<int> real_t_counts = {1, 2, 4, 8, 16, 24, 32, 48, 64, 80, 96}; 
    
    cout << "Mutex:\n";
    for(int t : real_t_counts) run_realistic_benchmark<MutexStack>("MutexStack", t);
    
    cout << "\nCAS:\n";
    for(int t : real_t_counts) run_realistic_benchmark<NaiveStack>("NaiveStack", t);
    
    cout << "\nTagged CAS (Bezpieczny Lock-Free):\n";
    for(int t : real_t_counts) run_realistic_benchmark<TaggedStack>("TaggedStack", t);

    return 0;
}