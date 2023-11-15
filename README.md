# ISEC-SO2-PumpGame
(Sistemas Operativos II)
(Operating Systems II)
Finished in June 2022

I have been tasked with the development of a game (Pump Game) using C.

The game supports a maximum of two players and will utilize shared memory, named pipes, and a circular buffer in its architecture. 

Objective: https://classicreload.com/win3x-pipe-dream-1991.html (replicate this game)




# Features that were proposed to be implemented:

![image](https://user-images.githubusercontent.com/72463113/211691061-9ac26738-dbf7-44a0-a7fd-bac1eb5f29e7.png)
![image](https://user-images.githubusercontent.com/72463113/211691099-c96d838e-80c9-46f0-ad75-52438bd46f94.png)
![image](https://user-images.githubusercontent.com/72463113/211691125-fdec6cb4-da4e-49a8-9861-b9b6a5a5c748.png)

Image translator: https://translate.yandex.com/ocr




# Features that weren't developed 

● GUI

● Random mode

● List of players with their score

● Pause the game *(partially implemented)




# Known bugs

● None




# How to use the application 

● Include the server.h on the client and monitor

![image](https://user-images.githubusercontent.com/72463113/211689481-56f284be-f0fe-4d2a-b9ff-7a832897b37a.png)

●Run the Server program passing it the parameters: height width time;

● Run the Monitor program;

● Run the Client program;

● You will see in the monitor program the board with only the start (I) and end (F) positions. The goal is to place pieces in order to unite the two points;

● The commands available in Monitor are:

stop (seconds)- Stops the water movement during the specified time;
switch – To enable disable random mode;
quit – Exit the program.

● The commands available on the Server are:

list – Shows the number of monitors connected;
pause – Suspends the game;
resume – Resumes the game;
quit – Exit the program.

● The commands available on the Client are:

place (piece) (line) (column) - Places a piece in the indicated cell;
quit – Exit the program.

● The program ends when the dots have been correctly connected or when the water meets an impassable block or the board limits.




# My personal analysis of the project

This project was definitely one of the harder ones I've worked on but it was still a good experience. The way we handled communication between the server, client, and monitor was interesting to me. Even though we didn't get to implement every feature we wanted, we still got a lot done. The GUI was the part that was the most difficult for us. It couldn't have been done this good without the help of my team mate and teatchers.




# Final grade

69,3%




# Authors

João Morais and Bruno Mendes
