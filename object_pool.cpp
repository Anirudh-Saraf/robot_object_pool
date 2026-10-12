#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <functional>
#include <iomanip>
// A thread-safe, generic Object Pool that recycles objects automatically
template <typename T>
class ObjectPool {
public:
    // Define the custom deleter type using std::function
    using DeleterType = std::function<void(T*)>;
    // The clean smart pointer type using our custom deleter
    using PooledPtr = std::unique_ptr<T, DeleterType>;
private:
    // The pool now stores raw unique pointers with the custom deleter
    std::queue<std::unique_ptr<T, DeleterType>> m_pool;
    std::mutex m_mutex;
    std::size_t m_capacity;

public:
    explicit ObjectPool(std::size_t capacity) : m_capacity{capacity} {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (std::size_t i = 0; i < m_capacity; ++i) {
            // Seed the pool with standard heap-allocated objects that delete themselves if the pool is destroyed
            m_pool.push(std::unique_ptr<T, DeleterType>(new T(), [](T* ptr) { delete ptr; }));
        }
        std::cout << "⚙️  [ObjectPool] Pre-allocated pool with " << m_capacity << " objects.\n";
    }

    PooledPtr acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_pool.empty()) {
            std::cout << "⚠️  [ObjectPool] Pool empty! Dynamically creating temporary fallback object.\n";
            // Create a temporary object on the fly that deletes itself when finished
            return PooledPtr(new T(), [](T* ptr) { delete ptr; });
        }

        // Extract an object and transfer its ownership
        auto raw_obj = std::move(m_pool.front());
        m_pool.pop();

        // Release the raw pointer from the queue wrapper
        T* raw_ptr = raw_obj.release();

        // Wrap the raw pointer in a new PooledPtr with our custom recycling deleter
        return PooledPtr(raw_ptr, [this](T* ptr) {
            std::lock_guard<std::mutex> recycle_lock(this->m_mutex);
            // Push the recycled object back into the queue
            this->m_pool.push(std::unique_ptr<T, DeleterType>(ptr, [](T* p) { delete p; }));
            std::cout << "♻️  [ObjectPool] Object recycled back into the pool.\n";
        });
    }

    std::size_t available() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }
};

// Represents a coordinate transformation packet in a robot kinematics stack
struct TransformPacket {
    double matrix[16]{1.0, 0.0, 0.0, 0.0, 
                      0.0, 1.0, 0.0, 0.0, 
                      0.0, 0.0, 1.0, 0.0, 
                      0.0, 0.0, 0.0, 1.0};
    int frame_id{0};

    TransformPacket() {
        std::cout << "🔨 [TransformPacket] Constructor executed (Allocation).\n";
    }

    ~TransformPacket() {
        std::cout << "💀 [TransformPacket] Destructor executed (System Deallocation).\n";
    }
};

int main() {
    std::cout << "=== 🤖 High-Frequency Object Pool Simulation Started 🤖 ===\n\n";

    // Initialize our object pool with a capacity of 2
    ObjectPool<TransformPacket> transformPool(2);

    std::cout << "\nAvailable pool objects: " << transformPool.available() << "\n";

    {
        std::cout << "\n--- 1. Acquiring Objects (Entering Local Scope) ---\n";
        auto packet1 = transformPool.acquire();
        packet1->frame_id = 101;
        std::cout << "Acquired Packet 1. Frame ID set to: " << packet1->frame_id << "\n";

        auto packet2 = transformPool.acquire();
        packet2->frame_id = 102;
        std::cout << "Acquired Packet 2. Frame ID set to: " << packet2->frame_id << "\n";

        std::cout << "\nAvailable pool objects: " << transformPool.available() << "\n";

        std::cout << "\nAttempting to acquire from an empty pool:\n";
        auto fallback_packet = transformPool.acquire(); // Triggers custom default-deleter fallback
    } 
    // Out of scope here! Custom deleters automatically run

    std::cout << "\nAvailable pool objects: " << transformPool.available() << "\n";

    std::cout << "\n--- 2. Re-acquiring Objects (Recycling Check) ---\n";
    {
        // These will be recycled instantly from the pool. No constructors are run!
        auto packet3 = transformPool.acquire();
        std::cout << "Re-acquired recycled object. Frame ID left over from last use: " << packet3->frame_id << "\n";
    }

    std::cout << "\n=== 🛑 Simulation Shutting Down 🛑 ===\n";
    return 0;
}
