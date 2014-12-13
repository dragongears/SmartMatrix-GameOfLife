# Game Of Life

[Conway's Game of Life](http://en.wikipedia.org/wiki/Conway's_Game_of_Life) for the [Teensy 3.1](http://pjrc.com) and [SmartMatrix Shield](http://pixelmatix.com).

Remote control functions:

	Fast forward: Increase speed/Leave single step mode
	Rewind: Decrease speed/Leave single step mode
	Play/Pause: Enter single step mode/Step to next generation
	Phone: Change color
	EQ: Toggle wrap on and off
	CH+: Increase display brightness
	CH-: Decrease display brightness
	5: Enter edit mode
	0: Randomize field

Edit mode (Green cursor: live cell under cursor, Red cursor: dead cell under cursor):

	Play/Pause: Leave edit mode
	0: Clear field
	+: Live cell at cursor
	-: Dead cell at cursor
	4: Move cursor left
	6: Move cursor right
	2: Move cursor up
	8: Move cursor down
	5: Center cursor
