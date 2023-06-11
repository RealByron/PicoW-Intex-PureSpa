# PicoW-Intex-PureSpa
Intex PureSpa SBH20 control interface for esphome.

Idea came from https://github.com/YorffoeG/diyscip and https://github.com/jnsbyr/esp8266-intexsbh20 for their great comprehension of PureSpa protocol.
Since many reports problems with controlling target temperature due to heavy processing in interrupts, I decide to give raspberry pi pico W a try.

That's my very project first with :
- Raspberry Pi Pico W
- esphome
So please be indulgent, I've learnt a lot, but it's just a beginning.

# Aspirin tube case !!
![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/8ec6f361-ab9b-407b-b083-84245200abdd)

# Fully discoverable in Home assistant
![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/d4230ada-e759-44a0-8b38-833575798f3a)

# Electronics
Don't blame me, it's not my professional activity !!!
Advices are welcome.

Hardware is very simple :
- Raspberry Pi Pico W
- Level shifters 
- 10 µF capcitor

![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/ca510e43-296f-4e0a-9e2c-d352a6a11b31)

![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/89278776-472c-42b0-af2c-f625bd6158a3)

Schematics to come... If you know how to connect level shifters, you just need to connect :
- DATA (3.3V) on GP3
- CLK (3.3V) on GP4
- CS (3.3V) on GP5

(see BASE_PIN in components/purespa/purespa_ctrl.cpp)

# PIO program
![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/0cf17844-4127-45b0-a2fc-9e69e141ccc2)

Mostly GPIO just read data bit on clock rising edge.
When ISR is full of 16 bits, then store that value on Y register.
ISR is also push to FIFO and an IRQ is fired.
Then C program have time to proces all these words in FIFO and decodes leds and digits.

For push button, a function put expect push button code in FIFO and the PIO program loads it in X register.

When X and Y are equal, a pulse is done on DATA line (4µs after CS rising edge and during 2 µs)

# Contributing
Contributions to this project are welcome. If you find any issues or have suggestions for improvements, please open an issue or submit a pull request.

# License
This project is licensed under the MIT License.

# Disclaimer
Using this project to control your Intex PureSpa may void your warranty. Please consult the documentation and terms of service provided by Intex before using this project.

