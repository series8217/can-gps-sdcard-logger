## Compatible boards

This is for the Longan Labs ObD-II CAN bus GPS dev kit, V2.0.

https://docs.longan-labs.cc/1030003/?fbclid=IwAR0dggiafDAoeiMO3dGJvFMwLghRtSnEgkrIWLar-EGupycz7Nz-sRUiWlo

I had to do some reverse engineering of the board because the
power control pin and chip select pins are not documented and
the examples from Longam Labs are incorrect.

## Features

This is a work in progress.

feature goals are:
- record GPS and CAN frames to a SavvyCAN conpatible CSV file
- automatically delete old logs to free up room for new ones
