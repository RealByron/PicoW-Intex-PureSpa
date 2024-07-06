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
![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/d7c6fecb-88e9-4a62-b593-ee8090b1208b)

# Electronics
Don't blame me, it's not my professional activity !!!
Advices are welcome.

Hardware is very simple :
- Raspberry Pi Pico W
- Level shifters 
- 10 µF capcitor

![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/ca510e43-296f-4e0a-9e2c-d352a6a11b31)

![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/89278776-472c-42b0-af2c-f625bd6158a3)

If you know how to connect level shifters, you just need to connect :
- DATA (3.3V) on GP3
- CLK (3.3V) on GP4
- HOLD (3.3V) on GP5

![245259164-c63af9a9-f848-4848-b8d3-af1a601fe964](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/d758ca06-3f4a-4b0d-b52f-cb86b6ce48c0)
![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/086b608e-5e62-42fe-aa9c-cd78b8a11c70)

Credits : https://github.com/YorffoeG/diyscip/blob/master/docs/connector%20pinout.png

# PIO program
![image](https://github.com/RealByron/PicoW-Intex-PureSpa/assets/1749192/0cf17844-4127-45b0-a2fc-9e69e141ccc2)

Mostly GPIO just read data bit on clock rising edge.
When ISR is full of 16 bits, then store that value on Y register.
ISR is also pushed to FIFO and an IRQ is fired.
Then C program have time to proces all these words in FIFO and decodes leds and digits.

For push button, a function put expected push button code in FIFO and the PIO program loads it in X register.

When X and Y are equal, a pulse is done on DATA line (4µs after CS rising edge and during 2 µs)

# Installation 

I'm using WSL Ubuntu on windows 11.
Go to your favourite dev directory then `git clone` this repository

```
git clone https://github.com/RealByron/PicoW-Intex-PureSpa.git PicowSpa
cd PicowSpa
```
enable virtual env and activate it
```
python -m venv spa-env
source spa-env/bin/activate
```
install esphome
```
pip install wheel
pip install esphome
```
and execute it
```
esphome dashboard .
```
Then you're at (esp)home

# Contributing
Contributions to this project are welcome. If you find any issues or have suggestions for improvements, please open an issue or submit a pull request.

# License
This project is licensed under the MIT License.

# Disclaimer
Using this project to control your Intex PureSpa may void your warranty. Please consult the documentation and terms of service provided by Intex before using this project.

