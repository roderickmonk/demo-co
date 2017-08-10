#include <node.h>
#include <node_object_wrap.h>
#include <iostream>
#include <cmath>
#include <node.h>
#include <v8.h>
#include <uv.h>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace std;
using namespace v8;

#include "HftEngine.h"

Order unpackOrder(Isolate *isolate, const Handle<Object> orderObj)
{
  Handle<Value> rate = orderObj->Get(String::NewFromUtf8(isolate, "rate"));
  Handle<Value> quantity = orderObj->Get(String::NewFromUtf8(isolate, "quantity"));

  return make_pair(rate->NumberValue(), quantity->NumberValue());
}

class HftEngine : public node::ObjectWrap
{
public:
  static void Init(v8::Local<v8::Object> exports)
  {
    Isolate *isolate = exports->GetIsolate();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "ComputeOrders"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "ComputeOrders", ComputeOrders);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "Engine"),
                 tpl->GetFunction());
  }

private:
  explicit HftEngine(
      double feeRate,
      double quantityLimit,
      double updateThreshold,
      double placeThreshold,
      vector<double> tuning1,
      vector<double> tuning2) : feeRate_(feeRate),
                                quantityLimit_(quantityLimit),
                                updateThreshold_(updateThreshold),
                                placeThreshold_(placeThreshold),
                                tuning1_(tuning1),
                                tuning2_(tuning2) {}
  ~HftEngine() {}

  static void New(const v8::FunctionCallbackInfo<v8::Value> &args)
  {
    cout << "junk 1" << endl;
    // Capture the parameters
    double feeRate = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
    double quantityLimit = args[1]->IsUndefined() ? 0 : args[1]->NumberValue();
    double updateThreshold = args[2]->IsUndefined() ? 0 : args[2]->NumberValue();
    double placeThreshold = args[3]->IsUndefined() ? 0 : args[3]->NumberValue();

    // Capture tuning1
    Local<Array> tuning1Array = Local<Array>::Cast(args[4]);
    std::vector<double> tuning1;
    for (unsigned int i = 0; i < tuning1Array->Length(); i++)
    {
      tuning1.push_back(tuning1Array->Get(i)->NumberValue());
    }

    cout << "junk 2" << endl;

    // Capture tuning2
    Local<Array> tuning2Array = Local<Array>::Cast(args[5]);
    std::vector<double> tuning2;
    for (unsigned int i = 0; i < tuning2Array->Length(); i++)
    {
      tuning2.push_back(tuning2Array->Get(i)->NumberValue());
    }

    // Create the new HftEngine instance
    HftEngine *obj = new HftEngine(feeRate, quantityLimit, updateThreshold, placeThreshold, tuning1, tuning2);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }

  static void ComputeOrders(const v8::FunctionCallbackInfo<v8::Value> &args);

  static v8::Persistent<v8::Function> constructor;

public:
  double feeRate_;
  double quantityLimit_;
  double updateThreshold_;
  double placeThreshold_;
  vector<double> tuning1_;
  vector<double> tuning2_;
};

Persistent<Function> HftEngine::constructor;

struct HftEngineWork
{
  uv_work_t request;

  Persistent<Function> callback;

  HftEngine *hftEngine;
  OrderBook orderBook;
  Order incumbentBuyOrder;
  Order incumbentSellOrder;
  NextOrders nextOrders;
};

// called by libuv worker in separate thread
static void HftEngineAsync(uv_work_t *req)
{
  HftEngineWork *work = static_cast<HftEngineWork *>(req->data);

  printVector<double>(work->hftEngine->tuning1_);

  work->nextOrders = ComputeOrders(
      work->hftEngine->feeRate_,
      work->hftEngine->quantityLimit_,
      work->hftEngine->updateThreshold_,
      work->hftEngine->placeThreshold_,
      work->hftEngine->tuning1_,
      work->hftEngine->tuning2_,
      work->orderBook,
      work->incumbentBuyOrder,
      work->incumbentSellOrder);
}

// called by libuv in event loop when async function completes
static void HftEngineAsyncComplete(uv_work_t *req, int status)
{
  Isolate *isolate = Isolate::GetCurrent();

  v8::HandleScope handleScope(isolate);

  HftEngineWork *work = static_cast<HftEngineWork *>(req->data);

  // set up return arguments
  if (get<2>(work->nextOrders) == 0)
  {
    Local<Object> errorObj = Object::New(isolate);
    errorObj->Set(String::NewFromUtf8(isolate, "status"), Number::New(isolate, 0));
    errorObj->Set(String::NewFromUtf8(isolate, "message"), String::NewFromUtf8(isolate->GetCurrent(), ""));

    Local<Array> nextOrders = Array::New(isolate);
    nextOrders->Set(0, Number::New(isolate, get<0>(work->nextOrders)));
    nextOrders->Set(1, Number::New(isolate, get<1>(work->nextOrders)));

    Handle<Value> argv[] = {errorObj, nextOrders};
    Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);

  }
  else if (get<2>(work->nextOrders) > 0)
  {
    Local<Object> errorObj = Object::New(isolate);
    errorObj->Set(String::NewFromUtf8(isolate, "status"), Number::New(isolate, get<2>(work->nextOrders)));
    errorObj->Set(String::NewFromUtf8(isolate, "message"), String::NewFromUtf8(isolate->GetCurrent(), get<3>(work->nextOrders).c_str()));

    Local<Array> nextOrders = Array::New(isolate);
    nextOrders->Set(0, Number::New(isolate, get<0>(work->nextOrders)));
    nextOrders->Set(1, Number::New(isolate, get<1>(work->nextOrders)));

    Handle<Value> argv[] = {errorObj, nextOrders};
    Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 2, argv);

  } else { // status < 0

    Local<Object> errorObj = Object::New(isolate);
    errorObj->Set(String::NewFromUtf8(isolate, "status"), Number::New(isolate, get<2>(work->nextOrders)));
    errorObj->Set(String::NewFromUtf8(isolate, "message"), String::NewFromUtf8(isolate->GetCurrent(), get<3>(work->nextOrders).c_str()));

    Handle<Value> argv[] = {errorObj, Null(isolate)};
    Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 2, argv); 

  }

  // Free up the persistent function callback
  work->callback.Reset();
  delete work;
}

void HftEngine::ComputeOrders(const v8::FunctionCallbackInfo<v8::Value> &args)
{
  Isolate *isolate = args.GetIsolate();

  auto *work = new HftEngineWork();
  work->request.data = work;
  work->hftEngine = ObjectWrap::Unwrap<HftEngine>(args.Holder());

  // Copy the buy orders into the C++ order book
  Local<Array> buyOrders = Local<Array>::Cast(args[0]);
  unsigned int buyCount = buyOrders->Length();
  for (unsigned int i = 0; i < buyCount; i++)
  {
    work->orderBook.first.push_back(unpackOrder(isolate, Handle<Object>::Cast(buyOrders->Get(i))));
  }

  // Copy the sell orders into the C++ order book
  Local<Array> sellOrders = Local<Array>::Cast(args[1]);
  unsigned int sellCount = sellOrders->Length();
  for (unsigned int i = 0; i < sellCount; i++)
  {
    work->orderBook.second.push_back(unpackOrder(isolate, Handle<Object>::Cast(sellOrders->Get(i))));
  }

  // Get the incumbent orders
  Local<Object> incumbentBuyOrder = args[2]->ToObject();
  work->incumbentBuyOrder = unpackOrder(isolate, Handle<Object>::Cast(incumbentBuyOrder));
  Local<Object> incumbentSellOrder = args[3]->ToObject();
  work->incumbentSellOrder = unpackOrder(isolate, Handle<Object>::Cast(incumbentSellOrder));

  /*
  cout << "incumbentBuyOrder.rate: " << work->incumbentBuyOrder.first << endl;
  cout << "incumbentBuyOrder.quantity: " << work->incumbentBuyOrder.second << endl;

  cout << "incumbentSellOrder.rate: " << work->incumbentSellOrder.first << endl;
  cout << "incumbentSellOrder.quantity: " << work->incumbentSellOrder.second << endl;
*/

  // store the callback from JS in the work package so we can invoke it later
  Local<Function> callback = Local<Function>::Cast(args[4]);
  work->callback.Reset(isolate, callback);

  // kick off the worker thread
  uv_queue_work(uv_default_loop(), &work->request, HftEngineAsync, HftEngineAsyncComplete);
  args.GetReturnValue().Set(Undefined(isolate));
}

void InitHftEngine(Local<Object> exports)
{
  HftEngine::Init(exports);
}

NODE_MODULE(hftengine, InitHftEngine)