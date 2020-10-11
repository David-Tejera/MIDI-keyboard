//#include <MIDI.h>

//MIDI_CREATE_DEFAULT_INSTANCE();
//Uncomment above when using MIDI-ouput
#include <SPI.h>

//determining the 3 pins from the Arduino to the shift register
#define dataPin 11      //Shift register port 14
#define clockPin 13     //11
#define latchPin 9      //12

#define S_0 3           //determing the pins from the multiplexer
#define S_1 4
#define S_2 5
#define S_3 6
#define SIG 7           //determinition of the signal pin on the arduino to the multiplexer

const int rowNum = 8;
const int numPCB = 8;   //number of how many ports/PCB from the shift register gets shifted

struct Button {
    bool isPressed = false;
};
//
struct Key {
    Button *startButton = new Button();  //startButton is a pointer to an bool
    Button *endButton = new Button();    //endButton is a pointer to an bool
    bool startTimeMeasured = false;
    bool endTimeMeasured = false;
    long time;
    int note;
};
//
struct PCB {
    int length = rowNum;
    Key *keys = new Key[length]; 
};

PCB *pcb = new PCB[numPCB];

void setup() {
    // put your setup code here, to run once:

    // putting all pins from the shift register as output for the keyboard matrix
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);

    pinMode(S_0, OUTPUT);
    pinMode(S_1, OUTPUT);
    pinMode(S_2, OUTPUT);
    pinMode(S_3, OUTPUT);
    // putting the signal pin from the multiplexer as input for the matrix
    pinMode(SIG, INPUT);

    //setting the setup from the SPI.library for the shift register to work
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.begin();

    //Setting up the note (MIDI-numbers)
    for (int i = 0; i < numPCB; i++) {
        for (int j = 0; j < rowNum; j++) {
            pcb[i].keys[j].note = i * rowNum + j;
        }
    }
    //putting the right baut rate for MIDI
    Serial.begin(9600);//Only if using Hairless MIDI
    //MIDI.begin(MIDI_CHANNEL_OMNI);
}

boolean defineMultiPlexer(int j) {
    //defining the Multiplexer rows
    digitalWrite(S_0, bitRead(j, 0));
    digitalWrite(S_1, bitRead(j, 1));
    digitalWrite(S_2, bitRead(j, 2));
    digitalWrite(S_3, bitRead(j, 3));
    return digitalRead(SIG) != 0;
}

//setting up global variable to decrease the amount of storage in the Arduino
long dt;
byte velocity;

//first function to read the inputs on the multiplexer and send the MIDI-messages
void play(PCB &pcb) {
    //Goes through the given PCB
    for (int j = 0; j < 8; j++) {
        Key &key = pcb.keys[j];

        //part of the start button
        key.startButton->isPressed = defineMultiPlexer(2 * j + 1);
        if (key.startButton->isPressed) {
            //Needed for one time measure. Otherwise it would remeasure every time when the start button is pressed
            if (!key.startTimeMeasured) {
                key.time = millis();
                key.startTimeMeasured = true;
            }
        }
        //part of the last button
        key.endButton->isPressed = defineMultiPlexer(2 * j);
        if (key.endButton->isPressed) {
            //Needed for one time measure. Otherwise it would remeasure every time when the end button is pressed
            if (!key.endTimeMeasured) {
                dt = millis() - key.time;
                velocity = pow(M_E, (-dt / 50)) * (97) + 30;
                key.endTimeMeasured = true;

                Serial.write(144);
                Serial.write(36 + key.note);
                Serial.write(velocity);
                //MIDI.sendNoteOn(36+note, velocity, 1);
            }
            //Turn off the key if both buttons are not pressed and time was measured (means that the key is/was playing)
        } else if (key.endTimeMeasured && !key.endButton->isPressed && !key.startButton->isPressed) {
            Serial.write(128);
            Serial.write(36 + key.note);
            Serial.write(0);
            //MIDI.sendNoteOff(36+key.note, velocity, 1);
            key.endTimeMeasured = key.startTimeMeasured = false;
        }
    }
}

//second function to shift the data in the shift register
void shiftMultiPlexer(int i) {
    SPI.transfer(1 << i);           //shifts one byte from port zero to the next one
    digitalWrite(latchPin, HIGH);
    digitalWrite(latchPin, LOW);
}

void loop() {
    // put your main code here, to run repeatedly:
    for (int i = 0; i < numPCB; i++) {
        shiftMultiPlexer(i);
        play(pcb[i]);
    }
}
