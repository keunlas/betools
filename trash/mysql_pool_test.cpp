#include "betoolx/mysql_pool.hpp"

#include <iostream>

int main() {
  using namespace betoolx;

  MysqlPool pool("127.0.0.1", 3306, "user", "password", "test");
  auto conn = pool.Borrow();
  if (!conn) {
    std::cerr << "Borrow err" << '\n';
  }

  std::cout << "mysql_ping: " << mysql_ping(conn->Raw()) << '\n';
}
