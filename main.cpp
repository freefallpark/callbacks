#include <csignal>
#include <iostream>
#include <thread>

#include "tools/callback_tools.h"

namespace{

class ZooKeeperBase{
 public:
  virtual void FeedTheAnimals() = 0;
};

//region Inherited Callbacks
class ZookeeperTasks{
 public:
  virtual void FeedTheAnimals() = 0; // mandatory callback
};
class ZooKeeperA :public ZooKeeperBase{
 public:
  explicit ZooKeeperA(ZookeeperTasks &task_definitions) : task_definitions_(task_definitions){}
  void FeedTheAnimals()override{
    task_definitions_.FeedTheAnimals();
  }

 private:
  ZookeeperTasks & task_definitions_;
};
//endregion

//region std::function Callbacks
class ZooKeeperB : public ZooKeeperBase{
 public:
  ZooKeeperB() {
    // Define default behavior here:
    cb_feed_the_animals_.RegisterCallback([]() -> void {
      ZooKeeperB::DefaultFeedTheAnimals_();
    });
  }

  void FeedTheAnimals()override{
    cb_feed_the_animals_();
  }
  //Expose Callback reference so owning scope can redefine callback
  Callback<void()>& GetFeedTheAnimalsCallback(){
    return cb_feed_the_animals_;
  }
 private:
  static void DefaultFeedTheAnimals_(){
    std::cout << "I haven't been trained to feed animals yet? Where is the food?" << std::endl;
  }

  Callback<void()> cb_feed_the_animals_;
};
//endregion

class ZooManagerBase{
 protected:
  ZooManagerBase() : food_stock_(100){}
  virtual void OpenZoo() = 0;
  virtual void CloseZoo() = 0;
  int food_stock_;
};

class TigerKingZooManager : public ZookeeperTasks, ZooManagerBase{
 public:
  TigerKingZooManager()
      : tiger_zookeeper_(*this){}

  void OpenZoo() override{
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Opening The Greater Wynnewood Exotic Animal Park" << std::endl;
    tiger_zookeeper_.FeedTheAnimals();
    std::cout << "------------------------------------------------" << std::endl;
  }

  void CloseZoo() override{
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Closing the Phoenix Zoo" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
  }

 private:
  // Define Zookeepers tasks
  void FeedTheAnimals() override {
    std::cout << "Feeding the tigers" << std::endl;
    food_stock_ = food_stock_ - 1;
  }

  ZooKeeperA tiger_zookeeper_;

//   what happens if we hire a second zoo_keeper to manage Tigers?
//  ZooKeeperA lion_zookeeper_;
};

class PhoenixZooManager final : ZooManagerBase{
 public:
  PhoenixZooManager()
      : tiger_keeper_tasks_(food_stock_),
        tiger_zookeeper_(tiger_keeper_tasks_),
        lion_keeper_tasks_(food_stock_),
        lion_zookeeper_(lion_keeper_tasks_){}

  void OpenZoo() override {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Opening The Phoenix Zoo" << std::endl;
    tiger_zookeeper_.FeedTheAnimals();
    lion_zookeeper_.FeedTheAnimals();
    std::cout << "------------------------------------------------" << std::endl;
  }

  void CloseZoo() override {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Closing The Phoenix Zoo" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
  }

 private:
  class TigerKeeperTasks final : public ZookeeperTasks{
   public:
    explicit TigerKeeperTasks(int &food_stock) : food_stock_reference_(food_stock){}
    void FeedTheAnimals() override {
      std::cout << "Feeding the Tigers" << std::endl;
      // Can't access food_stock, directly... :(
      std::cout << "Hey Manager, I'm gonna go reference your food stores, just a heads up..." << std::endl;
      food_stock_reference_ = food_stock_reference_-1;
    }

   private:
    int &food_stock_reference_;
  };
  class LionKeeperTasks final : public ZookeeperTasks{
   public:
    explicit LionKeeperTasks(int &food_stock) : food_stock_reference_(food_stock){}
    void FeedTheAnimals() override {
      std::cout << "Feeding the Lions" << std::endl;
      // Can't access food_stock, directly... :(
      std::cout << "Hey Manager, I'm gonna go reference your food stores, just a heads up..." << std::endl;
      food_stock_reference_ = food_stock_reference_-2;
    }

   private:
    int &food_stock_reference_;
  };

  TigerKeeperTasks tiger_keeper_tasks_;
  ZooKeeperA tiger_zookeeper_;
  LionKeeperTasks lion_keeper_tasks_;
  ZooKeeperA lion_zookeeper_;

};

class HogleZooManager final : public ZooManagerBase{
 public:
  HogleZooManager(){
    tiger_zookeeper_.GetFeedTheAnimalsCallback().RegisterCallback([this]() { this->FeedTigers(); });
    lion_zookeeper_.GetFeedTheAnimalsCallback().RegisterCallback([this]() { this->FeedLions(); });
    giraffe_zookeeper_.GetFeedTheAnimalsCallback().RegisterCallback([this]() { this->FeedGiraffes(); });
  }
  void OpenZoo() override {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Opening The Hogle Zoo" << std::endl;
    tiger_zookeeper_.FeedTheAnimals();
    lion_zookeeper_.FeedTheAnimals();
    giraffe_zookeeper_.FeedTheAnimals();
    pig_zookeeper_.FeedTheAnimals();
    std::cout << "------------------------------------------------" << std::endl;
  }

  void CloseZoo() override {
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "Closing the Hogle Zoo" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
  }

 private:
  void FeedTigers(){
    std::lock_guard lock(mtx_);
    std::cout << "I'm feeding Tigers" << std::endl;
    food_stock_ = food_stock_ -1;
  }
  void FeedLions(){
    std::lock_guard lock(mtx_);
    std::cout << "I'm feeding Lions" << std::endl;
    food_stock_ = food_stock_ -2;
  };
  void FeedGiraffes(){
    std::lock_guard lock(mtx_);
    std::cout << "I'm feeding giraffes" << std::endl;
    food_stock_ = food_stock_ -3;
  }

  ZooKeeperB tiger_zookeeper_;
  ZooKeeperB lion_zookeeper_;
  ZooKeeperB giraffe_zookeeper_;
  ZooKeeperB pig_zookeeper_{};

  std::mutex mtx_;
};
int ZooExample(sig_atomic_t & external_stop){
  TigerKingZooManager joe_exotic;
  PhoenixZooManager bert_castro;
  HogleZooManager doug_lund;

  //Init
  joe_exotic.OpenZoo();
  bert_castro.OpenZoo();
  doug_lund.OpenZoo();

  // Run
  while(true){
    if(external_stop != 0) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  //Close
  joe_exotic.CloseZoo();
  bert_castro.CloseZoo();
  doug_lund.CloseZoo();

  return 0;
}

//region Signal Handling
sig_atomic_t stop = 0;
void SignalHandler(int signal){
  stop = signal;
}
//endregion

class CallbackBenchmarkClass{
 public:
  virtual ~CallbackBenchmarkClass() = default;
  virtual void CallAll() = 0;
  int GetI(){return i;}
 protected:
  int i = 0;
};
class TestCallbacks{
 protected:
  virtual void on_event_1() {};
  virtual void on_event_2() {};
  virtual void on_event_3() {};
  virtual void on_event_4() {};
  virtual void on_event_5() {};
  virtual void on_event_6() {};
  virtual void on_event_7() {};
  virtual void on_event_8() {};
  virtual void on_event_9() {};
  virtual void on_event_10() {};
};
class TestInheritedCallbacks : public TestCallbacks, public CallbackBenchmarkClass{
 public:
  void CallAll() override{
    on_event_1();
    on_event_2();
    on_event_3();
    on_event_4();
    on_event_5();
    on_event_6();
    on_event_7();
    on_event_8();
    on_event_9();
    on_event_10();
  }
 private:
  void on_event_1() override{i = i + 1; }
  void on_event_2() override{i = i - 1; }
  void on_event_3() override{i = i + 2; }
  void on_event_4() override{i = i - 2; }
  void on_event_5() override{i = i + 3; }
  void on_event_6() override{i = i - 3; }
  void on_event_7() override{i = i + 4; }
  void on_event_8() override{i = i - 4; }
  void on_event_9() override{i = i + 5; }
  void on_event_10() override{ i = i -5;}
};
class TestTemplatedCallbacks : public CallbackBenchmarkClass{
 public:
  TestTemplatedCallbacks(){
    on_event_1.RegisterCallback([this](){this->event_handler_1();});
    on_event_2.RegisterCallback([this](){this->event_handler_2();});
    on_event_3.RegisterCallback([this](){this->event_handler_3();});
    on_event_4.RegisterCallback([this](){this->event_handler_4();});
    on_event_5.RegisterCallback([this](){this->event_handler_5();});
    on_event_6.RegisterCallback([this](){this->event_handler_6();});
    on_event_7.RegisterCallback([this](){this->event_handler_7();});
    on_event_8.RegisterCallback([this](){this->event_handler_8();});
    on_event_9.RegisterCallback([this](){this->event_handler_9();});
    on_event_10.RegisterCallback([this](){this->event_handler_10();});
  }
  void CallAll() override{
    on_event_1();
    on_event_2();
    on_event_3();
    on_event_4();
    on_event_5();
    on_event_6();
    on_event_7();
    on_event_8();
    on_event_9();
    on_event_10();
  }

 private:
  void event_handler_1(){i = i + 1; }
  void event_handler_2(){i = i - 1; }
  void event_handler_3(){i = i + 2; }
  void event_handler_4(){i = i - 2; }
  void event_handler_5(){i = i + 3; }
  void event_handler_6(){i = i - 3; }
  void event_handler_7(){i = i + 4; }
  void event_handler_8(){i = i - 4; }
  void event_handler_9(){i = i + 5; }
  void event_handler_10(){i = i - 5; }

  Callback<void()> on_event_1;
  Callback<void()> on_event_2;
  Callback<void()> on_event_3;
  Callback<void()> on_event_4;
  Callback<void()> on_event_5;
  Callback<void()> on_event_6;
  Callback<void()> on_event_7;
  Callback<void()> on_event_8;
  Callback<void()> on_event_9;
  Callback<void()> on_event_10;
};

//Subclass Inheritance solution idea:
class Object1EventCallbacks{
 public:
  virtual void on_event_1() = 0;
  virtual void on_event_2() = 0;
  virtual void on_event_3() = 0;
  virtual void on_event_4() = 0;
  virtual void on_event_5() = 0;
  virtual void on_event_6() = 0;
  virtual void on_event_7() = 0;
  virtual void on_event_8() = 0;
  virtual void on_event_9() = 0;
  virtual void on_event_10() = 0;
};
class Object1{
 public:
  explicit Object1(Object1EventCallbacks &callbacks) : callbacks_(callbacks){}
  void Trigger(){
    callbacks_.on_event_1();
    callbacks_.on_event_2();
    callbacks_.on_event_3();
    callbacks_.on_event_4();
    callbacks_.on_event_5();
    callbacks_.on_event_6();
    callbacks_.on_event_7();
    callbacks_.on_event_8();
    callbacks_.on_event_9();
    callbacks_.on_event_10();
  }
 private:
  Object1EventCallbacks &callbacks_;
};

class Object2EventCallbacks{
 public:
  virtual void on_event_1() = 0;
};
class Object2{
 public:
  explicit Object2(Object2EventCallbacks &callbacks) : callbacks_(callbacks){}
  void Trigger(){
    callbacks_.on_event_1();
  }
 private:
  Object2EventCallbacks &callbacks_;
};

class SubClassInheritedCallbacks : public CallbackBenchmarkClass{
 public:
  SubClassInheritedCallbacks()
      : object_1_event_handler_(*this),
        object_1_(object_1_event_handler_),
        i_(0){ };
  void Trigger(){
    object_1_.Trigger();
  }

 private:
  //Define object 1 and object 2 event callbacks here
  class Object1EventHandler final : public Object1EventCallbacks{
   public:
    explicit Object1EventHandler(SubClassInheritedCallbacks &process_class) : process_class_(process_class){}
    void on_event_1() override {process_class_.OnObject1Event1();};
    void on_event_2() override {process_class_.OnObject1Event2();};
    void on_event_3() override {process_class_.OnObject1Event3();};
    void on_event_4() override {process_class_.OnObject1Event4();};
    void on_event_5() override {process_class_.OnObject1Event5();};
    void on_event_6() override {process_class_.OnObject1Event6();};
    void on_event_7() override {process_class_.OnObject1Event7();};
    void on_event_8() override {process_class_.OnObject1Event8();};
    void on_event_9() override {process_class_.OnObject1Event9();};
    void on_event_10() override {process_class_.OnObject1Event10();};


   private:
    SubClassInheritedCallbacks &process_class_;
  };

  // Define Event Handling:
  void OnObject1Event1(){i_ = i_ + 1; }
  void OnObject1Event2(){i_ = i_ - 1; }
  void OnObject1Event3(){i_ = i_ + 2; }
  void OnObject1Event4(){i_ = i_ - 2; }
  void OnObject1Event5(){i_ = i_ + 3; }
  void OnObject1Event6(){i_ = i_ - 3; }
  void OnObject1Event7(){i_ = i_ + 4; }
  void OnObject1Event8(){i_ = i_ - 4; }
  void OnObject1Event9(){i_ = i_ + 5; }
  void OnObject1Event10(){i_ = i_ - 5; }

  void CallAll() override {
    object_1_.Trigger();
  }

  Object1EventHandler object_1_event_handler_;
  Object1 object_1_;
  int i_;
};

void BenchMark(CallbackBenchmarkClass & benchmark, const std::string & test_name){
  int num_tests = 100000;
  auto end = std::chrono::steady_clock::now();
  auto start = std::chrono::steady_clock::now();
  for(int i = 0; i<num_tests; i++){
    benchmark.CallAll();
  }
  end = std::chrono::steady_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
  std::cout << test_name << " Total Duration: " << total_duration << "\tmicroseconds ";
}
int BenchMarkCallbackTypes(){
  std::unique_ptr<CallbackBenchmarkClass> ic = std::make_unique<TestInheritedCallbacks>();
  ic->CallAll();
  BenchMark(*ic, "ic  ");
  std::cout << ic->GetI() << std::endl;

  std::unique_ptr<CallbackBenchmarkClass> scic = std::make_unique<SubClassInheritedCallbacks>();
  scic->CallAll();
  BenchMark(*scic, "scic");
  std::cout << scic->GetI() << std::endl;

  std::unique_ptr<CallbackBenchmarkClass> tc = std::make_unique<TestTemplatedCallbacks>();
  BenchMark(*tc, "tc  ");
  tc->CallAll();
  std::cout << tc->GetI() << std::endl;
  return 0;
}

int MultiInheritSolutionTest(){
  SubClassInheritedCallbacks process;
  process.Trigger();
  return 0;
}

}  // namespace
int main() {
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  BenchMarkCallbackTypes();
  return 0;
}
