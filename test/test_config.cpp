#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "betools/config.hpp"

int main() {
  // =========================================================================
  // 1. 基本 key=value 解析
  // =========================================================================
  {
    betools::Config cfg("key1=val1\nkey2=val2", false);
    assert(cfg.GetValue("key1") == "val1");
    assert(cfg.GetValue("key2") == "val2");
  }

  // =========================================================================
  // 2. 空配置字符串
  // =========================================================================
  {
    betools::Config cfg("", false);
    assert(cfg.GetValue("any") == "");
  }

  // =========================================================================
  // 3. 只有空白字符的行被跳过
  // =========================================================================
  {
    betools::Config cfg("   \t  \nkey=val\n  \t ", false);
    assert(cfg.GetValue("key") == "val");
  }

  // =========================================================================
  // 4. 空行被跳过
  // =========================================================================
  {
    betools::Config cfg("\n\nkey=val\n\n", false);
    assert(cfg.GetValue("key") == "val");
  }

  // =========================================================================
  // 5. 单行注释（# 开头）
  // =========================================================================
  {
    betools::Config cfg("# this is a comment\nkey=val\n# another comment", false);
    assert(cfg.GetValue("key") == "val");
    assert(cfg.GetValue("# this is a comment") == "");
  }

  // =========================================================================
  // 6. 行内注释（# 后的内容被忽略）
  // =========================================================================
  {
    betools::Config cfg("key=val  # inline comment", false);
    assert(cfg.GetValue("key") == "val");
  }

  // =========================================================================
  // 7. value 中包含 # 号（# 在 = 之前无意义，在 = 之后属于注释）
  // =========================================================================
  {
    betools::Config cfg("key=val#no_space_comment", false);
    // '#' 之后的内容被当作注释去掉，所以 value 是 "val"
    assert(cfg.GetValue("key") == "val");
  }

  // =========================================================================
  // 8. key 和 value 的首尾空白被 trim
  // =========================================================================
  {
    betools::Config cfg("  key1  =  val1  \n\tkey2\t=\tval2\t", false);
    assert(cfg.GetValue("key1") == "val1");
    assert(cfg.GetValue("key2") == "val2");
  }

  // =========================================================================
  // 9. 只有 key 没有 = 的行，value 为空字符串
  // =========================================================================
  {
    betools::Config cfg("key_only\nkey=val", false);
    assert(cfg.GetValue("key_only") == "");
    assert(cfg.GetValue("key") == "val");
  }

  // =========================================================================
  // 10. value 中包含 = 号，仅第一个 = 用于分割
  // =========================================================================
  {
    betools::Config cfg("key=val=ue=with=equals", false);
    assert(cfg.GetValue("key") == "val=ue=with=equals");
  }

  // =========================================================================
  // 11. 重复的 key，以最后一个为准
  // =========================================================================
  {
    betools::Config cfg("key=first\nkey=second\nkey=last", false);
    assert(cfg.GetValue("key") == "last");
  }

  // =========================================================================
  // 12. 不存在的 key 返回空字符串
  // =========================================================================
  {
    betools::Config cfg("a=1", false);
    assert(cfg.GetValue("nonexistent") == "");
  }

  // =========================================================================
  // 13. key 中不能包含 =（= 会被当作分隔符）
  // =========================================================================
  {
    betools::Config cfg("key=val", false);
    // 确认正常解析
    assert(cfg.GetValue("key") == "val");
  }

  // =========================================================================
  // 14. GetAs<std::string>
  // =========================================================================
  {
    betools::Config cfg("name=hello", false);
    std::string val = cfg.GetAs<std::string>("name");
    assert(val == "hello");
  }

  // =========================================================================
  // 15. GetAs<int>
  // =========================================================================
  {
    betools::Config cfg("port=8080\nnegative=-42\nzero=0", false);
    assert(cfg.GetAs<int>("port") == 8080);
    assert(cfg.GetAs<int>("negative") == -42);
    assert(cfg.GetAs<int>("zero") == 0);
  }

  // =========================================================================
  // 16. GetAs<long>
  // =========================================================================
  {
    betools::Config cfg("big=2147483648", false);
    assert(cfg.GetAs<long>("big") == 2147483648L);
  }

  // =========================================================================
  // 17. GetAs<long long>
  // =========================================================================
  {
    betools::Config cfg("huge=9223372036854775807", false);
    assert(cfg.GetAs<long long>("huge") == 9223372036854775807LL);
  }

  // =========================================================================
  // 18. GetAs<unsigned int>
  // =========================================================================
  {
    betools::Config cfg("val=4294967295", false);
    assert(cfg.GetAs<unsigned int>("val") == 4294967295U);
  }

  // =========================================================================
  // 19. GetAs<unsigned long>
  // =========================================================================
  {
    betools::Config cfg("val=4294967296", false);
    assert(cfg.GetAs<unsigned long>("val") == 4294967296UL);
  }

  // =========================================================================
  // 20. GetAs<unsigned long long>
  // =========================================================================
  {
    betools::Config cfg("val=18446744073709551615", false);
    assert(cfg.GetAs<unsigned long long>("val") == 18446744073709551615ULL);
  }

  // =========================================================================
  // 21. GetAs<float>
  // =========================================================================
  {
    betools::Config cfg("pi=3.14\nneg=-1.5", false);
    assert(cfg.GetAs<float>("pi") == 3.14f);
    assert(cfg.GetAs<float>("neg") == -1.5f);
  }

  // =========================================================================
  // 22. GetAs<double>
  // =========================================================================
  {
    betools::Config cfg("e=2.718281828\nsci=1.5e-3", false);
    assert(cfg.GetAs<double>("e") == 2.718281828);
    assert(cfg.GetAs<double>("sci") == 1.5e-3);
  }

  // =========================================================================
  // 23. GetAs<long double>
  // =========================================================================
  {
    betools::Config cfg("val=3.14159265358979323846", false);
    assert(cfg.GetAs<long double>("val") == 3.14159265358979323846L);
  }

  // =========================================================================
  // 24. GetAs<bool> — 合法的真值
  // =========================================================================
  {
    betools::Config cfg(
        "t1=1\n"
        "t2=true\n"
        "t3=TRUE\n"
        "t4=True\n"
        "t5=yes\n"
        "t6=YES\n"
        "t7=on\n"
        "t8=ON\n"
        "t9=y\n"
        "t10=Y\n"
        "t11=enable\n"
        "t12=ENABLE\n"
        "t13=enabled\n"
        "t14=ENABLED",
        false);
    assert(cfg.GetAs<bool>("t1") == true);
    assert(cfg.GetAs<bool>("t2") == true);
    assert(cfg.GetAs<bool>("t3") == true);
    assert(cfg.GetAs<bool>("t4") == true);
    assert(cfg.GetAs<bool>("t5") == true);
    assert(cfg.GetAs<bool>("t6") == true);
    assert(cfg.GetAs<bool>("t7") == true);
    assert(cfg.GetAs<bool>("t8") == true);
    assert(cfg.GetAs<bool>("t9") == true);
    assert(cfg.GetAs<bool>("t10") == true);
    assert(cfg.GetAs<bool>("t11") == true);
    assert(cfg.GetAs<bool>("t12") == true);
    assert(cfg.GetAs<bool>("t13") == true);
    assert(cfg.GetAs<bool>("t14") == true);
  }

  // =========================================================================
  // 25. GetAs<bool> — 合法的假值
  // =========================================================================
  {
    betools::Config cfg(
        "f1=0\n"
        "f2=false\n"
        "f3=FALSE\n"
        "f4=False\n"
        "f5=no\n"
        "f6=NO\n"
        "f7=off\n"
        "f8=OFF\n"
        "f9=n\n"
        "f10=N\n"
        "f11=disable\n"
        "f12=DISABLE\n"
        "f13=disabled\n"
        "f14=DISABLED\n"
        "f15=ignore\n"
        "f16=IGNORE\n"
        "f17=notfound\n"
        "f18=NOTFOUND",
        false);
    assert(cfg.GetAs<bool>("f1") == false);
    assert(cfg.GetAs<bool>("f2") == false);
    assert(cfg.GetAs<bool>("f3") == false);
    assert(cfg.GetAs<bool>("f4") == false);
    assert(cfg.GetAs<bool>("f5") == false);
    assert(cfg.GetAs<bool>("f6") == false);
    assert(cfg.GetAs<bool>("f7") == false);
    assert(cfg.GetAs<bool>("f8") == false);
    assert(cfg.GetAs<bool>("f9") == false);
    assert(cfg.GetAs<bool>("f10") == false);
    assert(cfg.GetAs<bool>("f11") == false);
    assert(cfg.GetAs<bool>("f12") == false);
    assert(cfg.GetAs<bool>("f13") == false);
    assert(cfg.GetAs<bool>("f14") == false);
    assert(cfg.GetAs<bool>("f15") == false);
    assert(cfg.GetAs<bool>("f16") == false);
    assert(cfg.GetAs<bool>("f17") == false);
    assert(cfg.GetAs<bool>("f18") == false);
  }

  // =========================================================================
  // 26. GetAs<bool> — trim 后的空白对布尔值解析无影响
  // =========================================================================
  {
    betools::Config cfg("b1= true \nb2= false ", false);
    assert(cfg.GetAs<bool>("b1") == true);
    assert(cfg.GetAs<bool>("b2") == false);
  }

  // =========================================================================
  // 27. GetAs 对不存在的 key 抛出 std::runtime_error
  // =========================================================================
  {
    betools::Config cfg("a=1", false);
    bool caught = false;
    try {
      cfg.GetAs<int>("nonexistent");
    } catch (const std::runtime_error&) {
      caught = true;
    }
    assert(caught);
  }

  // =========================================================================
  // 28. GetAs<bool> 对非法布尔字面量抛出 std::runtime_error
  // =========================================================================
  {
    betools::Config cfg("bad=maybe", false);
    bool caught = false;
    try {
      cfg.GetAs<bool>("bad");
    } catch (const std::runtime_error&) {
      caught = true;
    }
    assert(caught);
  }

  // =========================================================================
  // 29. GetAs<int> 对非数字值应抛出异常（由 std::stoi 抛出）
  // =========================================================================
  {
    betools::Config cfg("val=not_a_number", false);
    bool caught = false;
    try {
      cfg.GetAs<int>("val");
    } catch (const std::exception&) {
      caught = true;
    }
    assert(caught);
  }

  // =========================================================================
  // 30. 综合场景：模拟一个完整的配置文件
  // =========================================================================
  {
    std::string config_str =
        "# Database Configuration\n"
        "host = 127.0.0.1\n"
        "port = 5432\n"
        "username = admin\n"
        "password = secret\n"
        "max_connections = 100\n"
        "timeout = 30.5\n"
        "use_ssl = true\n"
        "debug = false\n"
        "\n"
        "# Logging\n"
        "log_level = info\n"
        "log_file = /var/log/app.log\n"
        "\n"
        "# empty value key\n"
        "empty_key\n";
    betools::Config cfg(config_str, false);

    assert(cfg.GetValue("host") == "127.0.0.1");
    assert(cfg.GetAs<int>("port") == 5432);
    assert(cfg.GetValue("username") == "admin");
    assert(cfg.GetValue("password") == "secret");
    assert(cfg.GetAs<int>("max_connections") == 100);
    assert(cfg.GetAs<double>("timeout") == 30.5);
    assert(cfg.GetAs<bool>("use_ssl") == true);
    assert(cfg.GetAs<bool>("debug") == false);
    assert(cfg.GetValue("log_level") == "info");
    assert(cfg.GetValue("log_file") == "/var/log/app.log");
    assert(cfg.GetValue("empty_key") == "");
  }

  // =========================================================================
  // 31. 只有注释的配置
  // =========================================================================
  {
    betools::Config cfg("# only comments\n# no keys here", false);
    assert(cfg.GetValue("anything") == "");
  }

  // =========================================================================
  // 32. 单行配置
  // =========================================================================
  {
    betools::Config cfg("single_key=single_value", false);
    assert(cfg.GetValue("single_key") == "single_value");
  }

  // =========================================================================
  // 33. value 中包含首尾空格，trim 后不保留
  // =========================================================================
  {
    betools::Config cfg("key=  value with spaces  ", false);
    assert(cfg.GetValue("key") == "value with spaces");
  }

  // =========================================================================
  // 34. GetAs<unsigned short>
  // =========================================================================
  {
    betools::Config cfg("val=65535", false);
    assert(cfg.GetAs<unsigned short>("val") == 65535);
  }

  // =========================================================================
  // 35. GetAs<short> — 包含负数
  // =========================================================================
  {
    betools::Config cfg("val=-32768", false);
    assert(cfg.GetAs<short>("val") == -32768);
  }

  return EXIT_SUCCESS;
}
