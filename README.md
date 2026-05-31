# Cyclive - Cyclic Executive Scheduler

A bare-metal implementation of a cyclic executive scheduler for the LPC2300 microcontroller, demonstrating real-time task scheduling with different task deadlines using timer-based interrupts.

## Project Overview

This project implements a **cyclic executive** scheduling pattern on the ARM LPC23xx microcontroller. A cyclic executive is a deterministic scheduling approach where tasks are executed at predetermined intervals within a fixed cycle, making it suitable for hard real-time systems.

**Target Hardware:** LPC2300 ARM Cortex-M3 Microcontroller

## Architecture

The system manages **4 concurrent ADC (Analog-to-Digital Converter) sampling tasks** with different execution periods:

| Task | Period | Frequency |
|------|--------|-----------|
| ADC0, ADC1, ADC3 | 100 ms | Every 100 milliseconds |
| ADC2 | 200 ms | Every 200 milliseconds |
| Result Calculation & Display | 1000 ms | Every 1 second |

## How It Works

### Cyclic Scheduling Model

The scheduler uses a **1 ms timer tick** as the base unit. All tasks are multiples of this tick:

```
Time (ms):  0    100    200    300    400    500    600    700    800    900    1000
          |-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
Task A:   |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|      (100ms)
Task B:   |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|      (100ms)
Task C:   |     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx|     |xxxxx| (200ms)
Calc:     |                                                                   |xxxxx| (1000ms)
```

### Core Components

#### 1. **Timer Interrupt Handler** (`init_timer()`)
- Initializes Timer0 to generate a 1 ms interrupt
- `T0PR = 11` sets prescaler; `T0MR0 = 999` sets match register for 1 ms tick
- Interrupt fires every millisecond, incrementing counters for task scheduling

#### 2. **ADC Driver** (`init_adc()` & `read_adc()`)
- Enables ADC module and configures AD0.0, AD0.1, AD0.2, AD0.3 pins
- `read_adc(channel, control_bits)` performs conversion and stores 10-bit result
- Accumulates readings in `sum_total_adc[]` array over each cycle

#### 3. **Main Cyclic Executive Loop** (in `main()`)
```
while (1) {
    if (Timer0_Interrupt_Flag) {
        Clear_Flag();
        
        // Every 100 ms: Sample 3 channels
        if (count_100ms % 100 == 0) {
            read_adc(1, 0x00200002);  // AD0.1
            read_adc(0, 0x00200001);  // AD0.0
            read_adc(3, 0x00200008);  // AD0.3
        }
        
        // Every 200 ms: Sample 1 channel
        if (count_200ms % 200 == 0) {
            read_adc(2, 0x00200004);  // AD0.2
        }
        
        // Every 1000 ms: Calculate & display results
        if (count_1s % 1000 == 0) {
            Calculate averages;
            Display on LCD;
            Reset accumulators;
        }
    }
}
```

#### 4. **LCD Display** (`LCD_4bit.c`)
- 4-bit parallel interface control for 16x2 character LCD
- Displays real-time ADC voltage readings and averages
- Converts raw ADC counts to voltage (scaled by 3.2V reference / 1023 counts)

#### 5. **Serial Communication** (`Serial.c`)
- UART interface for debugging (9600 baud, 8N1)
- Supports printf-style output redirection

### Data Processing Pipeline

```
Raw ADC Reading (10-bit) 
    → Accumulate in sum_total_adc[channel]
    → (Every 1000ms) Calculate: Average = Total_Sum / 35
    → Convert to Voltage: Voltage = (ADC_Count / 10.0) * (3.2 / 1023)
    → Display on LCD and reset accumulators
```

## Task Scheduling Details

The scheduler is **rate-monotonic** with the following characteristics:

- **Deterministic execution**: Tasks execute at fixed, predictable times
- **No preemption**: Once a task starts, it runs to completion before the next check
- **Fixed memory**: No dynamic allocation; all buffers pre-allocated
- **Jitter-free**: Tasks wake up on precise timer ticks, not through context switching

### Schedule Verification

With 1 ms tick and 1 second major cycle:
- Task A (100 ms period): Runs 10 times per second → 100 reads accumulated per channel
- Task B (200 ms period): Runs 5 times per second → 50 reads accumulated
- Task C (1 s period): Runs 1 time → calculates and displays
- Total CPU utilization is predictable and analyzable

## Technical Specifications

### Hardware Requirements
- **MCU**: LPC2300 ARM Cortex-M3 (12 MHz PCLK typical)
- **Memory**: ~110 KB (flash + configuration)
- **Peripherals**: Timer0, ADC module, UART1, GPIO

### Software
- **Language**: C with ARM Assembly (startup code in `LPC2300.s`)
- **Compiler**: Keil μVision ARM C/C++ compiler
- **Development Environment**: Keil MDK (LPC2300 target)

### ADC Configuration
- **Reference Voltage**: 3.2V
- **Resolution**: 10-bit (0-1023)
- **Channels Used**: AD0.0, AD0.1, AD0.2, AD0.3
- **Conversion Control**: Bits set for channel selection (e.g., `0x00200001` for AD0.0)

## File Structure

```
Cyclive/
├── CyclicExecutive_ADC.c       # Main scheduler and ADC task handler
├── LCD_4bit.c                  # LCD display driver (4-bit interface)
├── LCD.h                        # LCD function prototypes
├── IRQ.c                        # Interrupt handlers (timer, ADC)
├── Serial.c                     # UART serial communication
├── LPC2300.s                    # ARM startup/initialization assembly
├── Retarget.c                   # I/O redirection (printf support)
└── ADC.*                        # Build artifacts (Keil project files)
```

## Build & Deployment

1. Open project in Keil MDK: `ADC.Uv2`
2. Compile: Builds to `ADC.hex` (Intel HEX format)
3. Flash: Upload `ADC.hex` to LPC2300 via JTAG/SWD debugger
4. Execute: Microcontroller runs cyclic executive immediately on power-up

## Key Advantages of Cyclic Executive

✓ **Predictability**: No context switching overhead; execution times are deterministic
✓ **Simple**: Easy to analyze for hard real-time requirements
✓ **Efficient**: Minimal runtime overhead; suitable for resource-constrained systems
✓ **Debugging**: Task execution order is static and repeatable

## Limitations & Trade-offs

- **Less flexible**: Adding new tasks requires modifying the main schedule
- **Fixed period**: Task periods must divide evenly into the major cycle
- **No priority inversion protection**: All tasks have equal priority by design
- **No dynamic task management**: Tasks cannot be added/removed at runtime

## Design Rationale

This implementation demonstrates best practices for bare-metal real-time programming:
- **Timer-based scheduling**: Avoids OS overhead; achieves cycle-accurate execution
- **Accumulation pattern**: Buffers readings over the cycle, reducing LCD update flicker
- **Voltage scaling**: Post-processes raw ADC values for meaningful readings
- **Modular drivers**: LCD, Serial, and ADC functions are independently reusable

## References

- **LPC2300 User Manual**: ARM Cortex-M3 register definitions and peripheral specs
- **Cyclic Executive Pattern**: Used in aerospace and automotive real-time systems (e.g., AUTOSAR)
- **Rate-Monotonic Scheduling**: Theoretical foundation for deadline analysis

---

**Author**: InvincibleJuggernaut  
**License**: See individual source files for copyright notices  
**Last Updated**: December 2024
