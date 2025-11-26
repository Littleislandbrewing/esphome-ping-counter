import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_IP_ADDRESS

ping_counter_ns = cg.esphome_ns.namespace('ping_counter')
PingCounter = ping_counter_ns.class_('PingCounter', cg.PollingComponent)

CONF_ALERT_SENSOR = "alert_binary_sensor"
CONF_THRESHOLD = "threshold"

# 1. Define the schema for a SINGLE item
PING_COUNTER_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(PingCounter),
    cv.Required(CONF_IP_ADDRESS): cv.string,
    cv.Optional(CONF_THRESHOLD, default=10): cv.int_range(min=1, max=100),
    cv.Optional(CONF_ALERT_SENSOR): binary_sensor.binary_sensor_schema(),
}).extend(cv.polling_component_schema("10s"))

# 2. Allow a LIST of items
CONFIG_SCHEMA = cv.All(cv.ensure_list(PING_COUNTER_SCHEMA))

async def to_code(config):
    # FIX: Use full Git URL to prevent "UnknownPackageError"
    cg.add_library("AsyncPing", "1.1.0", "https://github.com/marian-craciunescu/AsyncPing.git")

    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        
        cg.add(var.set_target_ip(conf[CONF_IP_ADDRESS]))
        cg.add(var.set_threshold(conf[CONF_THRESHOLD]))

        if CONF_ALERT_SENSOR in conf:
            sens = await binary_sensor.new_binary_sensor(conf[CONF_ALERT_SENSOR])
            cg.add(var.set_alert_sensor(sens))
