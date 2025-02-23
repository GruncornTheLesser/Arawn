#pragma once
#include <cstdint>
#include <functional>

template<typename event_T>
struct Listener { 
    uint32_t handle;
    std::function<void(const event_T&)> callback;
    bool once;
};

template<typename ... event_Ts>
class Dispatcher : public Dispatcher<event_Ts>... {
public:
    template<typename event_T> Dispatcher<event_T>& on() { return *this; }
};

template<typename event_T>
class Dispatcher<event_T> {
public:
    uint32_t operator+=(const std::function<void(const event_T&)>& handler) {
        return attach(handler, false);
    }
    uint32_t operator^=(const std::function<void(const event_T&)>& handler) {
        return attach(handler, true);
    }

    uint32_t attach(const std::function<void(const event_T&)>& handler, bool once=false) {
        Listener<event_T>* listener;
        if (active == data.size()) {
            listener = &data.emplace_back(active++);
        } else {
            listener = &data[active++];
        }

        listener->callback = handler;
        listener->once = once;
        return listener->handle;
    }

    void detach(uint32_t handle) {
        auto end = std::remove_if(data.begin(), data.begin() + active, [&](const auto& listener) { 
            return listener.handle == handle; 
        });
        active = end - data.begin(); // should only have to remove 1 but this feels safer
    }
    void clear() { 
        active = 0;
        data.clear();
    }

    void invoke(event_T event) { 
        auto end = std::remove_if(data.begin(), data.begin() + active, [&](const auto& listener) {
            listener.callback(event);
            return listener.once;
        });
        std::for_each(end, data.begin() + active, [&](auto& listener) { 
            listener.callback = nullptr;
        }); // note: must set callback to null to deallocate lambdas
        active = end - data.begin();
    }
private:
    uint32_t active = 0;
    std::vector<Listener<event_T>> data;
};