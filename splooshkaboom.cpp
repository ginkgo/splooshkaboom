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

using std::cout;
using std::endl;

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t square_mask;

struct squid_layout
{
	square_mask combined;
	square_mask squid2;
	square_mask squid3;
	square_mask squid4;
};

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
	cout << "\n+";
	for (u32 x = 0; x < WIDTH; ++x)
	{
		cout << "---+";
	}
	cout << endl;

	for (u32 y = 0; y < WIDTH; ++y)
	{
		cout << "|";
		for (u32 x = 0; x < WIDTH; ++x)
		{
			cout << (square_get(mask, x, y) ? " X |" : "   |");
		}
		cout << endl;

		cout << "+";
		for (u32 x = 0; x < WIDTH; ++x)
		{
			cout << "---+";
		}
		cout << endl;
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

void generate_squids(std::mt19937 &rng, squid_layout &layout)
{
	layout.squid2 = insert_squid(0ull, 2, rng);
	layout.combined = layout.squid2;

	layout.squid3 = insert_squid(layout.combined, 3, rng);
	layout.combined |= layout.squid3;

	layout.squid4 = insert_squid(layout.combined, 4, rng);
	layout.combined |= layout.squid4;
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

square_mask generate_squid(u32 size, u32 x, u32 y, bool horizontal)
{
	square_mask mask = 0;

	for (u32 i = 0; i < size; ++i)
	{
		if (horizontal)
		{
			square_set(mask, x+i, y);
		}
		else
		{
			square_set(mask, x, y+i);
		}
	}

	return mask;
}

std::vector<squid_layout> generate_all_possible_squid_layouts()
{
	std::vector<squid_layout> layouts;
	std::vector<square_mask> squids[5];

	for (u32 size = 2; size <= 4; size++)
	{
		for (u32 x = 0; x <= 8 - size; ++x)
		{
			for (u32 y = 0; y < 8; ++y)
			{
				/* cout << size << " " << x << " " << y << endl; */
				squids[size].push_back(generate_squid(size, x, y, true));
				squids[size].push_back(generate_squid(size, y, x, false));
			}
		}
	}

	for (auto s2 : squids[2])
	{
		for (auto s3 : squids[3])
		{
			for (auto s4 : squids[4])
			{
				square_mask m = s2;

				if ((s2 & s3) || (s2 & s4) || (s3 & s4))
				{
					/* Invalid layout */
					continue;
				}
				else
				{
					/* Valid layout - insert into vector */
					layouts.push_back({s2|s3|s4, s2, s3, s4});
				}
			}
		}
	}

	return std::move(layouts);
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

namespace optimization_goal
{
	/* Hit at least 1 squid */
	u32 at_least_1(const square_mask &candidate, const squid_layout &layout)
	{
		return (layout.combined & candidate) ? 1 : 0;
	}

	/* Hit at least 2 unique squids */
	u32 at_least_2(const square_mask &candidate, const squid_layout &layout)
	{
		u32 hit_count = 0;

		hit_count += (candidate & layout.squid2) ? 1 : 0;
		hit_count += (candidate & layout.squid3) ? 1 : 0;
		hit_count += (candidate & layout.squid4) ? 1 : 0;

		return (hit_count >= 2) ? 1 : 0;
	}

	/* Hit all 3 squids */
	u32 at_least_3(const square_mask &candidate, const squid_layout &layout)
	{
		u32 hit_count = 0;

		hit_count += (candidate & layout.squid2) ? 1 : 0;
		hit_count += (candidate & layout.squid3) ? 1 : 0;
		hit_count += (candidate & layout.squid4) ? 1 : 0;

		return (hit_count >= 3) ? 1 : 0;
	}

	/* Hit the length 2 squid */
	u32 find_squid_2(const square_mask &candidate, const squid_layout &layout)
	{
		return (layout.squid2 & candidate) ? 1 : 0;
	}

	/* Hit the length 3 squid */
	u32 find_squid_3(const square_mask &candidate, const squid_layout &layout)
	{
		return (layout.squid3 & candidate) ? 1 : 0;
	}

	/* Hit the length 4 squid */
	u32 find_squid_4(const square_mask &candidate, const squid_layout &layout)
	{
		return (layout.squid4 & candidate) ? 1 : 0;
	}

	/* Find the pattern with the highest number of expected hits */
	u32 max_hits(const square_mask &candidate, const squid_layout &layout)
	{
		return __builtin_popcountll(candidate & layout.combined);
	}
}

int main()
{
	const u32 PATTERN_SIZE = 8;
	const u32 CANDIDATE_POPULATION = 1 << 14;
	const u32 TESTS = 1 << 14;
	const u32 ROUNDS = 100;

	const auto GOAL = optimization_goal::at_least_1;

	std::random_device dev;
    std::mt19937 rng(dev());

	std::vector<std::pair<u64, square_mask> > candidates;

	auto all_layouts = generate_all_possible_squid_layouts();

	for (u32 round = 0; round < ROUNDS; ++round)
	{
		cout << "Round " << round << endl;

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
			squid_layout layout;
			generate_squids(rng, layout);

			for (auto &candidate : candidates)
			{
				candidate.first += GOAL(candidate.second, layout);
			}
		}

		std::sort(candidates.begin(), candidates.end(), std::greater<>());

		cout << "Best: " << endl;
		print_square(candidates[0].second);
		for (u32 i = 0; i < 10; ++i)
		{
			auto &candidate = candidates[i];
			u64 hits = candidate.first;
			cout << 100.0 * static_cast<float>(hits) / static_cast<float>(TESTS) << endl;
		}

		cout << "Worst: " << endl;
		for (u32 i = 0; i < 10; ++i)
		{
			auto &candidate = candidates[candidates.size() - i - 1];
			u64 hits = candidate.first;
			cout << 100.0 * static_cast<float>(hits) / static_cast<float>(TESTS) << endl;
		}

		candidates.resize(candidates.size() / 2);

		u32 old_size = candidates.size() / 2;
		for (u32 i = 0; i < old_size; ++i)
		{
			candidates.emplace_back(0, mutate_pattern(rng, candidates[i].second));
		}
	}

	/* Take N best performers from last round of GA and test against all combinations */
	cout << "Doing final rating.." << endl;

	/* Remove duplicates */
	std::sort(candidates.begin(), candidates.end(), std::greater<>());
	candidates.erase( std::unique( candidates.begin(), candidates.end() ), candidates.end() );

	const u32 N = 100;
	static_assert(N < CANDIDATE_POPULATION);

	candidates.resize(std::min(N, static_cast<u32>(candidates.size())));
	for (auto &candidate : candidates)
	{
		candidate.first = 0;

		for (const auto &layout : all_layouts)
		{
			candidate.first += GOAL(candidate.second, layout);
		}
	}

	std::sort(candidates.begin(), candidates.end(), std::greater<>());

	cout << "Five best (unique)" << endl;
	for (u32 i = 0; i < std::min(5, static_cast<int>(candidates.size())); ++i)
	{
		cout << "#" << i+1;
		print_square(candidates.at(i).second);
		cout << "Probability: "
			 << 100.0 * static_cast<double>(candidates.at(i).first) / static_cast<double>(all_layouts.size())
			 << "%"
			 << endl << endl;
	}
}
