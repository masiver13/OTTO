#include "testing.t.hpp"

#include "lib/core/service.hpp"

using namespace otto;
using namespace otto::core;

TEST_CASE ("ServiceProvider / ServiceAccessor") {
  struct TestService : Service<TestService> {
    virtual int get_int() = 0;
  };
  static_assert(AService<TestService>);

  struct TSImpl : TestService {
    int get_int() override
    {
      // Chosen by fair dice roll
      // Guaranteed to be random
      return 4;
    }
  };
  static_assert(AServiceImpl<TSImpl>);
  static_assert(!AService<TSImpl>);

  SECTION ("set/unset active service manually") {
    TSImpl tsi;
    REQUIRE(core::unsafe_get_active_service<TestService>() == nullptr);
    set_active_service(tsi);
    // NOT A PUBLIC API!
    REQUIRE(core::unsafe_get_active_service<TestService>() == &tsi);
    unset_active_service(tsi);
    // NOT A PUBLIC API!
    REQUIRE(core::unsafe_get_active_service<TestService>() == nullptr);
  }

  SECTION ("Auto register/unregister service using ServiceHandle") {
    {
      auto prov = make_handle<TSImpl>().start();
      REQUIRE(core::unsafe_get_active_service<TestService>() == &prov.service());
    }
    REQUIRE(core::unsafe_get_active_service<TestService>() == nullptr);
  }

  struct TSAccessor : ServiceAccessor<TestService> {
    void test_4()
    {
      REQUIRE(service<TestService>().get_int() == 4);
    }
  };

  SECTION ("Access registered service through ServiceAccessor") {
    auto serv = make_handle<TSImpl>().start();
    TSAccessor a;
    a.test_4();
  }

  SECTION ("name") {
    TSImpl tsi;
    REQUIRE(tsi.name() == "TestService");
    REQUIRE(service_name<TestService>() == "TestService");
    REQUIRE(service_name<TSImpl>() == "TestService");
  }
}
