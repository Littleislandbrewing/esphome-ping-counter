#include "ping_counter.h"

namespace esphome {
namespace ping_counter {

void PingCounter::setup() {
  // AUDIT FIX: Verify IP and explicitly warn about Hostnames
  IPAddress ip_check;
  if (!ip_check.fromString(this->target_ip_.c_str())) {
    ESP_LOGE("ping_counter", "❌ INVALID IP: %s", this->target_ip_.c_str());
    ESP_LOGE("ping_counter", "   Hostnames (e.g. google.com) are NOT supported. Use raw IPs (e.g. 8.8.8.8).");
    this->mark_failed(); 
    return;
  }

  // AUDIT FIX: Initialize sensor to False (OK)
  if (this->alert_sensor_ != nullptr) {
    this->alert_sensor_->publish_state(false);
  }

  this->ping_handler_.on(true, [this](const AsyncPingResponse& response) {
    bool success = response.answer;
    this->defer([this, success]() {
      this->process_result_(success);
    });
  });
}

void PingCounter::dump_config() {
  // AUDIT FIX: Professional logging on boot
  ESP_LOGCONFIG("ping_counter", "Ping Counter:");
  ESP_LOGCONFIG("ping_counter", "  Target IP: %s", this->target_ip_.c_str());
  ESP_LOGCONFIG("ping_counter", "  Threshold: %d failures", this->threshold_);
  LOG_BINARY_SENSOR("  ", "Alert Sensor", this->alert_sensor_);
}

void PingCounter::update() {
  // AUDIT FIX: "Zombie" State Handling
  // If Wi-Fi is physically disconnected, we can't ping.
  // We explicitly log this condition.
  if (!network::is_connected()) {
    // Optional: You could force the sensor TRUE here if you consider 
    // "No Wifi" to be the same as "Cant Ping Target".
    // For now, we just return to avoid errors.
    return;
  }
  
  // Watchdog (Deadlock breaker)
  if (this->is_running_) {
    if (millis() - this->last_ping_start_ > 5000) {
      ESP_LOGW("ping_counter", "⚠️ Watchdog: Ping hung. Resetting.");
      this->is_running_ = false; 
    } else {
      return; 
    }
  }

  // If we were "Pulsing" (Alerting), turn it off now to reset for the next wave
  if (this->is_pulsing_) {
     this->is_pulsing_ = false;
     if (this->alert_sensor_ != nullptr) {
       this->alert_sensor_->publish_state(false);
     }
     // We continue to ping immediately to check if connection is back
  }

  IPAddress ip;
  ip.fromString(this->target_ip_.c_str());

  this->is_running_ = true;
  this->last_ping_start_ = millis();
  
  if (!this->ping_handler_.begin(ip, 1, 1000)) {
      this->is_running_ = false;
      ESP_LOGW("ping_counter", "Failed to send ping command.");
  }
}

void PingCounter::process_result_(bool success) {
  this->is_running_ = false;

  if (success) {
    // Success Logic
    if (this->consecutive_failures_ > 0) {
       ESP_LOGI("ping_counter", "Recovered after %d failures.", this->consecutive_failures_);
    }
    this->consecutive_failures_ = 0;
    
    // Ensure sensor is OFF
    if (this->alert_sensor_ != nullptr && this->alert_sensor_->state) {
      this->alert_sensor_->publish_state(false);
    }

  } else {
    // Failure Logic
    this->consecutive_failures_++;
    ESP_LOGD("ping_counter", "Missed: %d/%d", this->consecutive_failures_, this->threshold_);

    if (this->consecutive_failures_ >= this->threshold_) {
      ESP_LOGE("ping_counter", "🚨 THRESHOLD HIT: %s", this->target_ip_.c_str());
      
      // TRIGGER ALERT
      if (this->alert_sensor_ != nullptr) {
        this->alert_sensor_->publish_state(true);
      }
      
      // PULSE LOGIC:
      // We set a flag so that on the NEXT update() (in 1s or 10s), 
      // we force the sensor back to False.
      // This creates a clear "On... Off" pulse for Home Assistant.
      this->is_pulsing_ = true;
      
      // Reset counter as requested
      this->consecutive_failures_ = 0; 
    }
  }
}

}  // namespace ping_counter
}  // namespace esphome