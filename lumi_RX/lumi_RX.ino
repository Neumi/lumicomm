const unsigned long BIT_PERIOD_US = 208;  // equals to 4800 baud
const unsigned long DELAY_75_US = (BIT_PERIOD_US * 3) / 4;
const unsigned long WATCHDOG_US = (BIT_PERIOD_US * 5) / 2; 

int signalMin = 1023;
int signalMax = 0;
unsigned long lastDecayTime = 0;
unsigned long lastTelemetryTime = 0; 

bool currentLevel = 0;
bool lastLevel = 0;

uint8_t shiftReg = 0;
bool synced = false;
int bitCount = 0;
int preambleCount = 0;
unsigned long lastEdgeTime = 0;

char msgBuffer[64];          
int bufferIdx = 0;           
uint8_t calculatedCheck = 0; 
bool expectingCheck = false; 

// --- DIRECT REGISTER ADC SETUP ---
void setupADC() {
    // Set reference voltage to AVCC (5V) and hardcode multiplexer to channel A0
    ADMUX = (1 << REFS0);
    
    // Enable the ADC, and set the Prescaler to 16 (16MHz / 16 = 1MHz ADC clock)
    // Arduino default is 128. This makes conversions 6x faster!
    ADCSRA = (1 << ADEN) | (1 << ADPS2); 
}

// --- DIRECT REGISTER READ ---
// Skips all Arduino overhead. Takes ~16us instead of ~104us.
inline int fastAnalogReadA0() {
    ADCSRA |= (1 << ADSC);                 // Fire the "Start Conversion" bit
    while (ADCSRA & (1 << ADSC));          // Wait in a micro-loop until hardware clears the bit
    return ADC;                            // Return the 10-bit result directly from hardware
}

void smartDelay(unsigned long us) {
    delay(us / 1000);
    delayMicroseconds(us % 1000);
}

void setup() {
    Serial.begin(115200);
    
    setupADC(); // Initialize the high-speed analog registers
    
    Serial.println("{\"status\":\"boot\"}");
    
    currentLevel = readLevel();
    lastLevel = currentLevel;
    lastEdgeTime = micros();
}

bool readLevel() {
    // Call our blazing-fast register function instead of analogRead(A0)
    int val = fastAnalogReadA0(); 
    
    if (val > signalMax) signalMax = val;
    if (val < signalMin) signalMin = val;
    
    unsigned long now = micros();
    if (now - lastDecayTime > 2000) { 
        if (signalMax > signalMin) signalMax--;
        if (signalMin < signalMax) signalMin++;
        lastDecayTime = now;
    }
    
    int mid = (signalMax + signalMin) / 2;
    int hysteresis = (signalMax - signalMin) / 4; 
    if (hysteresis < 8) hysteresis = 8;
    
    if (val > mid + hysteresis) return 1;
    if (val < mid - hysteresis) return 0;
    
    return currentLevel; 
}

void loop() {
    currentLevel = readLevel();
    unsigned long now = micros();

    // --- 1. SMART WATCHDOG ---
    if (now - lastEdgeTime > WATCHDOG_US) {
        synced = false;
        bitCount = 0;
        shiftReg = 0;
        preambleCount = 0;
        
        int ambient = fastAnalogReadA0();
        signalMax = ambient + 5; 
        signalMin = ambient - 5;
    }

    // --- 2. TELEMETRY BLASTER ---
    if (!synced && (now - lastEdgeTime > 800000)) {
        if (now - lastTelemetryTime > 500000) {
            lastTelemetryTime = now;
            int mid = (signalMax + signalMin) / 2;
            Serial.print("{\"status\":\"telemetry\",\"min\":");
            Serial.print(signalMin);
            Serial.print(",\"max\":");
            Serial.print(signalMax);
            Serial.print(",\"mid\":");
            Serial.print(mid);
            Serial.println("}");
        }
    }

    // --- 3. EDGE DETECTOR & DECODER ---
    if (currentLevel != lastLevel) {
        bool bit = lastLevel; 
        bool previousBit = shiftReg & 1; 
        
        shiftReg = (shiftReg << 1) | bit; 

        if (!synced) {
            if (bit != previousBit) {
                preambleCount++;
            } else {
                preambleCount = 0; 
            }

            if (shiftReg == 0xD5 && preambleCount >= 4) {
                synced = true;
                bitCount = 0;
                preambleCount = 0;
                bufferIdx = 0;
                calculatedCheck = 0;
                expectingCheck = false;
            }
        } 
        else {
            bitCount++;
            if (bitCount == 8) {
                if (expectingCheck) {
                    int mid = (signalMax + signalMin) / 2;
                    if (shiftReg == calculatedCheck) {
                        Serial.print("{\"status\":\"ok\",\"msg\":\"");
                        Serial.print(msgBuffer);
                        Serial.print("\",\"min\":"); Serial.print(signalMin);
                        Serial.print(",\"max\":"); Serial.print(signalMax);
                        Serial.print(",\"mid\":"); Serial.print(mid);
                        Serial.println("}");
                    } else {
                        Serial.print("{\"status\":\"error\",\"min\":"); Serial.print(signalMin);
                        Serial.print(",\"max\":"); Serial.print(signalMax);
                        Serial.print(",\"mid\":"); Serial.print(mid);
                        Serial.println("}");
                    }
                    synced = false; 
                } 
                else if (shiftReg == '\0') {
                    msgBuffer[bufferIdx] = '\0';
                    expectingCheck = true; 
                } 
                else if (shiftReg >= 32 && shiftReg <= 126) {
                    if (bufferIdx < 63) {
                        msgBuffer[bufferIdx++] = (char)shiftReg;
                        calculatedCheck ^= shiftReg; 
                    }
                } 
                else {
                    synced = false; 
                }
                bitCount = 0;
            }
        }

        smartDelay(DELAY_75_US);
        currentLevel = readLevel();
        lastLevel = currentLevel;
        lastEdgeTime = micros(); 
    }
}