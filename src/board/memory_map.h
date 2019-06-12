// EEPROM
#define EEPROM_REBOOT_COUNT   0x00000000
#define EEPROM_DEPLOY_STATUS 0x00000001
#define EEPROM_ADM_DEPLOY_ATTEMPTS 0x00000002
#define EEPROM_CONFIGURATION_DATA_ADDRESS 0x0100

// FRAM
#define FRAM_AVAILABLE  0
#define FRAM_USED       1

#define FRAM_PAYLOAD_SIZE     0x00069
#define FRAM_TABLE_SIZE       0x00261

#define FRAM_TABLE_OFFSET     0x00000 // 609 bytes (4870 bits)
#define FRAM_PAYLOAD_OFFSET   0x00261 // 4870 packets capacity
