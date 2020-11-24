#pragma once

#include "lib/util/concepts.hpp"
#include "lib/util/spin_lock.hpp"

#include "channel.hpp"
#include "executor.hpp"
#include "state.hpp"

namespace otto::itc {
  // Declarations
  template<AState State>
  struct Consumer<State> {
    Consumer(Channel<State>& ch, IExecutor& executor) : executor_(executor), channel_(&ch)
    {
      ch.internal_add_consumer(this);
    }

    virtual ~Consumer() noexcept
    {
      if (channel_) std::erase(channel_->consumers_, this);
    }

    /// The channel this consumer is registered on
    Channel<State>* channel() const noexcept
    {
      return channel_;
    }

    /// Access the newest state available.
    const State& state() const noexcept
    {
      return state_;
    }

    // FOR UNIFORMITY WITH Consumer<State...>

    /// Access the newest state available.
    template<std::same_as<State> S>
    const S& state() const noexcept
    {
      return state();
    }

  protected:
    /// Hook called immediately after the state is updated.
    ///
    /// The parameter is the same as `this->state()`
    ///
    /// Override in subclass if needed
    virtual void on_state_change(const State&) noexcept {}

  private:
    friend Channel<State>;

    /// Called from {@ref Channel::internal_commit}
    void internal_commit(const State& state)
    {
      {
        std::scoped_lock l(lock_);
        tmp_state_ = state;
      }
      executor_.execute([this] {
        // Apply changes
        {
          std::scoped_lock l(lock_);
          state_ = tmp_state_;
        }
        on_state_change(state_);
      });
    }

    util::spin_lock lock_;
    State state_;
    IExecutor& executor_;
    Channel<State>* channel_;
    State tmp_state_;
  };

  template<AState... States>
  struct Consumer : Consumer<States>... {
    template<AChannelFor<States...> Ch>
    Consumer(Ch& channel, IExecutor& ex) : Consumer<States>(channel, ex)...
    {}

    template<util::one_of<States...> S>
    const S& state() const noexcept
    {
      return Consumer<S>::state();
    }
  };

} // namespace otto::itc