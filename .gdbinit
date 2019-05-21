source ~/tools/bin/micro-trace-buffer.py
target extended-remote :2331
load
b HardFault_Handler
b UsageFault_Handler
b os_error
monitor reset
continue
