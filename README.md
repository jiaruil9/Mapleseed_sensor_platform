### Features
Real time systems of GPS Location tracking and rf packet transmission for the Mapleseed sensor platform. 

## Mapleseed sensor platform

###Hardware Setup
- Texas Instrument CC1310 Launchpad x2
- MikroElektronika Nano GPS click Nano Hornet
- One Launchpad connected to the gps click board serves as Tx and the other one serves as Rx

###Software Setup
- Compiler: Code Composer Studio
- Load rfPacketTx to the Tx Launchpad and rfPacketRx to the Rx Launchpad
- UART port on Rx Launchpad to read message received

