#pragma once

#include "esphome.h"

// NATIVE ESP-IDF HEADERS
// These are standard in the ESP-IDF framework
#include <ping/ping_sock.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

namespace esphome {
namespace ping_counter {

class PingCounter : public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void set_target_ip(const std::string &ip) { target_ip_ = ip; }
  void set_threshold(int t) { threshold_ = t; }
  void set_alert_sensor(binary_sensor::BinarySensor *s) { alert_sensor_ = s; }

  // Internal handler called by the callbacks
  void _process_ping_result(bool success);

 protected:
  std::string target_ip_;
  int threshold_ = 10;
  int consecutive_failures_ = 0;
  
  // State flags
  bool is_running_{false};
  bool pulse_reset_pending_{false};
  
  binary_sensor::BinarySensor *alert_sensor_{nullptr};

  // Static Callbacks (Required for C-API compatibility)
  static void on_ping_success(esp_ping_handle_t hdl, void *args);
  static void on_ping_timeout(esp_ping_handle_t hdl, void *args);
  static void on_ping_end(esp_ping_handle_t hdl, void *args);
};

}  // namespace ping_counter
}  // namespace esphome
