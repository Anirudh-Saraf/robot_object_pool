# Thread-Safe C++ Object Pool with Custom Smart Pointer Deleters

A high-speed, thread-safe generic Object Pool written in modern C++17. This framework utilizes custom smart pointer (`std::unique_ptr`) deleters to automatically recycle dynamic objects back into a pre-allocated queue, completely bypassing runtime heap construction and deallocation overhead.

---

##  How It Works

Repeatedly allocating and deallocating high-frequency state structures (such as kinematic coordinate transformations or network packets) leads to dynamic memory fragmentation and latency. 

This **Object Pool** eliminates this overhead through smart recycling:

```text
    [ Client Code (Local Scope) ] ── (Acquire Pointer) ──> [ Object Pool ]
                 │                                               ▲
                 │ (Pointer Out of Scope)                        │
                 └────── (Custom Lambda Deleter) ────────────────┘
                         Recycles raw pointer into queue
