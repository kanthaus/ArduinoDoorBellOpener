# ArduinoDoorBellOpener

This project implements an automatic door opener using a door bell keypad, an electric door opener as well as an Arduino (A digispark clone in this case).

It features

* Feedback light (Here: Doorbell background light exchanged with LED)
* Triggering a door opener (Here: One that works with a low DC voltage generated with a step down module)
  * by entering a single code on a 4-digit keypad (Here: 4 Doorbell switches that have been in place already)

As this project aims to be as simple but still effective as possible, the combination is fixed in code.

Theoretic future work and play could implement nice algorithms or "games" requesting the user to react with the buttons on changes of the light to open the door.

## Circuit

As we had some Digispark modules to play with, this is the one getting used here. Although, any Arduino with at least 5 (or 6) IOs would be fine.

### Door opener

Door openers have different requirements of feeding voltage.
At best, try with available power supplies what works with yours.
Typically, door bell systems run with 8-16V AC voltage.
Still, most should work fine with DC but the current draw might rise when fed with DC (as they have a higher AC resistance due to the inductive coil allowing to open the door).

Mine works beginning from approximately 2.5V DC and draws about 350mA.
That translates to about 1W of power, which seems alright.
At 12V DC, the current draw is nearly 1.5A, meaning the small door opener would generate heat of almost 20W which would break it within a couple of seconds.

I decided to use a small step down module with an enable pin to power the door opener:

* Independence of input voltage
* buck converters usually should be able to handle inductive loads without further consideration in the circuit
* Enable pin can be driven by Arduino directly
* Decouples the high current door opener from the sensitive Arduino as both use independent voltage regulators
* Using a higher voltage supply and the dcdc, the door opener actually does only use very little currents (12V: ~80 mA)

The door opener gets directly connected to the step down module (GND and Vout).
Vin gets connected to the used power supply.
The enable pin gets connected to an IO pin of the arduino.

#### A word on the enable pin of DC DC converters

My module has an active high enable pin.
The used converter IC (labeled AGCH - I don't know what it actually is) pulls it to Vin internally, so this pin might get pulled up to 12V or whatever we use as a supply which would break your Arduino IF the current would not be very low.
The Arduino (as most other digital circuits) have input clamping diodes pulling overvoltages to the supply.
This works safely, as long as the current is way below 1mA.

Also, most active high enable pins basically just sense being pulled low - any voltage higher than 1-2V turns the device on, below that it is turned off.

### Power supply

Most Arduino boards have an on board voltage regulator, which gets used here. This allows us to feed the arduino with 5-30 V (look up the specification of yours!).
The selected DC DC converter for the door opener works up to 20V.
So any old 500mA USB charger (that you don't want to use anymore in times of QuickCharge or 2A charging capable phones) would be fine here, but also any other 12V or old notebook adapter (if you don't have a better use for them).


### Input keys

Our door bell system here is old and "analog" - each doorbell is individually wired to a switch.
So the circuit becomes very simple:

* One side of all switches is common and connected to ground
* The other side is individually connected to one IO Pin each
* The IO pins of the Arduino are configured as Inputs with pullup, so a pressed key reads as 0


### Indicator light

Being fully optional, there can be an indicator light next to the keypad.
As we already have a common ground there, we just use another IO pin of the arduino to connect an LED with a resistor of 220 to 470 Ohms in series.

