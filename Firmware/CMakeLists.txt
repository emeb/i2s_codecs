cmake_minimum_required(VERSION 3.25)

# Pull in SDK (must be before project)
include(/opt/pico/pico-sdk/external/pico_sdk_import.cmake)

project(rp2040_i2s_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Needed to keep multicore from choking the XIP cache
set(PICO_COPY_TO_RAM 1)

# Initialize the SDK
pico_sdk_init()

add_executable(rp2040_i2s_test
	main.c
	i2s_fulldup.c
	wm8731.c
	aic3101.c
	nau88c22.c
	sgtl5000.c
	audio.c
	led.c
	button.c
	debounce.c
)

# Move the logging UART to different pins
target_compile_definitions(rp2040_i2s_test PRIVATE
    PICO_DEFAULT_UART_TX_PIN=28
    PICO_DEFAULT_UART_RX_PIN=29
# Adjust system clock setup for 159.75MHz which gives nearly 48kHz sample rate
	PLL_SYS_REFDIV=2
	PLL_SYS_VCO_FREQ_HZ=1278000000
	PLL_SYS_POSTDIV1=4
	PLL_SYS_POSTDIV2=2
	SYS_CLK_HZ=159750000
)

pico_enable_stdio_uart(rp2040_i2s_test 1)
#pico_enable_stdio_usb(rp2040_i2s_test 1)

pico_generate_pio_header(rp2040_i2s_test ${CMAKE_CURRENT_LIST_DIR}/i2s_fulldup.pio)

target_link_libraries(rp2040_i2s_test
	pico_stdlib
	hardware_pio
	hardware_dma
	hardware_i2c
	cmsis_core
	pico_multicore
	pico_unique_id
)

pico_add_extra_outputs(rp2040_i2s_test)
