#include "ping_counter.h"
#include "esphome/components/network/util.h"

namespace esphome {
namespace ping_counter {

// --- STATIC CALLBACK WRAPPERS ---
// These run in the context of the LwIP Ping Task.
// We use defer() to safely schedule the update on the ESPHome Main Loop.

void PingCounter::on_ping_success(esp_ping_handle_t hdl, void *args) {
  PingCounter *instance = (PingCounter *)args;
  instance->defer([instance]() {
      instance->_process_ping_result(true);
  });
}

void PingCounter::on_ping_timeout(esp_ping_handle_t hdl, void *args) {
  PingCounter *instance = (PingCounter *)args;
  instance->defer([instance]() {
      instance->_process_ping_result(false);
  });
}

void PingCounter::on_ping_end(esp_ping_handle_t hdl, void *args) {
  PingCounter *instance = (PingCounter *)args;
  
  // CRITICAL MEMORY MANAGEMENT: 
  // The ping is done. We MUST delete the session handle or we will leak RAM.
  esp_ping_delete_session(hdl);
  
  // Release the lock so the next update cycle can run
  instance->defer([instance]() {
    instance->is_running_ = false;
  });
}
// ---------------------------------

void PingCounter::setup() {
  if (this->alert_sensor_ != nullptr) {
    this->alert_sensor_->publish_state(false);
  }
}

void PingCounter::dump_config() {
  ESP_LOGCONFIG("ping_counter", "Ping Counter (Native ESP-IDF):");
  ESP_LOGCONFIG("ping_counter", "  Target IP: %s", this->target_ip_.c_str());
  ESP_LOGCONFIG("ping_counter", "  Threshold: %d", this->threshold_);
}

void PingCounter::update() {
  // 1. Connectivity Check
  if (!network::is_connected()) return;

  // 2. Pulse Reset Logic
  if (this->pulse_reset_pending_) {
    this->pulse_reset_pending_ = false;
    if (this->alert_sensor_ != nullptr) {
      this->alert_sensor_->publish_state(false);
    }
  }

  // 3. Overlap Protection
  if (this->is_running_) {
    ESP_LOGV("ping_counter", "Skipping: Ping already in progress.");
    return;
  }

  // 4. Parse IP (Native LwIP)
  ip_addr_t target_addr;
  memset(&target_addr, 0, sizeof(target_addr));
  
  // ipaddr_aton returns 1 on success.
  if (!ipaddr_aton(this->target_ip_.c_str(), &target_addr)) {
     ESP_LOGE("ping_counter", "Invalid IP Address format: %s", this->target_ip_.c_str());
     return;
  }

  // 5. Configure Session
  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
  ping_config.target_addr = target_addr;
  ping_config.count = 1; // Send exactly 1 packet
  ping_config.timeout_ms = 1000; // 1 second timeout for the packet

  // 6. Setup Callbacks
  esp_ping_callbacks_t cbs = {0};
  cbs.cb_args = this; // Pass 'this' class instance so callback knows who we are
  cbs.on_ping_success = PingCounter::on_ping_success;
  cbs.on_ping_timeout = PingCounter::on_ping_timeout;
  cbs.on_ping_end = PingCounter::on_ping_end;

  // 7. Create Session
  esp_ping_handle_t ping;
  esp_err_t err = esp_ping_new_session(&ping_config, &cbs, &ping);
  
  if (err != ESP_OK) {
    ESP_LOGE("ping_counter", "Failed to create ping session");
    return;
  }

  // 8. Start Ping
  this->is_running_ = true;
  err = esp_ping_start(ping);
  
  // Safety: If start fails, on_ping_end won't fire, so we must delete manually.
  if (err != ESP_OK) {
     ESP_LOGE("ping_counter", "Failed to start ping");
     esp_ping_delete_session(ping);
     this->is_running_ = false;
  }
}

void PingCounter::_process_ping_result(bool success) {
  if (success) {
    if (this->consecutive_failures_ > 0) {
      ESP_LOGD("ping_counter", "Recovered: %s", this->target_ip_.c_str());
    }
    this->consecutive_failures_ = 0;
  } else {
    this->consecutive_failures_++;
    ESP_LOGW("ping_counter", "Drop: %s (%d/%d)", 
             this->target_ip_.c_str(), this->consecutive_failures_, this->threshold_);

    if (this->consecutive_failures_ >= this->threshold_) {
      ESP_LOGE("ping_counter", "🚨 FAIL: %s", this->target_ip_.c_str());
      
      if (this->alert_sensor_ != nullptr) {
        this->alert_sensor_->publish_state(true);
      }
      // Reset immediately to prepare for pulse
      this->consecutive_failures_ = 0;
      this->pulse_reset_pending_ = true;
    }
  }
}

}  // namespace ping_counter
}  // namespace esphome
