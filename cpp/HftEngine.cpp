#include <vector>
#include "../node_modules/@1057405bcltd/compute-orders-addon/cpp/HftEngine.h"
unsigned int iteration = 0;


NextOrders ComputeOrders(
	double &feeRate,
	double &quantityLimit,
	double &updateThreshold,
	double &placeThreshold,
	Tuning &tuning1,
	Tuning &tuning2,
	OrderBook &orderBook,
	Order &incumbentBuyOrder,
	Order &incumbentSellOrder)
{
	return make_tuple(0.10100001, 0.10599999, 0, "C++ Addon Demonstration");

}
