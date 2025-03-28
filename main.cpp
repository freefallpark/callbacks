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

//region Signal Handling
sig_atomic_t stop = 0;
void SignalHandler(int signal){
  stop = signal;
}
//endregion

}  // namespace
int main() {
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);
  OnClientLost on_client_lost_;
  on_client_lost_.RegisterCallback([]()->void{std::cout << "handling lost client\n";});
  on_client_lost_.Call();

/*
  TigerKingZooManager joe_exotic;
  PhoenixZooManager bert_castro;
  HogleZooManager doug_lund;

  //Init
  joe_exotic.OpenZoo();
  bert_castro.OpenZoo();
  doug_lund.OpenZoo();

  // Run
  while(true){
    if(stop != 0) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  //Close
  joe_exotic.CloseZoo();
  bert_castro.CloseZoo();
  doug_lund.CloseZoo();
*/

  return 0;
}
