import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_IP_ADDRESS

ping_counter_ns = cg.esphome_ns.namespace('ping_counter')
PingCounter = ping_counter_ns.class_('PingCounter', cg.PollingComponent)

CONF_ALERT_SENSOR = "alert_binary_sensor"
CONF_THRESHOLD = "threshold"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PingCounter),
    cv.Required(CONF_IP_ADDRESS): cv.string,
    cv.Optional(CONF_THRESHOLD, default=10): cv.int_range(min=1, max=100),
    cv.Optional(CONF_ALERT_SENSOR): binary_sensor.binary_sensor_schema(),
}).extend(cv.polling_component_schema("10s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    cg.add(var.set_target_ip(config[CONF_IP_ADDRESS]))
    cg.add(var.set_threshold(config[CONF_THRESHOLD]))

    if CONF_ALERT_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_ALERT_SENSOR])
        cg.add(var.set_alert_sensor(sens))

    # We use the AsyncPing library for BOTH ESP32 and ESP8266
    cg.add_library("marian-craciunescu/AsyncPing", "1.1.0")
