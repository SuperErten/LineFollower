#include "Cdrv8833.h"

constexpr uint8_t PIN_AIN1          = 19;
constexpr uint8_t PIN_AIN2          = 18;
constexpr uint8_t CHANNEL_MOTOR_A   = 0;
constexpr bool SWAP_MOTOR_A         = 0;

constexpr uint8_t PIN_BIN1          = 16;
constexpr uint8_t PIN_BIN2          = 17;
constexpr uint8_t CHANNEL_MOTOR_B   = 1;
constexpr bool SWAP_MOTOR_B         = 0;

Cdrv8833 motorA(PIN_AIN1, PIN_AIN2, CHANNEL_MOTOR_A, SWAP_MOTOR_A);
Cdrv8833 motorB(PIN_BIN1, PIN_BIN2, CHANNEL_MOTOR_B, SWAP_MOTOR_B);

void setup()
{
    Serial.begin(115200);
    Serial.println("\n");
    Serial.println("DRV8833 tester");
    Serial.println("--------------");
    Serial.printf("IN1 pin: %u\nIN2 pin: %u\n\n", IN1_PIN, IN2_PIN);

    Serial.println("SWAP    - swap motor rotation direction.");
    Serial.println("NOSWAP  - restore motor rotation direction.");
    Serial.println("SLOW    - decay mode SLOW - good torque, high power consumption.");
    Serial.println("FAST    - decay mode FAST - poor torque, low power consumption.");
    Serial.println("MOVEXXX - start rotation (XXX = -100..100).");
    Serial.println("STOP    - stop the motor.");
}

void loop()
{
    String command;
    if (Serial.available()) { // check Serial for new command
        command = Serial.readString(); // read the new command from Serial
        command.toLowerCase(); // convert it to lowercase

        if (command.equals("swap")) {
            motorA.swapDirection(true); // swap rotation direction
            Serial.println("--> swapped rotation direction.");
        }
        else if (command.equals("noswap")) {
            motorA.swapDirection(false); // default rotation direction
            Serial.println("--> default rotation direction.");
        }
        else if (command.equals("slow")) {
            motorA.setDecayMode(drv8833DecaySlow); // decay mode SLOW
            Serial.println("--> Decay mode SLOW - good torque.");
        }
        else if (command.equals("fast")) {
            motorA.setDecayMode(drv8833DecayFast); // decay mode FAST
            Serial.println("--> Decay mode FAST - poor torque.");
        }
        else if (command.equals("stop")) {
            motorA.stop(); // stop moto rotation
            Serial.println("--> Motor stopped.");
        }
        else if (command.startsWith("move")) {
            command.replace("move", ""); // remove the word "move"
            command.replace(" ", "");    // remove spaces (if present)
            motorA.move(command.toInt()); // start rotation at desired speed
            Serial.printf("--> Motor rotation speed: %ld.\n", command.toInt());
        }
    }
}