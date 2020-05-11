#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

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

struct partial_solution
{
	square_mask shot_locations = 0ull;
	square_mask revealed_squids = 0ull;
	u32 squids_found = 0u;
};

bool layout_matches_partial(const squid_layout &layout, const partial_solution &partial)
{
	assert(~partial.shot_locations & partial.revealed_squids == 0u);

	if (((~partial.shot_locations & layout.combined) | (partial.revealed_squids)) != layout.combined)
	{
		return false;
	}

	u32 found = 0;

	if ((partial.revealed_squids & layout.squid2) == layout.squid2)
	{
		found++;
	}

	if ((partial.revealed_squids & layout.squid3) == layout.squid3)
	{
		found++;
	}

	if ((partial.revealed_squids & layout.squid4) == layout.squid4)
	{
		found++;
	}

	return found == partial.squids_found;
}

template<u32 N>
struct game
{
	u8 packed_shots[N] = {0};
	double weight = 0;

public:

	void set_weight(double w)
	{
		weight = w;
	}

	double get_weight() const
	{
		return weight;
	}

	u8 get_raw(u32 index)
	{
		return packed_shots[index];
	}

	void set_shot(u32 index, u8 position, bool hit, bool sank_squid)
	{
		assert(index < N);
		assert(position < 64);
		packed_shots[index] = static_cast<u8>((position << 2u) | (hit ? 2u : 0u) | (sank_squid ? 1u : 0u));
	}

	void get(u32 index, u32 &pos, bool &is_hit, bool &sank_squid) const
	{
		assert(index < N);

		pos = packed_shots[index] >> 2;
		is_hit = (packed_shots[index] & 2u) != 0;
		sank_squid = (packed_shots[index] & 1u) != 0;
	}

	u32 get_pos(u32 index) const
	{
		assert(index < N);
		return packed_shots[index] >> 2;
	}

	bool is_hit(u32 index) const
	{
		assert(index < N);
		return (packed_shots[index] & 2u) != 0;
	}

	bool is_sank_squid(u32 index) const
	{
		return (packed_shots[index] & 1u) != 0;
	}

	bool operator== (const game<N> &other) const
	{
		return std::memcmp(packed_shots, other.packed_shots, sizeof(packed_shots)) == 0 && weight == other.weight;
	}

	bool operator< (const game<N> &other) const
	{
		int cmp = std::memcmp(packed_shots, other.packed_shots, sizeof(packed_shots));
		return cmp < 0 || (cmp == 0 && weight < other.weight);
	}

	void print () const
	{
		for (u32 i = 0; i < N; ++i)
		{
			u32 pos;
			bool is_hit, sank_squid;

			get(i, pos, is_hit, sank_squid);

			cout << pos;

			if (is_hit)
			{
				cout << 'h';
			}

			if (sank_squid)
			{
				cout << '!';
			}
			cout << ' ';
		}

		cout << ": " << weight << endl;
	}
};

template<u32 N>
void gen_random_game (const std::vector<squid_layout> &layouts, const partial_solution &partial, game<N> &g, std::mt19937 &rng)
{
	/* Pick a random layout */
	const squid_layout layout = layouts[randint(rng, layouts.size()-1)];

	/* Generate a random game that finds this layout */
	u8 positions[N];
	u32 positions_set = 0;

	/* Fill start position array with hits */
	square_mask not_found = layout.combined & ~partial.revealed_squids;

	while (not_found)
	{
		u32 pos = __builtin_ctzll(not_found);
		not_found &= ~(1ull << pos);

		positions[positions_set++] = static_cast<u8>(pos);
	}

	/* Fill up rest with arbitrary misses */
	not_found = ~(layout.combined | partial.shot_locations);
	while (positions_set < N)
	{
		u32 pos = __builtin_ctzll(_pdep_u64(1ull << randint(rng, __builtin_popcountll(not_found) - 1), not_found));
		not_found &= ~(1ull << pos);

		positions[positions_set++] = static_cast<u8>(pos);
	}

	/* Shuffle array */
	std::shuffle(&positions[0], &positions[N], rng);


	/* Fill in positions along with game metadata */
	square_mask shots = partial.shot_locations;
	for (u32 i = 0; i < N; ++i)
	{
		u8 pos = positions[i];
		square_mask pos_mask = 1ull << pos;

		shots |= pos_mask;

		bool is_hit = (layout.combined & pos_mask) != 0;
		bool sank_squid = false;

		if (is_hit)
		{
			if ((layout.squid2 & pos_mask) != 0 && (layout.squid2 & shots) == layout.squid2)
			{
				sank_squid = true;
			}
			else if ((layout.squid3 & pos_mask) != 0 && (layout.squid3 & shots) == layout.squid3)
			{
				sank_squid = true;
			}
			else if ((layout.squid4 & pos_mask) != 0 && (layout.squid4 & shots) == layout.squid4)
			{
				sank_squid = true;
			}
		}

		g.set_shot(i, pos, is_hit, sank_squid);
	}

	g.set_weight(layout.probability);
}

template<u32 N>
double calc_score(u32 level, typename std::vector<game<N> >::iterator &it, const typename std::vector<game<N>>::iterator &end)
{
	if (level == N)
	{
		double score = it->get_weight();
		it++;
		return score;
	}

	auto start = it;

	double best_miss = 0.0;
	double best_hit = 0.0;
	double best_sink = 0.0;

	while(1) {
		bool is_hit = it->is_hit(level);
		bool sank_squid = it->is_sank_squid(level);

		double score = calc_score<N>(level+1, it, end);

		if (sank_squid)
		{
			best_sink = std::max(score, best_sink);
		}
		else if (is_hit)
		{
			best_hit = std::max(score, best_hit);
		}
		else
		{
			best_miss = std::max(score, best_miss);
		}

		if (it == end)
		{
			break;
		}

		bool keep_going = true;
		for (u32 i = 0; i < level; ++i)
		{
			if (start->get_raw(i) != it->get_raw(i))
			{
				keep_going = false;
				break;
			}
		}

		if (start->get_pos(level) != it->get_pos(level))
		{
			keep_going = false;
		}

		if (!keep_going)
		{
			break;
		}

	}

	return best_miss + best_hit + best_sink;
}

template<u32 N>
std::pair<u32,u32> find_best_position(std::vector<squid_layout> &layouts, const partial_solution &partial, const u32 n_samples, std::mt19937 &rng)
{
	/* Remove layouts that don't match partial solution in advance */
	layouts.erase(std::remove_if(layouts.begin(), layouts.end(),
								 [&] (const squid_layout &layout) { return !layout_matches_partial(layout, partial); }),
				  layouts.end());

	/* Randomly sample possible winning games */
	std::vector<game<N>> games;
	for (u32 i = 0; i < n_samples; ++i)
	{
		auto &new_game = games.emplace_back();

		gen_random_game(layouts, partial, new_game, rng);
	}

	/* Sort games */
	std::sort(games.begin(), games.end());

	/* Find best opening */
	u32 best = ~0;
	double best_score = -1.0f;

	typename std::vector<game<N>>::iterator it = games.begin();
	while (it != games.end())
	{
		u32 pos = it->get_pos(0);
		double score = calc_score<N>(0, it, games.end());

		if (score > best_score)
		{
			best = pos;
			best_score = score;
		}
	}

	cout << best_score << endl;
	return std::pair<u32,u32>(best % 8, best / 8);
}


std::pair<u32,u32> find_best_position(u32 max_levels, std::vector<squid_layout> &layouts, const partial_solution &partial, const u32 n_samples, std::mt19937 &rng)
{
	switch (max_levels)
	{
	case 1:
		return find_best_position<1>(layouts, partial, n_samples, rng);
	case 2:
		return find_best_position<2>(layouts, partial, n_samples, rng);
	case 3:
		return find_best_position<3>(layouts, partial, n_samples, rng);
	case 4:
		return find_best_position<4>(layouts, partial, n_samples, rng);
	case 5:
		return find_best_position<5>(layouts, partial, n_samples, rng);
	case 6:
		return find_best_position<6>(layouts, partial, n_samples, rng);
	case 7:
		return find_best_position<7>(layouts, partial, n_samples, rng);
	case 8:
		return find_best_position<8>(layouts, partial, n_samples, rng);
	case 9:
		return find_best_position<9>(layouts, partial, n_samples, rng);
	case 10:
		return find_best_position<10>(layouts, partial, n_samples, rng);
	case 11:
		return find_best_position<11>(layouts, partial, n_samples, rng);
	case 12:
		return find_best_position<12>(layouts, partial, n_samples, rng);
	case 13:
		return find_best_position<13>(layouts, partial, n_samples, rng);
	case 14:
		return find_best_position<14>(layouts, partial, n_samples, rng);
	case 15:
		return find_best_position<15>(layouts, partial, n_samples, rng);
	case 16:
		return find_best_position<16>(layouts, partial, n_samples, rng);
	case 17:
		return find_best_position<17>(layouts, partial, n_samples, rng);
	case 18:
		return find_best_position<18>(layouts, partial, n_samples, rng);
	case 19:
		return find_best_position<19>(layouts, partial, n_samples, rng);
	case 20:
		return find_best_position<20>(layouts, partial, n_samples, rng);
	case 21:
		return find_best_position<21>(layouts, partial, n_samples, rng);
	case 22:
		return find_best_position<22>(layouts, partial, n_samples, rng);
	case 23:
		return find_best_position<23>(layouts, partial, n_samples, rng);
	case 24:
		return find_best_position<24>(layouts, partial, n_samples, rng);
	case 25:
		return find_best_position<25>(layouts, partial, n_samples, rng);
	case 26:
		return find_best_position<26>(layouts, partial, n_samples, rng);
	case 27:
		return find_best_position<27>(layouts, partial, n_samples, rng);
	case 28:
		return find_best_position<28>(layouts, partial, n_samples, rng);
	case 29:
		return find_best_position<29>(layouts, partial, n_samples, rng);
	default:
		assert(0);
	}

	return std::pair<u32,u32>(0,0);
}


int main()
{
	std::random_device dev;
	std::mt19937 rng(dev());

	std::vector<std::pair<double, square_mask> > candidates;

	auto all_layouts = generate_all_possible_squid_layouts();

	partial_solution partial = {};

	auto pos = find_best_position(18, all_layouts, partial, 100000, rng);

	cout << pos.first << " " << pos.second << endl;

}
