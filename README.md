# STM32 Reflex Game

## Polytech Grenoble - 4th Year Software Engineering Portfolio

This repository contains a low-level embedded system project featuring a reflex-testing game built for the STM32 microcontroller. Developed entirely in C, the project demonstrates proficiency in hardware-level programming, interrupt management, state machines, and peripheral configuration (USART, PWM, Timers).

## Gameplay Mechanics

The game tests the user's reaction time over three rounds, using both visual cues and physical inputs.

* **Rules:** The user waits for an RGB LED to change from red to green, then immediately presses a button.


* **Inputs:** Players can use either the STM32 motherboard's push-button or the "Enter" key on a connected computer keyboard.


* **Penalties:** Pressing the button before the LED turns green results in a false start, playing a defeat melody and forcing a complete restart of the game.


* **Win Condition:** The user wins if their average reaction time across the three rounds is strictly under 300 ms.


* **Feedback:** The game plays custom victory or defeat melodies upon completion and automatically restarts for the next session.



## Hardware Requirements

* **Microcontroller:** STM32 development board.


* **Peripherals:** A compatible daughter board equipped with an RGB LED and a buzzer.


* **Interface:** A PC running a graphical serial terminal (e.g., GTKTerm) to display the text-based interface and reaction times.



## Technical Architecture

The firmware is designed around an event-driven architecture, relying heavily on non-blocking routines to manage concurrent hardware events.

### Core Systems

* **State Machine Logic:** The game loop operates on a strictly defined set of "Modes" (states) managed via a main `switch` statement. Depending on the active mode, hardware interrupts trigger completely different behaviors (e.g., starting a round vs. validating a reflex).


* **SysTick Timer & Time Management:** The system timer is configured to trigger an interrupt every 1 ms, serving as the global clock. Reaction times are calculated by timestamping the LED state change and the button press, then performing a subtraction.


* **Pulse Width Modulation (PWM):** The daughter board's buzzer is driven using PWM. By programmatically supplying specific real-world musical frequencies and durations (tracked via SysTick), the board plays sustained, accurate musical notes to form complete songs.



### Interrupt Handlers

* **USART (Serial Communication):** Keyboard inputs are captured via USART interrupts. To prevent data transmission from bottlenecking or blocking other critical system interrupts, output is handled via a custom asynchronous function: `void _async_puts(void);`.


* **GPIO (External Interrupts):** The primary physical interaction uses the STM32 motherboard's push-button, configured to trigger hardware interrupts that advance the game's state machine.



## Installation & Deployment

Building and flashing the project requires an ARM cross-compiler and the OpenOCD debugging tool.

**1. Install Dependencies (Debian / Ubuntu):**

```bash
sudo apt install gcc-arm-none-eabi openocd gdb-multiarch

```

**2. Configure USB Permissions:**
To allow the current user to access the board without root privileges, add them to the `plugdev` group (do not run this as root):

```bash
id | grep -sq '(plugdev)' || sudo adduser $(id -un) plugdev

```

Note: If the command executes successfully, you must log out and log back in to apply the group changes. Ensure the STM32 board is plugged in **after** installing `openocd`.

**3. Build and Flash:**
Navigate to the `TP1` directory to compile and load the firmware onto the board:

```bash
make
make load

```

**4. Debugging:**
To debug the STM32 directly from your machine using `gdb-multiarch`, execute the following in the `TP1` directory:

```bash
make gdb

```

