# Sploosh Kaboom Starting Pattern Search

This program is trying to answer a question of [Linkus7](https://www.twitch.tv/linkus7) regarding the [Sinking Ships Minigame in Wind Waker](https://zelda.fandom.com/wiki/Sinking_Ships):

> What is the optimal pattern to hit at least one ship with 8 shots?

I'm solving this using a genetic algorithm that generates a candidate set of random patterns and then does a (large) number of simulated games, counting the number of times each of the candidates hit a ship.

It does this in rounds. At the end of each round, the worst performing half is discarded. The best performing quarter is used to generate mutated children where some random hit is moved to another random location. After that the program generates some new candidates to top the candidate set back up to its original size. After that a new round of simulations start. There's 100 rounds in total.

## Compiling and Running
On Linux call

```
$ make
```

then run with

```
$ ./splooshkaboom
```

You can tune various parameters by changing the constants defined in the `main()` function in `splooshkaboom.cpp`.

## Findings
The resulting winning patterns are surprizingly consistent.

There's essentially two patterns that perform best (along with their flipped variant). Each give about 87% chance to hit a ship.

### Two Lines
```
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   | X |   |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   |   | X |   |   |   |
+---+---+---+---+---+---+---+---+
|   | X |   |   |   | X |   |   |
+---+---+---+---+---+---+---+---+
|   |   | X |   |   |   | X |   |
+---+---+---+---+---+---+---+---+
|   |   |   | X |   |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   |   | X |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+
```

### Wind Mill
```
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   | X |   |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   |   | X |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   | X |   |   |   | X |   |
+---+---+---+---+---+---+---+---+
|   | X |   |   |   | X |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   | X |   |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   |   | X |   |   |   |
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+
```

## Future Work

It would also be interesting to find out the best pattern to hit at least 2 or 3 different ships. Some code changes will need to be necessary, but that shouldn't be so bad.

Another approach to conclusively pick the best pattern would be to generate all possible patterns. According to my calculations there should be less than 860160(8x7x2 x 8x6x2 x 8x5x2) patterns like that, so for the best candidates it should be feasible to try out all of them.