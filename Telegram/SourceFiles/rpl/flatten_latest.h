/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

#include <rpl/producer.h>

namespace rpl {
namespace details {

class flatten_latest_helper {
public:
	template <typename Value, typename Error>
	rpl::producer<Value, Error> operator()(
			rpl::producer<
				rpl::producer<Value, Error>,
				Error
			> &&initial) const {
		return [initial = std::move(initial)](
				const consumer<Value, Error> &consumer) mutable {
			auto state = std::make_shared<State>();
			return std::move(initial).start(
			[consumer, state](rpl::producer<Value, Error> &&inner) {
				state->finished = false;
				state->alive = std::move(inner).start(
				[consumer](Value &&value) {
					consumer.put_next(std::move(value));
				}, [consumer](Error &&error) {
					consumer.put_error(std::move(error));
				}, [consumer, state] {
					if (state->finished) {
						consumer.put_done();
					} else {
						state->finished = true;
					}
				});
			}, [consumer](Error &&error) {
				consumer.put_error(std::move(error));
			}, [consumer, state] {
				if (state->finished) {
					consumer.put_done();
				} else {
					state->finished = true;
				}
			});
		};
	}

private:
	struct State {
		lifetime alive;
		bool finished = false;
	};

};

} // namespace details

inline auto flatten_latest()
-> details::flatten_latest_helper {
	return details::flatten_latest_helper();
}

} // namespace rpl
