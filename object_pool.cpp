#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <chrono>
#include <iomanip>

// A thread-safe, generic Object Pool that recycles objects automatically [12]
template <typename T>
class ObjectPool {
private:
    std::queue<std::unique_ptr<T>> m_pool;
    std::mutex m_mutex;
    std::size_t m_capacity;

public:
    explicit ObjectPool(std::size_t capacity) : m_capacity{capacity} {
        for (std::size_t i = 0; i < m_capacity; ++i) {
            m_pool.push(std::make_unique<T>());
        }
        std::cout << "⚙️  [ObjectPool] Pre-allocated pool with " << m_capacity << " objects.\n";
    }

    // Custom alias: returns a unique_ptr with a CUSTOM DELETER [12]
    // Instead of deleting the raw pointer, the custom deleter returns it to the pool [12].
    using PooledPtr = std::unique_ptr<T, std::function<void(T*)>>;

    PooledPtr acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_pool.empty()) {
            std::cout << "⚠️  [ObjectPool] Pool empty! Dynamically creating temporary fallback object.\n";
            // Return an object with a default deleter that deletes it when done
            return PooledPtr(new T(), [](T* ptr) { delete ptr; });
        }

        // Retrieve an object from the pool
        std::unique_ptr<T> obj = std::move(m_pool.front());
        m_pool.pop();

        // Release raw pointer from unique_ptr so we can custom-wrap it
        T* raw_ptr = obj.release();

        // Return the custom PooledPtr. When it goes out of scope, it recycles back to the pool! [12]
        return PooledPtr(raw_ptr, [this](T* ptr) {
            std::lock_guard<std::mutex> recycle_lock(this->m_mutex);
            this->m_pool.push(std::unique_ptr<T>(ptr));
            std::cout << "♻️  [ObjectPool] Object recycled back into the pool.\n";
        });
    }

    std::size_t available() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }
};

// Represents a high-frequency coordinate transformation packet in a robot kinematics stack
struct TransformPacket {
    double matrix[16]{1.0, 0.0, 0.0, 0.0, 
                      0.0, 1.0, 0.0, 0.0, 
                      0.0, 0.0, 1.0, 0.0, 
                      0.0, 0.0, 0.0, 1.0};
    int frame_id{0};

    TransformPacket() {
        std::cout << "🔨 [TransformPacket] Constructor executed (System Allocation).\n";
    }

    ~TransformPacket() {
        std::cout << "💀 [TransformPacket] Destructor executed (System Deallocation).\n";
    }
};

int main() {
    std::cout << "=== 🤖 High-Frequency Object Pool Simulation Started 🤖 ===\n\n";

    // Initialize an object pool of transform packets with a capacity of 2
    ObjectPool<TransformPacket> transformPool(2);

    std::cout << "\nAvailable pool objects: " << transformPool.available() << "\n";

    {
        std::cout << "\n--- 1. Acquiring Objects (Entering Local Scope) ---\n";
        // Acquire both pre-allocated elements
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
    // Out of scope! Custom deleters automatically run here [12]

    std::cout << "\nAvailable pool objects: " << transformPool.available() << "\n";

    std::cout << "\n--- 2. Re-acquiring Objects (Recycling Check) ---\n";
    {
        // These will be recycled instantly from the pool. No constructors are run! [12]
        auto packet3 = transformPool.acquire();
        std::cout << "Re-acquired recycled object. Default Frame ID: " << packet3->frame_id << "\n";
    }

    std::cout << "\n=== 🛑 Simulation Shutting Down 🛑 ===\n";
    return 0;
}
