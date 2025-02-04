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

const byte CRC8_POLYNOMIAL = 0x07;
bool original_data[130];
bool received_first[130];
bool encoded_data[156];
bool received_second[156];
bool decoded_data[130];
int first_error_count = 0;
int second_error_count = 0;

byte calculateCRC8(bool* data, int length) {
    byte crc = 0;
    for (int i = 0; i < length; i++) {
        crc ^= (data[i] ? 1 : 0) << 7;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
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
    initialize_data();
}

void initialize_data() {
    original_data[0] = 0;
    original_data[1] = 1;
    for (int i = 2; i < 130; i++) {
        original_data[i] = random(2);
    }
}

void print_binary_array(const char* label, bool* array, int length) {
    Serial.print(label);
    for(int i = 0; i < length; i++) {
        if(i % 8 == 0) Serial.print(" ");
        Serial.print(array[i]);
    }
    Serial.println();
}

void first_transmission() {
    Serial.println("\n=== First Transmission Stage (TX1 -> RX2) ===");
    print_binary_array("Original Data (130b): ", original_data, 130);
    
    byte original_crc = calculateCRC8(original_data, 130);
    byte received_crc = 0;
    
    // Send original data bits
    for (int i = 0; i < 130; i++) {
        if(i == 50){
          Serial.print("Flipping bit at position ");
          Serial.print(i);
          Serial.print(" from ");
          Serial.print(original_data[i]);
          Serial.print(" to ");
          bool new_bit = (original_data[i] == 0 ? 1 : 0);
          Serial.println(new_bit);
          digitalWrite(TX_PIN_1, new_bit);
        } 
        else {
            digitalWrite(TX_PIN_1, original_data[i]);
        }
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

    // Send CRC bits
    for (int i = 0; i < 8; i++) {
        bool crc_bit = (original_crc >> (7-i)) & 1;
        digitalWrite(TX_PIN_1, crc_bit);
        delay(10);
        bool received_crc_bit = digitalRead(RX_PIN_2);
        received_crc |= (received_crc_bit << (7-i));
        delay(40);
    }
    
    print_binary_array("Received Data (130b): ", received_first, 130);
    
    byte calculated_received_crc = calculateCRC8(received_first, 130);
    
    if(received_crc != calculated_received_crc) {
        first_error_count++;
        Serial.print("CRC Mismatch - Received CRC: ");
        Serial.print(received_crc, BIN);
        Serial.print(" Calculated from received data: ");
        Serial.println(calculated_received_crc, BIN);
    }
}

void encode_received_data() {
    Serial.println("\n=== Encoding Stage ===");
    
    if (received_first[0] != 0 || received_first[1] != 1) {
        Serial.println("ERROR: First two bits must be 0 and 1");
        while(1);
    }
    
    received_first[0] = 0;
    received_first[1] = 0;
    
    for (int block = 0; block < 26; block++) {
        Serial.print("Block ");
        Serial.print(block);
        Serial.print(" (5b->6b): ");
        
        for (int j = 0; j < 5; j++) {
            Serial.print(received_first[(block * 5) + j]);
        }
        Serial.print(" -> ");
        
        int index = 0;
        for (int j = 0; j < 5; j++) {
            index = (index << 1) | received_first[(block * 5) + j];
        }
        for (int j = 0; j < 6; j++) {
            encoded_data[(block * 6) + j] = ENCODE_5B6B[index][j];
            Serial.print(encoded_data[(block * 6) + j]);
        }
        Serial.println();
    }
    
    print_binary_array("Complete encoded data (156b): ", encoded_data, 156);
}

void second_transmission() {
    Serial.println("\n=== Second Transmission Stage (TX2 -> RX1) ===");
    
    for (int block = 0; block < 26; block++) {
        Serial.print("\nBlock ");
        Serial.print(block);
        Serial.println(":");
        
        byte original_block_crc = calculateCRC8(&encoded_data[block * 6], 6);
        byte received_block_crc = 0;
        int bit_pos = random(6);
        for (int i = 0; i < 6; i++) {
            if(i == bit_pos){
               Serial.print(" from block: ");
               Serial.print(block);
               Serial.print(" from ");
               Serial.print(encoded_data[(block * 6) + i]);
               Serial.print(" to ");
              bool new_bit = (encoded_data[(block * 6) + i] == 0 ? 1 : 0);
              Serial.println(new_bit);
              digitalWrite(TX_PIN_2, new_bit);
            }
            else {
                digitalWrite(TX_PIN_2, encoded_data[(block * 6) + i]);
            }
            delay(10);
            received_second[(block * 6) + i] = digitalRead(RX_PIN_1);
            Serial.print("Bit ");
            Serial.print(i);
            Serial.print(": Sent ");
            Serial.print(encoded_data[(block * 6) + i]);
            Serial.print(", Received ");
            Serial.println(received_second[(block * 6) + i]);
            delay(40);
        }
        
        // Send block CRC
        for (int i = 0; i < 8; i++) {
            bool crc_bit = (original_block_crc >> (7-i)) & 1;
            digitalWrite(TX_PIN_2, crc_bit);
            delay(10);
            bool received_crc_bit = digitalRead(RX_PIN_1);
            received_block_crc |= (received_crc_bit << (7-i));
            delay(40);
        }
        
        byte calculated_received_block_crc = calculateCRC8(&received_second[block * 6], 6);
        
        if(received_block_crc != calculated_received_block_crc) {
            second_error_count++;
            Serial.print("CRC Mismatch in block ");
            Serial.print(block);
            Serial.print(" - Received CRC: ");
            Serial.print(received_block_crc, BIN);
            Serial.print(", Calculated from received data: ");
            Serial.println(calculated_received_block_crc, BIN);
        }
    }
}

void decode_and_display() {
    Serial.println("\n=== Decoding Stage ===");
    bool current_block[6];
    
    for (int block = 0; block < 26; block++) {
        Serial.print("Block ");
        Serial.print(block);
        Serial.print(": ");
        
        for (int j = 0; j < 6; j++) {
            current_block[j] = received_second[(block * 6) + j];
            digitalWrite(LED_PINS[j], current_block[j]);
            Serial.print(current_block[j]);
        }
        
        int value_5bit = -1;
        for (int i = 0; i < 32; i++) {
            bool match = true;
            for (int j = 0; j < 6; j++) {
                if (ENCODE_5B6B[i][j] != current_block[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                value_5bit = i;
                break;
            }
        }
        
        Serial.print(" -> ");
        if (value_5bit != -1) {
            for (int j = 0; j < 5; j++) {
                decoded_data[(block * 5) + j] = (value_5bit >> (4 - j)) & 1;
                Serial.print(decoded_data[(block * 5) + j]);
            }
        } else {
            Serial.print("INVALID");
        }
        Serial.println();
        
        delay(10);
    }
    
    print_binary_array("\nFinal decoded data (130b): ", decoded_data, 130);
}

void loop() {
    Serial.println("\n=== Starting new transmission cycle ===");
    first_error_count = 0;
    second_error_count = 0;
    
    initialize_data();
    first_transmission();
    encode_received_data();
    second_transmission();
    decode_and_display();
    
    Serial.print("\nTotal CRC errors - First transmission: ");
    Serial.print(first_error_count);
    Serial.print(", Second transmission: ");
    Serial.println(second_error_count);
    
    delay(5000);
}