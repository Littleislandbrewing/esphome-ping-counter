# 1. Import the component from your GitHub
external_components:
  - source: github://littleislandbrewing/esphome-ping-counter
    refresh: 0s   # <--- Forces ESPHome to re-download the new folder structure
    components: [ ping_counter ]

# 2. Configure your monitors
ping_counter:
  # Example 1: Critical Local Device (e.g., Router)
  # Pings every 2s. Alerts after 5 missed packets (10 seconds downtime)
  - id: monitor_router
    ip_address: "192.168.1.1" 
    update_interval: 2s
    threshold: 5
    alert_binary_sensor:
      name: "Router Link Status"

  # Example 2: Internet Check (Google DNS)
  # Pings every 5s. Alerts after 10 missed packets (50 seconds downtime)
  - id: monitor_internet
    ip_address: "8.8.8.8"
    update_interval: 5s
    threshold: 10
    alert_binary_sensor:
      name: "Internet Drop Alert" 
