#include <vector>
#include <string>
#include <iostream>
#include <tuple>

using namespace std;

template <typename T>
void printVector(const vector<T> &v)
{
  for (auto &&e : v)
  {
    std::cout << e << std::endl;
  }
}

typedef pair<double, double> Order;
typedef vector<Order> BuyOrders;
typedef vector<Order> SellOrders;
typedef pair<BuyOrders, SellOrders> OrderBook;
typedef tuple<double, double, int, std::string> NextOrders;
typedef vector<double> Tuning;

NextOrders ComputeOrders(
    double &feeRate,
    double &quantityLimit,
    double &updateThreshold,
    double &placeThreshold,
    Tuning &tuning1,
    Tuning &tuning2,
    OrderBook &orderBook,
    Order &buyIncumbentOrder,
    Order &sellIncumbentOrder);
