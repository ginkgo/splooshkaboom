#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

extern "C"
{
#include <x86intrin.h>
}

using std::cout;
using std::endl;

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t square_mask;

struct squid_layout
{
	square_mask combined;
	square_mask squid2;
	square_mask squid3;
	square_mask squid4;
	double probability;
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
	assert(n < __builtin_popcountll(mask));

	return _pdep_u64(1ull << n, mask);

	/* Non-BMI2 implementation in case your CPU lacks HW support: */

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

	double squid2_prob = 1.0 / static_cast<double>(squids[2].size());
	for (auto s2 : squids[2])
	{
		u32 valid_s3s = 0;
		for (auto s3 : squids[3])
		{
			if ((s2 & s3) == 0)
			{
				valid_s3s++;
			}
		}

		double squid3_prob = 1.0 / static_cast<double>(valid_s3s);
		for (auto s3 : squids[3])
		{
			if (s2 & s3)
			{
				/* Invalid layout */
				continue;
			}

			u32 valid_s4s = 0;
			for (auto s4 : squids[4])
			{
				if ((s2 & s4) == 0 && (s3 & s4) == 0)
				{
					valid_s4s++;
				}
			}

			double squid4_prob = 1.0 / static_cast<double>(valid_s4s);
			for (auto s4 : squids[4])
			{
				if ((s2 & s4) == 0 && (s3 & s4) == 0)
				{
					/* Valid layout - insert into vector */
					layouts.push_back({s2|s3|s4, s2, s3, s4, squid2_prob * squid3_prob * squid4_prob});
				}
			}
		}
	}

#if !NDEBUG
	/* Verify probabilities by computing sum */
	double sum = 0.0;
	double min = 1.0;
	double max = 0.0;
	for (auto layout : layouts)
	{
		sum += layout.probability;
		min = std::min(layout.probability, min);
		max = std::max(layout.probability, max);
	}

	cout << "Layout probability sum: " << sum << endl;
	cout << "Minimum layout probability: " << min << endl;
	cout << "Maximum layout probability: " << max << endl;
#endif

	return std::move(layouts);
}


template<u32 N>
struct start_pattern
{
	u8 positions[N];

	start_pattern ()
	{
		/* Needed by compiler, but should never be called */
		assert(0);
	}


	start_pattern (std::mt19937 &rng)
	{
		square_mask mask = 0;

		for (u32 i = 0; i < N; ++i)
		{
			u32 b;
			do
			{
				b = randint(rng, 63);
			} while (mask & (1ull << b));

			positions[i] = static_cast<u8>(b);
		}
	}

	void get_pos(u32 i, u32 &x, u32 &y) const
	{
		assert(i < N);

		const u32 p = positions[i];

		x = p & 0x7;
		y = (p >> 3);
	}

	square_mask get_mask() const
	{
		square_mask mask = 0;

		for (u32 i = 0; i < N; ++i)
		{
			const u32 p = positions[i];

			assert(p < 64);

			mask |= (1ull << p);
		}

		return mask;
	}

	void print() const
	{
		u8 grid[WIDTH][WIDTH] = {0};

		for (u32 i = 0; i < N; ++i)
		{
			u32 x,y;
			get_pos(i,x,y);
			grid[x][y] = i+1;
		}

		cout << "\n+";
		for (u32 x = 0; x < WIDTH; ++x)
		{
			cout << "----+";
		}
		cout << endl;

		for (u32 y = 0; y < WIDTH; ++y)
		{
			cout << "|";
			for (u32 x = 0; x < WIDTH; ++x)
			{
				cout << " ";

				if (0 != grid[x][y])
				{
					cout << std::setfill(' ') << std::setw(2) << static_cast<u32>(grid[x][y]);
				}
				else
				{
					cout << "  ";
				}

				cout << " |";
			}
			cout << endl;

			cout << "+";
			for (u32 x = 0; x < WIDTH; ++x)
			{
				cout << "----+";
			}
			cout << endl;
		}
	}

	void mutate(std::mt19937 &rng)
	{
		/* pick random index */
		u32 idx = randint(rng, N-1);

		/* pick random new position */
		u32 new_pos = randint(rng, 63);

		/* Check if any other indices already use this value */
		for (u32 i = 0; i < N; ++i)
		{
			if (positions[i] == new_pos)
			{
				/* Swap and return */
				std::swap(positions[i], positions[idx]);
				return;
			}
		}

		/* Override otherwise */
		positions[idx] = new_pos;
	}

	start_pattern<N> mutated(std::mt19937 &rng) const
	{
		start_pattern<N> copy = *this;

		copy.mutate(rng);

		return copy;
	}

	bool operator== (const start_pattern& other) const
	{
		return std::memcmp(positions, other.positions, sizeof(positions)) == 0;
	}

	bool operator< (const start_pattern& other) const
	{
		return std::memcmp(positions, other.positions, sizeof(positions)) < 0;
	}
};

namespace optimization_goal
{
	/* Hit at least 1 squid */
	template<u32 N>
	u32 at_least_1(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		return (layout.combined & candidate.get_mask()) ? 1 : 0;
	}

	/* Hit at least 2 unique squids */
	template<u32 N>
	u32 at_least_2(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		u32 hit_count = 0;

		hit_count += (candidate.get_mask() & layout.squid2) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid3) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid4) ? 1 : 0;

		return (hit_count >= 2) ? 1 : 0;
	}

	/* Hit all 3 squids */
	template<u32 N>
	u32 at_least_3(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		u32 hit_count = 0;

		hit_count += (candidate.get_mask() & layout.squid2) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid3) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid4) ? 1 : 0;

		return (hit_count >= 3) ? 1 : 0;
	}

	/* Hit the length 2 squid */
	template<u32 N>
	u32 find_squid_2(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		return (layout.squid2 & candidate.get_mask()) ? 1 : 0;
	}

	/* Hit the length 3 squid */
	template<u32 N>
	u32 find_squid_3(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		return (layout.squid3 & candidate.get_mask()) ? 1 : 0;
	}

	/* Hit the length 4 squid */
	template<u32 N>
	u32 find_squid_4(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		return (layout.squid4 & candidate.get_mask()) ? 1 : 0;
	}

	/* Find the pattern with the highest number of expected hits */
	template<u32 N>
	u32 max_hits(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		return __builtin_popcountll(candidate.get_mask() & layout.combined);
	}

	/* Find nothing - anti-optimization*/
	template<u32 N>
	u32 find_0(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		return (candidate.get_mask() & layout.combined) ? 0 : 1;
	}

	/* Find exactly one squid */
	template<u32 N>
	u32 find_1(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		u32 hit_count = 0;

		hit_count += (candidate.get_mask() & layout.squid2) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid3) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid4) ? 1 : 0;

		return (hit_count == 1) ? 1 : 0;
	}

	/* Find exactly two squids */
	template<u32 N>
	u32 find_2(const start_pattern<N> &candidate, const squid_layout &layout)
	{
		u32 hit_count = 0;

		hit_count += (candidate.get_mask() & layout.squid2) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid3) ? 1 : 0;
		hit_count += (candidate.get_mask() & layout.squid4) ? 1 : 0;

		return (hit_count == 2) ? 1 : 0;
	}
}

int main()
{
	const u32 PATTERN_SIZE = 8;
	const u32 CANDIDATE_POPULATION = 1 << 13;
	const u32 TESTS = 1 << 13;
	const u32 ROUNDS = 100;

	const auto GOAL = optimization_goal::at_least_1<PATTERN_SIZE>;

	std::random_device dev;
	std::mt19937 rng(dev());

	std::vector<std::pair<double, start_pattern<PATTERN_SIZE> > > candidates;

	auto all_layouts = generate_all_possible_squid_layouts();


	/* start_pattern<PATTERN_SIZE> pattern(rng); */
	/* for (u32 i = 0; i < 10; ++i) */
	/* { */
	/* 	pattern.mutate(rng); */
	/* 	pattern.print(); */
	/* } */
	/* return 0; */

	for (u32 round = 0; round < ROUNDS; ++round)
	{
		cout << "Round " << round << endl;

		for (auto &candidate : candidates)
		{
			candidate.first = 0;
		}

		while(candidates.size() < CANDIDATE_POPULATION)
		{
			candidates.emplace_back(0, start_pattern<PATTERN_SIZE>(rng));
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
		candidates[0].second.print();
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
			candidates.emplace_back(0, candidates[i].second.mutated(rng));
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
			candidate.first += GOAL(candidate.second, layout) * layout.probability;
		}
	}

	std::sort(candidates.begin(), candidates.end(), std::greater<>());

	cout << "Five best (unique)" << endl;
	for (u32 i = 0; i < std::min(5, static_cast<int>(candidates.size())); ++i)
	{
		cout << "#" << i+1;
		candidates.at(i).second.print();
		cout << "Probability: "
			 << 100.0 * static_cast<double>(candidates.at(i).first)
			 << "%"
			 << endl << endl;
	}
}
