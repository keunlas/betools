#include <cassert>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "betools/singleton.hpp"

// =============================================================================
// 测试辅助：Tag 类型（用于多实例测试）
// =============================================================================
struct TagA {};
struct TagB {};

// =============================================================================
// 测试辅助：带参 / 多参构造类
// =============================================================================
struct NamedObj {
  std::string name;
  explicit NamedObj(const std::string& n) : name(n) {}
};

struct MultiArg {
  int a;
  double b;
  std::string c;
  MultiArg(int x, double y, std::string z) : a(x), b(y), c(std::move(z)) {}
};

// =============================================================================
// 测试辅助：独立计数器类型（避免单例跨测试段残留）
// =============================================================================
struct Counter1 {
  inline static int count = 0;
  int id;
  Counter1() : id(++count) {}
};

struct Counter4 {
  inline static int count = 0;
  int id;
  Counter4() : id(++count) {}
};

struct Counter5 {
  inline static int count = 0;
  int id;
  Counter5() : id(++count) {}
};

struct Counter7 {
  inline static int count = 0;
  int id;
  Counter7() : id(++count) {}
};

struct LazyFlag {
  inline static bool constructed = false;
  LazyFlag() { constructed = true; }
};

struct AddrCheck {
  AddrCheck() = default;
};

/// 独立于 NamedObj，避免测试 2 的单例残留
struct NamedObj10 {
  std::string name;
  explicit NamedObj10(const std::string& n) : name(n) {}
};

int main() {
  // =========================================================================
  // 1. 默认构造函数 — 多次调用返回同一实例
  // =========================================================================
  {
    Counter1::count = 0;

    auto& c1 = betools::Singleton<Counter1>::Instance();
    auto& c2 = betools::Singleton<Counter1>::Instance();

    assert(&c1 == &c2);            // 地址相同
    assert(c1.id == c2.id);        // id 相同
    assert(Counter1::count == 1);  // 仅构造一次
  }

  // =========================================================================
  // 2. 带参构造 — 首次传参构造，后续参数被忽略
  // =========================================================================
  {
    auto& n1 = betools::Singleton<NamedObj>::Instance("hello");
    assert(n1.name == "hello");

    auto& n2 = betools::Singleton<NamedObj>::Instance("world");
    assert(&n1 == &n2);          // 同一实例
    assert(n2.name == "hello");  // 值未变（第二次的参数被忽略）
  }

  // =========================================================================
  // 3. 多参数构造函数
  // =========================================================================
  {
    auto& m1 =
        betools::Singleton<MultiArg>::Instance(42, 3.14, std::string("foo"));
    assert(m1.a == 42);
    assert(m1.b == 3.14);
    assert(m1.c == "foo");

    auto& m2 =
        betools::Singleton<MultiArg>::Instance(0, 0.0, std::string("bar"));
    assert(&m1 == &m2);
    assert(m1.a == 42);  // 首次构造的值不变
  }

  // =========================================================================
  // 4. 不同 Tag 产生不同实例
  // =========================================================================
  {
    Counter4::count = 0;

    auto& cA = betools::Singleton<Counter4, TagA>::Instance();
    auto& cB = betools::Singleton<Counter4, TagB>::Instance();

    assert(&cA != &cB);            // 不同地址
    assert(cA.id != cB.id);        // 不同 id
    assert(Counter4::count == 2);  // 各构造一次

    // 各自重复调用返回自身
    auto& cA2 = betools::Singleton<Counter4, TagA>::Instance();
    auto& cB2 = betools::Singleton<Counter4, TagB>::Instance();
    assert(&cA == &cA2);
    assert(&cB == &cB2);
  }

  // =========================================================================
  // 5. 默认 Tag (void) 与自定义 Tag 共存
  // =========================================================================
  {
    Counter5::count = 0;

    auto& cDefault = betools::Singleton<Counter5>::Instance();
    auto& cA = betools::Singleton<Counter5, TagA>::Instance();

    assert(&cDefault != &cA);
    assert(cDefault.id != cA.id);
    assert(Counter5::count == 2);

    // 重复调用一致性
    assert((&cDefault == &betools::Singleton<Counter5>::Instance()));
    assert((&cA == &betools::Singleton<Counter5, TagA>::Instance()));
  }

  // =========================================================================
  // 6. 不同 Tag 可传不同构造参数
  // =========================================================================
  {
    auto& nA = betools::Singleton<NamedObj, TagA>::Instance("Alpha");
    auto& nB = betools::Singleton<NamedObj, TagB>::Instance("Beta");

    assert(nA.name == "Alpha");
    assert(nB.name == "Beta");
    assert(&nA != &nB);  // 不同实例
  }

  // =========================================================================
  // 7. 三个不同 Tag 各自独立（含默认 void）
  // =========================================================================
  {
    Counter7::count = 0;

    auto& cDefault = betools::Singleton<Counter7>::Instance();
    auto& cA = betools::Singleton<Counter7, TagA>::Instance();
    auto& cB = betools::Singleton<Counter7, TagB>::Instance();

    assert(&cDefault != &cA);
    assert(&cDefault != &cB);
    assert(&cA != &cB);
    assert(Counter7::count == 3);
  }

  // =========================================================================
  // 8. 懒加载 — 未调用 Instance() 前不构造
  // =========================================================================
  {
    LazyFlag::constructed = false;
    assert(LazyFlag::constructed == false);

    auto& c = betools::Singleton<LazyFlag>::Instance();
    assert(LazyFlag::constructed == true);  // 调用后才构造
    (void)c;
  }

  // =========================================================================
  // 9. 返回引用类型正确、可寻址
  // =========================================================================
  {
    auto& ref = betools::Singleton<AddrCheck>::Instance();
    static_assert(std::is_same_v<decltype(ref), AddrCheck&>);

    AddrCheck* ptr = &ref;
    assert(ptr != nullptr);
  }

  // =========================================================================
  // =========================================================================
  // 10. std::string 与 const char* 统一传参（显式指定类型避免歧义）
  // =========================================================================
  {
    // 统一使用 std::string 类型调用，确保始终命中同一个 Instance 实例化
    auto& n1 = betools::Singleton<NamedObj10>::Instance(std::string("first"));
    assert(n1.name == "first");

    auto& n2 = betools::Singleton<NamedObj10>::Instance(std::string("second"));
    assert(&n1 == &n2);          // 同一实例
    assert(n1.name == "first");  // 值不变

    // 错误示范：不同参数类型产生不同 static 实例
    auto& n3 = betools::Singleton<NamedObj10>::Instance("second");
    assert(&n3 != &n2);  // ← 不是同一个单例！
  }

  return 0;
}
