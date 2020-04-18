# Sploosh Kaboom Starting Pattern Search

This program is trying to answer a question of [Linkus7](https://www.twitch.tv/linkus7) regarding the [Sinking Ships Minigame in Wind Waker](https://zelda.fandom.com/wiki/Sinking_Ships):

> What is the optimal pattern to hit at least one squid with 8 shots?

I'm solving this using a genetic algorithm that generates a candidate set of random patterns and then does a (large) number of simulated games, counting the number of times each of the candidates hit a squid.

It does this in rounds. At the end of each round, the worst performing half is discarded. The best performing quarter is used to generate mutated children where some random hit is moved to another random location. After that the program generates some new candidates to top the candidate set back up to its original size. After that a new round of simulations start. There's 100 rounds in total.

At the end it will test the best performers against all possible squid layouts and list the 3 best unique patterns along with their probabilities.

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
A rating function for various optimization goals can be assigned here. The following are available right now:

- `optimization_goal::at_least_1` Hit at least 1 squid
- `optimization_goal::at_least_2` Hit at least 2 unique squids
- `optimization_goal::at_least_3` Hit all 3 squids
- `optimization_goal::find_squid_2` Hit the length 2 squid
- `optimization_goal::find_squid_3` Hit the length 3 squid
- `optimization_goal::find_squid_4` Hit the length 4 squid
- `optimization_goal::max_hits` Find the pattern with the highest number of expected hits

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
Probability(at least 1): 86.5911%
Probability(at least 2): 41.2522%
Probability(at least 3): 6.29987%

```

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
Probability(at least 1): 86.5904%
Probability(at least 2): 41.2515%
Probability(at least 3): 6.30119%

```

These also seem to be the patterns that perform the best for finding at least 2 or all 3 squids, although the probability of those are of course lower.

## Future Work

The generated layouts have varying probabilities to be generated. This is currently not considered when rating the final candidates against all possible layout combinations.