# Snake Game on Atmel controller with IR port

## Components
List of physical components for realizing the idea.

![alt text](https://github.com/dimail777/snake_game_controller/blob/main/components.png?raw=true?raw=true) 

## How to work with Matrix 788BS 8x8
The topology of the matrix presents as:

![alt text](https://github.com/dimail777/snake_game_controller/blob/main/matrix.png?raw=true)  

For example, if you want to fire LEDs on (4,2) and (4,5) positions, needs to get voltage with next configuration:

![alt text](https://github.com/dimail777/snake_game_controller/blob/main/matrix_example.png?raw=true)  

If needs to fire LEDs with different rows, you should fire all rows in different time for avoid firing neighbour LEDs.

## How to work with IR port
The coding method was used here is NEC protocol.

* [NEC Protocol](https://www.sbprojects.net/knowledge/ir/nec.php)

![alt text](https://github.com/dimail777/snake_game_controller/blob/main/ir_protocol.png?raw=true)

The picture above shows a typical pulse train of the NEC protocol. 
A message is started by a 9ms AGC burst. 
This AGC burst is then followed by a 4.5ms space, which is then followed by the address and command. 
Address and Command are transmitted twice. 
The second time all bits are inverted and can be used for verification of the received message.

## How to connect all components

![alt text](https://github.com/dimail777/snake_game_controller/blob/main/schema.png?raw=true)

## Final Result

![alt text](https://github.com/dimail777/snake_game_controller/blob/main/1614093218524.jpg?raw=true)