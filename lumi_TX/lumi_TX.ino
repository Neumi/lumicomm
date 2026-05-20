const int LED_PIN = 13;
const unsigned long BIT_PERIOD_US = 208; // equals to 4800 baud
const unsigned long HALF_BIT_US = BIT_PERIOD_US / 2;

unsigned int messageID = 1; // Counter to track our messages
char txBuffer[64];          // Array to hold the dynamically built sentence

void smartDelay(unsigned long us) {
    delay(us / 1000);
    delayMicroseconds(us % 1000);
}

void sendBit(bool b) {
    if (b) {
        digitalWrite(LED_PIN, HIGH);
        smartDelay(HALF_BIT_US);
        digitalWrite(LED_PIN, LOW);
        smartDelay(HALF_BIT_US);
    } else {
        digitalWrite(LED_PIN, LOW);
        smartDelay(HALF_BIT_US);
        digitalWrite(LED_PIN, HIGH);
        smartDelay(HALF_BIT_US);
    }
}

void sendByte(uint8_t v) {
    for (int i = 7; i >= 0; i--) {
        sendBit((v >> i) & 1);
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    // 0. Build the dynamic message!
    // This injects the messageID into the string, then increments the ID by 1.
    snprintf(txBuffer, sizeof(txBuffer), "Msg ID [%u]: Hello, optical world!", messageID++);

    // 1. Preamble
    for (int i = 0; i < 16; i++) {
        sendBit(i % 2); 
    }

    // 2. Sync Byte
    sendByte(0xD5); 

    // 3. Send Message AND Calculate Checksum
    uint8_t checksum = 0;
    int i = 0;
    while (txBuffer[i] != '\0') {
        sendByte(txBuffer[i]);
        checksum ^= txBuffer[i]; 
        i++;
    }

    // 4. Send Null Terminator
    sendByte('\0');

    // 5. Send Checksum Byte
    sendByte(checksum);

    // 6. Idle Period
    digitalWrite(LED_PIN, LOW);
    delay(10); 
}