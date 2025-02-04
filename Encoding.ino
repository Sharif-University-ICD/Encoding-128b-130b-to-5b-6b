const int LED_PINS[] = {2, 3, 4, 5, 6, 7};
const int NUM_LEDS = 6;
const int TX_PIN_1 = 8;
const int RX_PIN_1 = 9;
const int TX_PIN_2 = 10;
const int RX_PIN_2 = 11;

const bool ENCODE_5B6B[32][6] = {
    {1,0,0,1,1,1}, {0,1,1,1,0,1}, {1,0,1,1,0,1}, {1,1,0,0,0,1},
    {1,1,0,1,0,1}, {1,0,1,0,0,1}, {0,1,1,0,0,1}, {1,1,1,0,0,0},
    {1,1,1,0,0,1}, {1,0,0,1,0,1}, {0,1,0,1,0,1}, {1,1,0,1,0,0},
    {0,0,1,1,0,1}, {1,0,1,1,0,0}, {0,1,1,1,0,0}, {0,1,0,1,1,1},
    {0,1,1,0,1,1}, {1,0,0,0,1,1}, {0,1,0,0,1,1}, {1,1,0,0,1,0},
    {0,0,1,0,1,1}, {1,0,1,0,1,0}, {0,1,1,0,1,0}, {1,1,1,0,1,0},
    {1,1,0,0,1,1}, {1,0,0,1,1,0}, {0,1,0,1,1,0}, {1,1,0,1,1,0},
    {0,0,1,1,1,0}, {1,0,1,1,1,0}, {0,1,1,1,1,0}, {1,0,1,0,1,1}
};

bool original_data[130];
bool received_first[130];
bool encoded_data[156];
bool received_second[156];
bool decoded_data[130];
int error_count = 0;

void initialize_random_data() {
    // Set control bits
    original_data[0] = 0;
    original_data[1] = 1;
    
    // Generate random data for remaining bits
    for (int i = 2; i < 130; i++) {
        original_data[i] = random(2);
    }
}

void setup() {
    for (int i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    pinMode(TX_PIN_1, OUTPUT);
    pinMode(RX_PIN_1, INPUT);
    pinMode(TX_PIN_2, OUTPUT);
    pinMode(RX_PIN_2, INPUT);
    Serial.begin(9600);
    randomSeed(analogRead(0));
}

void first_transmission() {
    Serial.println("\nStarting first transmission (130 bits) - TX1 -> RX2...");
    for (int i = 0; i < 130; i++) {
        digitalWrite(TX_PIN_1, original_data[i]);
        delay(10);
        received_first[i] = digitalRead(RX_PIN_2);  
        
        Serial.print("Bit ");
        Serial.print(i);
        Serial.print(": Sent ");
        Serial.print(original_data[i]);
        Serial.print(", Received ");
        Serial.println(received_first[i]);
        delay(40);
    }
}

void encode_received_data() {
    Serial.println("\nEncoding received data into 5B/6B format...");
      if (received_first[0] != 0 || received_first[1] != 1) {
        Serial.println("ERROR: First two bits must be 0 and 1");
        while(1); // Halt program
    }
    
    // Replace first two bits with 0,0
    received_first[0] = 0;
    received_first[1] = 0;
    for (int block = 0; block < 26; block++) {
        int index = 0;
        for (int j = 0; j < 5; j++) {
            index = (index << 1) | received_first[(block * 5) + j];
        }
        
        for (int j = 0; j < 6; j++) {
            encoded_data[(block * 6) + j] = ENCODE_5B6B[index][j];
        }
        
        Serial.print("Block ");
        Serial.print(block);
        Serial.print(" - Original 5b: ");
        for (int j = 0; j < 5; j++) {
            Serial.print(received_first[(block * 5) + j]);
        }
        Serial.print(" -> Encoded 6b: ");
        for (int j = 0; j < 6; j++) {
            Serial.print(encoded_data[(block * 6) + j]);
        }
        Serial.println();
    }
}

void second_transmission() {
    Serial.println("\nStarting second transmission (156 encoded bits) - TX2 -> RX1...");
    for (int i = 0; i < 156; i++) {
        digitalWrite(TX_PIN_2, encoded_data[i]);
        delay(10);
        received_second[i] = digitalRead(RX_PIN_1); 
        
        Serial.print("Bit ");
        Serial.print(i);
        Serial.print(": Sent ");
        Serial.print(encoded_data[i]);
        Serial.print(", Received ");
        Serial.println(received_second[i]);
        delay(40);
    }
}

int find_5bit_value(bool* sixBitSequence) {
    for (int i = 0; i < 32; i++) {
        bool match = true;
        for (int j = 0; j < 6; j++) {
            if (ENCODE_5B6B[i][j] != sixBitSequence[j]) {
                match = false;
                break;
            }
        }
        if (match) return i;
    }
    return -1;
}

void decode_and_display() {
    Serial.println("\nDecoding received data and displaying on LEDs...");
    bool current_6bit[6];
    
    for (int block = 0; block < 26; block++) {
        for (int j = 0; j < 6; j++) {
            current_6bit[j] = received_second[(block * 6) + j];
            digitalWrite(LED_PINS[j], current_6bit[j]);
        }
        
        int value_5bit = find_5bit_value(current_6bit);
        
        if (value_5bit != -1) {
            for (int j = 0; j < 5; j++) {
                decoded_data[(block * 5) + j] = (value_5bit >> (4 - j)) & 1;
            }
            
            Serial.print("Block ");
            Serial.print(block);
            Serial.print(" - Received 6b: ");
            for (int j = 0; j < 6; j++) {
                Serial.print(current_6bit[j]);
            }
            Serial.print(" -> Decoded 5b: ");
            for (int j = 0; j < 5; j++) {
                Serial.print(decoded_data[(block * 5) + j]);
            }
            Serial.println();
        } else {
            Serial.print("ERROR: Invalid 6-bit sequence in block ");
            Serial.println(block);
        }
        
        delay(1000);
    }
}

void check_errors() {
    error_count = 0;
    Serial.println("\nChecking decode errors (original vs decoded):");
    for (int i = 2; i < 130; i++) {
        if (original_data[i] != decoded_data[i]) {
            error_count++;
            Serial.print("Mismatch at bit ");
            Serial.print(i);
            Serial.print(": Original ");
            Serial.print(original_data[i]);
            Serial.print(" but decoded as ");
            Serial.println(decoded_data[i]);
        }
    }
}

void loop() {
    Serial.println("\n=== Starting new transmission cycle ===\n");
    
    initialize_random_data();
    first_transmission();
    encode_received_data();
    second_transmission();
    decode_and_display();
    check_errors();
    
    Serial.print("\nTransmission complete. Total errors: ");
    Serial.println(error_count);
    if (error_count == 0) {
        Serial.println("SUCCESS: All bits transmitted and decoded correctly!");
    } else {
        Serial.println("WARNING: Errors detected in transmission/decoding!");
    }
    
    delay(5000);
}