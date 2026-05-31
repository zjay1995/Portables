#include <EEPROM.h>  // Include EEPROM library to read from EEPROM

class Gas {
public:
    Gas() {
        // Constructor can be empty or used to initialize other members if needed
    }

    // Utility function to read a double from EEPROM at a specific address
    double readDoubleFromEEPROM(int address) {
        double value;
        EEPROM.get(address, value);  // Read the value from EEPROM
        return value;
    }

    double calculateSLM(int voltage) {
        // Read the gas index from its EEPROM address
        int gasIndex = EEPROM.read(EEPROM_GAS_SELECT_OFFSET);

        // Initialize variables for intercept and slope
        double intercept = 0.0;
        double slope = 0.0;

        // Select the appropriate EEPROM addresses based on the gas index
        switch (gasIndex) {
            case 0:
                intercept = readDoubleFromEEPROM(8);
                slope = readDoubleFromEEPROM(16);
                break;
            case 1:
                intercept = readDoubleFromEEPROM(24);
                slope = readDoubleFromEEPROM(32);
                break;
            case 2:
                intercept = readDoubleFromEEPROM(40);
                slope = readDoubleFromEEPROM(48);
                break;
            case 3:
                intercept = readDoubleFromEEPROM(56);
                slope = readDoubleFromEEPROM(64);
                break;
            default:
                // Handle unexpected gas index, perhaps set to default values or log an error
                intercept = readDoubleFromEEPROM(EEPROM_GAS_INTERCEPT_OFFSET); // Default to Gas 0's intercept
                slope = readDoubleFromEEPROM(EEPROM_GAS_SLOPE_OFFSET); // Default to Gas 0's slope
                break;
        }

        return (voltage - intercept) / slope;  // Use the read intercept and slope for calculation
    }
};
