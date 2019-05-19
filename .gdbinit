target extended-remote :2331
load
b HardFault_Handler
monitor reset
continue
