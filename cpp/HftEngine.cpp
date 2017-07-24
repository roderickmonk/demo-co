#include <vector>
#include "../node_modules/@1057405bcltd/compute-orders-addon/cpp/HftEngine.h"
unsigned int iteration = 0;

NextOrders incumbentOrderQuantities(Order &incumbentBuyOrder, Order &incumbentSellOrder)
{
	return make_pair(incumbentBuyOrder.second, incumbentSellOrder.second);
}

NextOrders incumbentOrderRates(Order &incumbentBuyOrder, Order &incumbentSellOrder)
{
	return make_pair(incumbentBuyOrder.first, incumbentSellOrder.first);
}

NextOrders sumOrderBookQualities(OrderBook &orderBook)
{
	double buySum = 0;
	for (unsigned int i = 0; i < orderBook.first.size(); ++i)
	{
		buySum += orderBook.first[i].second;
	}
	//cout << "sumBuyOrderBookRates: " << sumBuyOrderBookRates << endl;

	double sellSum = 0;
	for (unsigned int i = 0; i < orderBook.second.size(); ++i)
	{
		sellSum += orderBook.second[i].second;
	}
	//cout << "sumSellOrderBookRates: " << sumBuyOrderBookRates << endl;

	return make_pair(buySum, sellSum);
}

NextOrders sumOrderBookRates(OrderBook &orderBook)
{
	//cout << "size of buy orderbook: " << orderBook.first.size() << endl;
	//cout << "size of sell orderbook: " << orderBook.second.size() << endl;

	double sumBuyOrderBookRates = 0;
	for (unsigned int i = 0; i < orderBook.first.size(); ++i)
	{
		sumBuyOrderBookRates += orderBook.first[i].first;
		//cout << orderBook.first[i].first;
	}
	//cout << "sumBuyOrderBookRates: " << sumBuyOrderBookRates << endl;

	double sumSellOrderBookRates = 0;
	for (unsigned int i = 0; i < orderBook.second.size(); ++i)
	{
		sumSellOrderBookRates += orderBook.second[i].first;
	}
	//cout << "sumSellOrderBookRates: " << sumBuyOrderBookRates << endl;

	return make_pair(sumBuyOrderBookRates, sumSellOrderBookRates);
}

NextOrders sumTuning(Tuning &tuning1, Tuning &tuning2)
{
	double sumTuning1 = 0;
	for (uint i = 0; i < tuning1.size(); ++i)
	{
		sumTuning1 += tuning1[i];
	}

	double sumTuning2 = 0;
	for (uint i = 0; i < tuning2.size(); ++i)
	{
		sumTuning2 += tuning2[i];
	}

	return make_pair(sumTuning1, sumTuning2);
}

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

	switch (iteration++)
	{
	case 0:
		return sumTuning(tuning1, tuning2);
	case 1:
		return sumOrderBookRates(orderBook);
	case 2:
		return sumOrderBookQualities(orderBook);
	default:
		return make_pair(0, 0);
	}
}