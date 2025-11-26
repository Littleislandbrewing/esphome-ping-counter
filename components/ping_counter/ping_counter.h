#pragma once

#include "esphome.h"
#include "esphome/components/network/util.h"
#include <AsyncPing.h>

namespace esphome {
namespace ping_counter {

class PingCounter : public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override; // <--- NEW: Standard logging

  void set_target_ip(const std::string &ip) { target_ip_ = ip; }
  void set_threshold(int t) { threshold_ = t; }
  void set_alert_sensor(binary_sensor::BinarySensor *s) { alert_sensor_ = s; }

 protected:
  std::string target_ip_;
  int threshold_ = 10;
  
  int consecutive_failures_ = 0;
  bool is_running_{false};
  uint32_t last_ping_start_ = 0;
  
  // Logic tracker for the "Pulse" behavior
  bool is_pulsing_{false}; 

  binary_sensor::BinarySensor *alert_sensor_{nullptr};
  AsyncPing ping_handler_;
  
  void process_result_(bool success);
};

}  // namespace ping_counter
}  // namespace esphome
