ADC_OBJECTS-$(BT_CONFIG_DRIVERS_ADC_SPI_MAX1231) += $(BUILD_DIR)/drivers/adc/max1231.o
ADC_OBJECTS-$(BT_CONFIG_DRIVERS_ADC_I2C_MAX1363) += $(BUILD_DIR)/drivers/adc/max1363.o


ADC_OBJECTS += $(ADC_OBJECTS-y)

$(ADC_OBJECTS): MODULE_NAME="drivers-ADC"
OBJECTS += $(ADC_OBJECTS)
