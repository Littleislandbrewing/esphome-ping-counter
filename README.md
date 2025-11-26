# ESPHome Non-Blocking Ping Counter

A high-performance, **non-blocking** network monitor component for ESPHome.

Unlike the standard ESPHome ping sensor, this component uses asynchronous ICMP packets (`AsyncPing`). This means it can check network status without pausing the main loop or freezing your device. This is critical for PLCs, Brewery Controllers, LED strips, or any device handling real-time sensors (like flow meters or rotary encoders) where a 1-second freeze could ruin data.

## Features

* **Non-Blocking:** Zero "lag" or freezing, even when pings time out. Safe for real-time control applications.
* **Consecutive Counting:** Intelligently counts consecutive failures. One successful packet instantly resets the counter to 0.
* **Pulse Alerting:** When the failure threshold is hit, the sensor triggers `ON` for a single cycle and then automatically resets. This creates a clean "event" for Home Assistant automations.
* **Safety Logic:** Includes a watchdog timer to prevent deadlocks and automatically pauses checking if the ESP itself loses Wi-Fi.

## Installation

Add the following to your ESPHome YAML configuration file:

```yaml
external_components:
  - source: github://littleislandbrewing/esphome-ping-counter
    components: [ ping_counter ]
```

## Configuration Examples

### Basic Usage (Internet Check)

```yaml
ping_counter:
  - id: monitor_internet
    ip_address: "8.8.8.8"
    update_interval: 60s
    threshold: 5
    alert_binary_sensor:
      name: "Internet Connection Lost"
```

### Advanced Usage (Critical Device Monitoring)

```yaml
ping_counter:
  # Check Router every 1 second (High Priority)
  - id: monitor_router
    ip_address: "192.168.1.1"   # Must be raw IP, no hostnames
    update_interval: 1s
    threshold: 5
    alert_binary_sensor:
      name: "Router Failure Alert"

  # Check a Server every 10 seconds (Low Priority)
  - id: monitor_server
    ip_address: "192.168.1.50"
    update_interval: 10s
    threshold: 10
    alert_binary_sensor:
      name: "Server Offline Alert"
```

## Configuration Variables

| Variable | Required | Description | Default |
| :--- | :--- | :--- | :--- |
| `ip_address` | **Yes** | The target IP address to ping. **Must be a raw IP** (e.g., `192.168.1.1`). Hostnames are not supported. | - |
| `alert_binary_sensor` | Optional | A binary sensor that pulses `ON` when the threshold is reached. | - |
| `threshold` | No | The number of consecutive missed packets required to trigger the alert. | `10` |
| `update_interval` | No | How often to send a ping request. | `10s` |

## How the Logic Works

This component uses a **Pulse/Reset** logic designed for automation reliability:

1.  **Normal Operation:** The sensor stays `OFF`. The internal counter stays at `0`.
2.  **Packet Loss:** If a ping fails, the counter increments (`1`, `2`, `3`...).
3.  **Threshold Hit:** When the counter hits the `threshold` (e.g., 5):
    * The `alert_binary_sensor` turns **ON**.
    * The internal counter resets to `0`.
4.  **Auto-Reset:** On the very next update cycle, the sensor automatically turns **OFF**.

This ensures that if a connection is down for an hour, you get a distinct "Pulse" event every time the threshold is hit.

## Troubleshooting

* **"Invalid IP Address" Error:** You likely used a hostname (like `homeassistant.local`). This component **requires** numeric IP addresses.
* **Wi-Fi Dependency:** If the ESP's own Wi-Fi disconnects, the component will stop checking (to avoid generating false "missed packet" counts). It will resume automatically when Wi-Fi reconnects.
