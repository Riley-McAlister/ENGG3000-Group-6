# Whack-a-Mole Project

This is the repository for a real-world, physical Whack-a-Mole game.

If you want to play the game, navigate to the directory `ENGG3000-Group-6/game` and run the executable like so:

`./game.exe`

The machine running the game will then need to connect to the ESP-32 created Wi-Fi network "ES3-AM-06-GATEWAY" in order to properly update the physical player's location.
In the absence of the player's physical location, the player is free to use the directional arrow keys to move the player around.

The player must run over moles to receive points in order to reach the next level. If you fail to get the required number of points before the round timer runs out, it results in a game over. If you complete all levels available, congratulations! You have beaten the game.

# Development Notes

This game is written in C with the graphics framework raylib. 
raylib will need to be installed before you are able to modify and compile the game.

The game is compiled using gcc; run the command below when inside of the ./game directory in order to create the runnable executable 'game.exe':

`gcc game.c network.c -o ./game.exe -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32`

The player's location is tracked in real time via three boxes housing ultrasonic sensors -- each of the boxes are managed by an ESP32 microcontroller. The ESP32 microcontrollers are identified by their MAC address. If you change the microcontrollers out you will need to update the MAC addresses in order to ensure the network transmits the right information.