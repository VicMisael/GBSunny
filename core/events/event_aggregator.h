#pragma once
#include <functional>

template <typename... EventTypes>
class EventAggregator {

public:
	template<typename EventType>
	using Handler = std::function<void(const EventType&)>;

	template <typename EventType>

	void subscribe(Handler<EventType> handler) {

		auto& handlers_for_event = std::get<std::vector<Handler<EventType>>>(handlers);
		handlers_for_event.push_back(std::move(handler));

	}



	template <typename EventType>

	void send (const EventType& event) {
		auto& handlers_for_event = std::get<std::vector<Handler<EventType>>>(handlers);
		for (const auto& handler : handlers_for_event) {		
			handler(event);
		}
		
	}

private:
	std::tuple<std::vector<Handler<EventTypes>>...> handlers;

};

