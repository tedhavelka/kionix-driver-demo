// - DEV 1127 -
//
// 503 /* Create the struct device for every status "okay"*/
// 504 DT_INST_FOREACH_STATUS_OKAY(KX132_DEFINE)

#define DT_DRV_COMPAT kionix_kx132_1211

#define KX132_DRDY_INTERRUPT_DEFINE(inst) \
static const struct gpio_dt_spec int_gpio_diag1 = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 });
//const struct gpio_dt_spec int_gpio_##inst = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 });

DT_INST_FOREACH_STATUS_OKAY(KX132_DRDY_INTERRUPT_DEFINE)

