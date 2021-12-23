import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display
from esphome.const import CONF_ID, CONF_LAMBDA, CONF_NUM_CHIPS

CODEOWNERS = ["@ruudvd"]
#DEPENDENCIES = [""]

CONF_SCROLL_SPEED = "scroll_speed"
CONF_SCROLL_DWELL = "scroll_dwell"
CONF_SCROLL_DELAY = "scroll_delay"
CONF_SCROLL_ENABLE = "scroll_enable"
CONF_SCROLL_MODE = "scroll_mode"
CONF_REVERSE_ENABLE = "reverse_enable"
CONF_NUM_CHIP_LINES = "num_chip_lines"

integration_ns = cg.esphome_ns.namespace("LedDisplay")

ScrollMode = integration_ns.enum("ScrollMode")
SCROLL_MODES = {
    "CONTINUOUS": ScrollMode.CONTINUOUS,
    "STOP": ScrollMode.STOP,
}

LedDisplay_ns = cg.esphome_ns.namespace("LedDisplay")
LedDisplayComponent = LedDisplay_ns.class_(
    "LedDisplayComponent", cg.PollingComponent, display.DisplayBuffer
)
LedDisplayComponentRef = LedDisplayComponent.operator("ref")

CONFIG_SCHEMA = (
    display.BASIC_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(LedDisplayComponent),
            cv.Optional(CONF_NUM_CHIPS, default=10): cv.int_range(min=1, max=20),
            cv.Optional(CONF_NUM_CHIP_LINES, default=7): cv.int_range(min=1, max=20),
            cv.Optional(CONF_SCROLL_MODE, default="CONTINUOUS"): cv.enum(
                SCROLL_MODES, upper=True
            ),
            cv.Optional(CONF_SCROLL_ENABLE, default=True): cv.boolean,
            cv.Optional(
                CONF_SCROLL_SPEED, default="250ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_SCROLL_DELAY, default="1000ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_SCROLL_DWELL, default="1000ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_REVERSE_ENABLE, default=False): cv.boolean,
        }
    )
    .extend(cv.polling_component_schema("500ms"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await display.register_display(var, config)

    cg.add(var.set_num_chips(config[CONF_NUM_CHIPS]))
    cg.add(var.set_num_chip_lines(config[CONF_NUM_CHIP_LINES]))
    cg.add(var.set_scroll_speed(config[CONF_SCROLL_SPEED]))
    cg.add(var.set_scroll_dwell(config[CONF_SCROLL_DWELL]))
    cg.add(var.set_scroll_delay(config[CONF_SCROLL_DELAY]))
    cg.add(var.set_scroll(config[CONF_SCROLL_ENABLE]))
    cg.add(var.set_scroll_mode(config[CONF_SCROLL_MODE]))
    cg.add(var.set_reverse(config[CONF_REVERSE_ENABLE]))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(LedDisplayComponentRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
