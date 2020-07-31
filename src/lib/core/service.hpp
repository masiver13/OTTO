#pragma once

#include <memory>

#include <nameof.hpp>

#include "lib/meta.hpp"
#include "lib/util/concepts.hpp"

#include "lib/logging.hpp"

namespace otto::lib::core {

  /// Base class of all services. Don't extend this directly, use `Service` instead
  ///
  /// Only ensures a virtual constructor, otherwise its just used for identifying types.
  struct IService {
    IService() = default;
    IService(const IService&) = delete;
    IService(IService&&) = delete;
    virtual ~IService() = default;
  };

  /// CRTP Base class for services. All services should extend this class.
  ///
  /// It is used to help detect the difference between the `AService` and `AServiceImpl` concepts.
  ///
  /// CRTP = Curiously Recurring Template Pattern. It's a C++ pattern for static
  /// polymorphism. All you need to know is, this is how you use this class:
  ///
  /// ```cpp
  /// struct SomeService : Service<SomeService> {
  ///   // ...
  /// };
  /// ```
  ///
  /// So, you extend it, templated on the derived class you are defining.
  template<typename Derived>
  struct Service : IService {
    using ServiceType = Derived;
  };

  template<typename S>
  concept AService = std::is_base_of_v<IService, S>&& std::is_same_v<typename S::ServiceType, S>;

  template<typename SI>
  concept AServiceImpl = !std::is_abstract_v<SI> && std::is_base_of_v<IService, SI>;

  template<typename SI, typename S>
  concept AServiceImplOf = AServiceImpl<SI>&& AService<S>&& std::is_same_v<typename SI::ServiceType, S>;

  namespace detail {
    template<AService S>
    S* active_service_ = nullptr;
  }

  template<AService S>
  void set_active_service(S& s)
  {
    OTTO_ASSERT(detail::active_service_<S> == nullptr, "Registering a service, when another is already registered");
    detail::active_service_<S> = &s;
  }

  template<AService S>
  void unset_active_service(S& s)
  {
    OTTO_ASSERT(detail::active_service_<S> == &s, "Unregistering a service different from the one registered");
    detail::active_service_<S> = nullptr;
  }

  template<AServiceImpl S>
  void set_active_service(S& s) requires(!AService<S>)
  {
    set_active_service(static_cast<typename S::ServiceType&>(s));
  }
  template<AServiceImpl S>
  void unset_active_service(S& s) requires(!AService<S>)
  {
    unset_active_service(static_cast<typename S::ServiceType&>(s));
  }

  /// DO NOT USE!
  ///
  /// Only available for testing and library code!
  ///
  /// Returns a pointer to the currently active service of type `S`
  template<AService S>
  S* unsafe_get_active_service()
  {
    return detail::active_service_<S>;
  }

  template<AService Service>
  struct ServiceHandle {
    using Constructor = std::function<std::unique_ptr<Service>()>;

    ServiceHandle(Constructor c) noexcept : constructor_(std::move(c)) {}

    ServiceHandle(ServiceHandle&&) = default;

    ~ServiceHandle() noexcept
    {
      if (started()) stop();
    }

    Service& service() const noexcept
    {
      return *service_;
    }

    Service* operator->() const noexcept
    {
      return service_.get();
    }

    ServiceHandle& start() & noexcept 
    {
      service_ = constructor_();
      set_active_service(*service_);
      return *this;
    }

    ServiceHandle&& start() && noexcept 
    {
      start();
      return std::move(*this);
    }

    void stop() noexcept
    {
      unset_active_service(*service_);
      service_ = nullptr;
    }

    bool started() const noexcept
    {
      return service_ != nullptr;
    }

  private:
    std::function<std::unique_ptr<Service>()> constructor_;
    std::unique_ptr<Service> service_;
  };

  template<AServiceImpl SI, typename... Args>
  requires(std::is_constructible_v<SI, Args...>)
    [[nodiscard("The returned handle manages the lifetime of the service")]] auto make_handle(Args&&... args)
  {
    return ServiceHandle<typename SI::ServiceType>([... as = FWD(args)] { return std::make_unique<SI>(FWD(as)...); });
  }


  template<AService... Services>
  struct ServiceAccessor {
    ServiceAccessor()
    {
      (check_service<Services>(), ...);
    }

  protected:
    template<util::one_of<Services...> S>
    S& service() const noexcept
    {
      OTTO_ASSERT(detail::active_service_<S> != nullptr, "Tried to access service '{}' with none registered",
                  NAMEOF_TYPE(S));
      return *detail::active_service_<S>;
    }

  private:
    template<util::one_of<Services...> S>
    void check_service()
    {
      if (detail::active_service_<S> == nullptr) {
        LOGF("ServiceAccessor constructed with no service {} available", NAMEOF_TYPE(S));
      }
    }
  };

  /// Require access to a service from a type that might be constructed before the service
  template<AService... Services>
  struct UnsafeServiceAccessor {
  protected:
    template<util::one_of<Services...> S>
    S* service_unsafe() const noexcept
    {
      return detail::active_service_<S>;
    }
  };
} // namespace otto::lib::core
