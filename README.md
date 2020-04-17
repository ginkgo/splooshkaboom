# Sploosh Kaboom Starting Pattern Search

This program is trying to answer a question of [Linkus7](https://www.twitch.tv/linkus7) regarding the [Sinking Ships Minigame in Wind Waker](https://zelda.fandom.com/wiki/Sinking_Ships):

> What is the optimal pattern to hit at least one squid with 8 shots?

I'm solving this using a genetic algorithm that generates a candidate set of random patterns and then does a (large) number of simulated games, counting the number of times each of the candidates hit a squid.

It does this in rounds. At the end of each round, the worst performing half is discarded. The best performing quarter is used to generate mutated children where some random hit is moved to another random location. After that the program generates some new candidates to top the candidate set back up to its original size. After that a new round of simulations start. There's 100 rounds in total. At the end of those, the program will print the 3 best unique candidates.


You can tune various parameters by changing the constants defined in the `main()` function in `splooshkaboom.cpp`:

##### `PATTERN_SIZE`
Number of shots to use for generated patterns

##### `CANDIDATE_POPULATION`
Number of candidate patterns to consider per round

##### `TESTS`
Number of tests to perform per round. (i.e. number of layouts that will be generated per round)

##### `ROUNDS`
Number of test rounds

##### `GOAL`
Enum of various optimization goals. The following are available right now:


- `AT_LEAST_1` Hit at least 1 squid
- `AT_LEAST_2` Hit at least 2 unique squids
- `AT_LEAST_3` Hit all 3 squids
- `FIND_SQUID_2` Hit the length 2 squid
- `MAX_HITS` Find the pattern with the highest number of expected hits

## Compiling and Running
On Linux call

```
$ make
```

then run with

```
$ ./splooshkaboom
```

## Findings
The resulting winning patterns are surprizingly consistent.

There's essentially two patterns that perform best (along with their flipped variant). Each give about 87% chance to hit at least one squid.

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

These also seem to be the patterns that perform the best for finding at least 2 or all 3 squids, although the probability of those are of course lower.

## Future Work

Another approach to conclusively pick the best pattern would be to generate all possible patterns. According to my calculations there should be less than 860160(8x7x2 x 8x6x2 x 8x5x2) patterns like that, so for the best candidates it should be feasible to try out all of them.