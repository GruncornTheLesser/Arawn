#pragma once
#include <cstdint>
#include <functional>
#include <algorithm>

template<typename ... event_Ts>
class Dispatcher;

template<typename event_T>
class Dispatcher<event_T> {
public:
    using Callback = std::function<void(const event_T&)>;
    using Filter = std::function<bool(const event_T&)>;
private:
    struct Listener { 
        uint32_t handle;
        Filter filter;
        Callback callback;
        bool once;
    };
public:
    struct FilterView { // filter view to make syntax sugar
    public:
        FilterView(Dispatcher<event_T>& dispatcher, Filter filter) : dispatcher(dispatcher), filter(filter) { }

        uint32_t operator+=(Callback callback) { return dispatcher.attach(callback, filter, false); }
        uint32_t operator^=(Callback callback) { return dispatcher.attach(callback, filter, true); }
        void operator -=(uint32_t handle) { dispatcher.detach(handle); }

        uint32_t attach(Callback callback, bool once=false) { return dispatcher.attach(callback, filter, once); }
        void detach(uint32_t handle) { dispatcher.detach(handle); }
    private:
        Dispatcher<event_T>& dispatcher;
        Filter filter;
    };

    uint32_t operator+=(Callback callback) { return attach(callback, nullptr, false); }
    uint32_t operator^=(Callback callback) { return attach(callback, nullptr, true); }
    void operator-=(uint32_t handle) { detach(handle); }

    uint32_t attach(Callback callback, Filter filter=nullptr, bool once=false) {
        Listener* listener;
        if (active == data.size()) {
            listener = &data.emplace_back(active++);
        } else {
            listener = &data[active++];
        }
        listener->filter = filter;
        listener->callback = callback;
        listener->once = once;
        return listener->handle;
    }
    void detach(uint32_t handle) {
        auto end = data.begin() + active;
        auto it = std::stable_partition(data.begin(), end, [&](const auto& listener) {
            return listener.handle != handle;
        });
        
        if (it == end) return;

        std::for_each(it, end, [](auto& listener) {
            listener.filter = nullptr;
            listener.callback = nullptr;
        });
        
        active = it - data.begin();
    }

    void clear() { 
        active = 0;
        data.clear();
    }

    void invoke(event_T event) {
        auto end = data.begin() + active;
        auto it = std::stable_partition(data.begin(), end, [&](const auto& listener) {
            if (listener.filter == nullptr || listener.filter(event)) {
                listener.callback(event);
                return !listener.once;
            }
            return true;
        });
        std::for_each(it, data.begin() + active, [&](auto& listener) { 
            // deallocate lambdas/functors
            listener.callback = nullptr;
            listener.filter = nullptr;
        });
        active = it - data.begin();
    }

    size_t size() const {
        return active;
    }
private:
    uint32_t active = 0;
    std::vector<Listener> data;
};

template<typename ... event_Ts>
class Dispatcher : public Dispatcher<event_Ts>... {
public:
    template<typename event_T> 
    Dispatcher<event_T>& on() { 
        return *this;
    }
    template<typename event_T>
    Dispatcher<typename event_T::event_type>::FilterView on(event_T event) { 
        return { *this, event };
    }
};