#include <cstdio>
#include <cstdint>
#include <cassert>

#include <random>
#include <vector>
#include <algorithm>
#include <iostream>

extern "C"
{
#include <x86intrin.h>
}

typedef uint32_t u32;
typedef uint64_t square_mask;

#define WIDTH 8

u32 square_offset(u32 x, u32 y)
{
	assert(x < WIDTH);
	assert(y < WIDTH);

	return x + WIDTH * y;
}

void square_set(square_mask &mask, u32 x, u32 y)
{
	mask |= (1ull << square_offset(x,y));
}

bool square_get(square_mask mask, u32 x, u32 y)
{
	return (mask & (1ull << square_offset(x,y))) != 0;
}

void print_square(square_mask mask)
{
	std::cout << "\n+";
	for (u32 x = 0; x < WIDTH; ++x)
	{
		std::cout << "---+";
	}
	std::cout << std::endl;

	for (u32 y = 0; y < WIDTH; ++y)
	{
		std::cout << "|";
		for (u32 x = 0; x < WIDTH; ++x)
		{
			std::cout << (square_get(mask, x, y) ? " X |" : "   |");
		}
		std::cout << std::endl;

		std::cout << "+";
		for (u32 x = 0; x < WIDTH; ++x)
		{
			std::cout << "---+";
		}
		std::cout << std::endl;
	}
}

u32 randint(std::mt19937 &rng, u32 max)
{
	std::uniform_int_distribution<u32> dist(0,max);

	return dist(rng);
}

square_mask insert_squid(square_mask current, u32 squid_length, std::mt19937 &rng)
{
	square_mask new_squid;
	do {
		new_squid = 0;

		if (randint(rng, 1) == 0)
		{
			/* horizontal */
			u32 x = randint(rng, 8-squid_length);
			u32 y = randint(rng, 7);

			for (u32 i = 0; i < squid_length; ++i)
			{
				square_set(new_squid, x+i, y);
			}
		}
		else
		{
			/* vertical */
			u32 x = randint(rng, 7);
			u32 y = randint(rng, 8-squid_length);

			for (u32 i = 0; i < squid_length; ++i)
			{
				square_set(new_squid, x, y+i);
			}
		}
	} while ((current & new_squid) != 0);

	return new_squid;
}

square_mask generate_squids(std::mt19937 &rng, square_mask &squid2, square_mask &squid3, square_mask &squid4)
{
	square_mask current = 0;

	squid2 = insert_squid(current, 2, rng);
	current |= squid2;

	squid3 = insert_squid(current, 3, rng);
	current |= squid3;

	squid4 = insert_squid(current, 4, rng);
	current |= squid4;

	return current;
}

square_mask nth_set(u32 n, square_mask mask)
{
	return _pdep_u64(1ull << n, mask);

	/* Non-BMI2 implementation in case your CPU lacks HW support: */
	/* assert(n < 64); */
	/* assert(n < std::popcount(mask)); */

	/* for (u32 bit = 0; bit < 64; bit++) */
	/* { */
	/* 	if (mask & (1ull << bit)) */
	/* 	{ */
	/* 		if (n == 0) */
	/* 		{ */
	/* 			return 1ull << bit; */
	/* 		} */
	/* 		n--; */
	/* 	} */
	/* } */

	/* assert(0); */
	/* return 0; */
}

square_mask generate_pattern(std::mt19937 &rng, u32 tries)
{
	square_mask pattern = 0ull;
	for (u32 i = 0; i < tries; ++i)
	{
		/* Find random unset bit */
		square_mask unset_bit = nth_set(randint(rng, 63-i), ~pattern);

		/* Set */
		pattern |= unset_bit;
	}

	return pattern;
}

square_mask mutate_pattern(std::mt19937 &rng, square_mask original)
{
	u32 pop = __builtin_popcountll(original);

	/* Find random set bit */
	square_mask set_bit   = nth_set(randint(rng, pop-1), original);

	/* Find random unset bit */
	square_mask unset_bit = nth_set(randint(rng, 63-pop), ~original);

	assert(set_bit & original);
	assert(unset_bit & ~original);
	assert((set_bit & unset_bit) == 0);

	/* Flip those bits */
	original ^= set_bit;
	original ^= unset_bit;

	return original;
}

enum class optimization_goal
{
	AT_LEAST_1,    /* Hit at least 1 squid */
	AT_LEAST_2,    /* Hit at least 2 unique squids */
	AT_LEAST_3,    /* Hit all 3 squids */
	FIND_SQUID_2,  /* Hit the length 2 squid */
	MAX_HITS,      /* Find the pattern with the highest number of expected hits */
};

int main()
{
	const u32 PATTERN_SIZE = 8;
	const u32 CANDIDATE_POPULATION = 1 << 18;
	const u32 TESTS = 1 << 12;
	const u32 ROUNDS = 100;

	const optimization_goal GOAL = optimization_goal::AT_LEAST_1;

	std::random_device dev;
    std::mt19937 rng(dev());

	std::vector<std::pair<u32, square_mask> > candidates;

	for (u32 round = 0; round < ROUNDS; ++round)
	{
		std::cout << "Round " << round << std::endl;

		for (auto &candidate : candidates)
		{
			candidate.first = 0;
		}

		while(candidates.size() < CANDIDATE_POPULATION)
		{
			candidates.emplace_back(0, generate_pattern(rng, PATTERN_SIZE));
		}

		for (u32 test = 0; test < TESTS; ++test)
		{
			square_mask s2,s3,s4;

			square_mask board = generate_squids(rng, s2, s3, s4);

			for (auto &candidate : candidates)
			{
				if (GOAL == optimization_goal::AT_LEAST_1)
				{
					candidate.first += (candidate.second & board) ? 1 : 0;
				}
				else if (GOAL == optimization_goal::AT_LEAST_2)
				{
					u32 hit_count = 0;

					hit_count += (candidate.second & s2) ? 1 : 0;
					hit_count += (candidate.second & s3) ? 1 : 0;
					hit_count += (candidate.second & s4) ? 1 : 0;

					candidate.first += (hit_count >= 2) ? 1 : 0;
				}
				else if (GOAL == optimization_goal::AT_LEAST_3)
				{
					u32 hit_count = 0;

					hit_count += (candidate.second & s2) ? 1 : 0;
					hit_count += (candidate.second & s3) ? 1 : 0;
					hit_count += (candidate.second & s4) ? 1 : 0;

					candidate.first += (hit_count >= 3) ? 1 : 0;
				}
				else if (GOAL == optimization_goal::FIND_SQUID_2)
				{
					candidate.first += (candidate.second & board) ? 1 : 0;
				}
				else if (GOAL == optimization_goal::MAX_HITS)
				{
					candidate.first += __builtin_popcountll(candidate.second & board);
				}
				else
				{
					__builtin_unreachable();
				}

			}
		}

		std::sort(candidates.begin(), candidates.end(), std::greater<>());

		std::cout << "Best: " << std::endl;
		print_square(candidates[0].second);
		for (u32 i = 0; i < 10; ++i)
		{
			auto &candidate = candidates[i];
			u32 hits = candidate.first;
			std::cout << 100.0 * static_cast<float>(hits) / static_cast<float>(TESTS) << std::endl;
		}

		std::cout << "Worst: " << std::endl;
		for (u32 i = 0; i < 10; ++i)
		{
			auto &candidate = candidates[candidates.size() - i - 1];
			u32 hits = candidate.first;
			std::cout << 100.0 * static_cast<float>(hits) / static_cast<float>(TESTS) << std::endl;
		}

		candidates.resize(candidates.size() / 2);

		u32 old_size = candidates.size() / 2;
		for (u32 i = 0; i < old_size; ++i)
		{
			candidates.emplace_back(0, mutate_pattern(rng, candidates[i].second));
		}
	}

	std::cout << "Three best (unique)" << std::endl;
	square_mask last = 0;
	u32 idx = 0;
	for (u32 i = 0; i < 3; ++i)
	{
		while (candidates[idx].second == last)
		{
			idx++;
		}

		print_square(candidates[idx].second);
		last = candidates[idx].second;
	}
}
