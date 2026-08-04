// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

// This file is used to compile a cli tool that will construct and print the
// SZ sequence generator matrices in an optimal format for the main library.
// Resulting programs are inlined into a header file include/oqmc/owen.h, and
// the matrices into the reference test in src/tests/owen.cpp.
//
// Matrices are constructed following the work by Abdalla G. M. Ahmed, Matt
// Pharr, Victor Ostromoukhov and Hui Huang in 'SZ Sequences:
// Binary-Constructed (0, 2^q)-Sequences'.

#include <oqmc/reverse.h>

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

constexpr auto numDimensions = 4;
constexpr auto size = 16;

// GF(4) elements as 2x2 GF(2) matrices, packed as [a b; c d].
struct Block
{
	int a, b, c, d;
};

constexpr Block multiply(Block x, Block y)
{
	return {(x.a & y.a) ^ (x.b & y.c), (x.a & y.b) ^ (x.b & y.d),
	        (x.c & y.a) ^ (x.d & y.c), (x.c & y.b) ^ (x.d & y.d)};
}

constexpr bool equal(Block x, Block y)
{
	return x.a == y.a && x.b == y.b && x.c == y.c && x.d == y.d;
}

// A generator of the multiplicative group of GF(4), an invertible block whose
// cube is the identity but whose square is not. Two of the six invertible
// blocks qualify, either will do, so the scan takes the first.
constexpr Block findAlpha()
{
	const Block zero = {0, 0, 0, 0};
	const Block identity = {1, 0, 0, 1};

	for(int bits = 0; bits < 16; ++bits)
	{
		const Block candidate = {(bits >> 3) & 1, (bits >> 2) & 1,
		                         (bits >> 1) & 1, bits & 1};

		if(equal(candidate, zero) || equal(candidate, identity))
		{
			continue;
		}

		const Block squared = multiply(candidate, candidate);
		const Block cubed = multiply(squared, candidate);

		if(equal(cubed, identity) && !equal(squared, identity))
		{
			return candidate;
		}
	}

	assert(false);
	return identity;
}

// The generator matrix of a symbol is the base-4 Pascal matrix of the GF(4)
// alphabet (Ahmed et al. 2025, eq. 24): the block at 2x2 block position
// (i, j) is symbol^(j-i) where C(j, i) is odd (Lucas: (~j & i) == 0), and
// empty otherwise. Each block is then pre-multiplied by pascal below, which
// nests a 2D Sobol sequence in the set (section 4.3): the identity symbol
// then reproduces the classic Sobol Pascal matrix, so dimensions 0 and 1 are
// unchanged. The factor is block diagonal, so the base-4 properties are
// untouched. Columns are stored as 32 bit words with the most significant bit
// as the first row.
constexpr std::array<std::uint32_t, size> buildMatrix(Block symbol)
{
	constexpr auto numBlocks = size / 2;
	constexpr Block pascal = {1, 1, 0, 1};

	Block powers[numBlocks] = {};
	powers[0] = {1, 0, 0, 1};
	for(int exponent = 1; exponent < numBlocks; ++exponent)
	{
		powers[exponent] = multiply(powers[exponent - 1], symbol);
	}

	std::array<std::uint32_t, size> matrix = {};
	for(int j = 0; j < numBlocks; ++j)
	{
		for(int i = 0; i <= j; ++i)
		{
			if((~j & i) != 0)
			{
				continue;
			}

			const Block block = multiply(pascal, powers[j - i]);
			const int entries[2][2] = {{block.a, block.b}, {block.c, block.d}};

			for(int row = 0; row < 2; ++row)
			{
				for(int column = 0; column < 2; ++column)
				{
					if(entries[row][column] != 0)
					{
						matrix[2 * j + column] |= 1u << (31 - (2 * i + row));
					}
				}
			}
		}
	}

	return matrix;
}

// Product of two generator matrices, column by column.
constexpr std::array<std::uint32_t, size>
multiplyMatrix(const std::array<std::uint32_t, size>& lhs,
               const std::array<std::uint32_t, size>& rhs)
{
	std::array<std::uint32_t, size> matrix = {};
	for(int column = 0; column < size; ++column)
	{
		for(int row = 0; row < size; ++row)
		{
			if((rhs[column] >> (31 - row) & 1) != 0)
			{
				matrix[column] ^= lhs[row];
			}
		}
	}

	return matrix;
}

// Dimension 0 is the identity, dimensions 1 and 2 are the matrices of the
// first two alphabet symbols, and dimension 3 follows from dimension 2 by
// the pair relation of eq. (13), P * SZ[d] == SZ[d ^ 1].
constexpr std::array<std::array<std::uint32_t, size>, numDimensions>
buildMatrices()
{
	std::array<std::array<std::uint32_t, size>, numDimensions> matrices = {};

	for(int i = 0; i < size; ++i)
	{
		matrices[0][i] = 1u << (31 - i);
	}

	matrices[1] = buildMatrix({1, 0, 0, 1});
	matrices[2] = buildMatrix(findAlpha());
	matrices[3] = multiplyMatrix(matrices[1], matrices[2]);

	return matrices;
}

constexpr auto matrices = buildMatrices();

void printMatrices(int dimensionSize, int indexSize)
{
	assert(dimensionSize >= 0);
	assert(indexSize >= 0);

	for(int i = 0; i < dimensionSize; ++i)
	{
		for(int j = 0; j < indexSize; ++j)
		{
			const auto indexRev = indexSize - (j + 1);
			const auto valueRev = oqmc::reverseBits32(matrices[i][indexRev]);

			if(j % 8 == 0)
			{
				std::printf(j == 0 ? "{" : " ");
			}

			if(j % 8 == 7)
			{
				std::printf(j == indexSize - 1 ? "0x%04x},\n" : "0x%04x,\n",
				            valueRev);
			}
			else
			{
				std::printf("0x%04x, ", valueRev);
			}
		}
	}

	std::printf("\n");
}

// In the bit reversed basis, the generator matrix is unit lower-triangular, so
// GF(2) elimination factors it into shift-mask-xor steps (Ahmed 2024, eq. 18),
// one per sub-diagonal. Reversed, the steps form the program for that
// dimension. Programs print twice: left shifts for the device, and the
// conjugate form with reversed masks and right shifts for the host.

constexpr int wordBits = 16;

std::uint16_t reversedRow(int dimension, int row)
{
	assert(dimension >= 0 && dimension < numDimensions);
	assert(row >= 0 && row < wordBits);

	std::uint16_t bits = 0;
	for(int column = 0; column < wordBits; ++column)
	{
		const auto reversedColumn = wordBits - 1 - column;
		const auto reversedRowIndex = wordBits - 1 - row;

		const auto value =
		    oqmc::reverseBits32(matrices[dimension][reversedColumn]);
		const auto reversed = std::bitset<wordBits>(value);
		if(reversed[reversedRowIndex])
		{
			bits |= static_cast<std::uint16_t>(1u << column);
		}
	}
	return bits;
}

void printPrograms(int dimensionSize, bool conjugate)
{
	assert(dimensionSize >= 0);

	for(int dimension = 0; dimension < dimensionSize; ++dimension)
	{
		std::array<std::uint16_t, wordBits> rows;
		for(int row = 0; row < wordBits; ++row)
		{
			rows[row] = reversedRow(dimension, row);
		}

		std::vector<std::pair<std::uint16_t, int>> program;
		for(int shift = 1; shift < wordBits; ++shift)
		{
			std::uint16_t mask = 0;
			for(int row = 0; row + shift < wordBits; ++row)
			{
				if(((rows[row + shift] >> row) & 1) != 0)
				{
					mask |= static_cast<std::uint16_t>(1u << row);
				}
			}
			if(mask == 0)
			{
				continue;
			}
			for(int row = 0; row + shift < wordBits; ++row)
			{
				if(((mask >> row) & 1) != 0)
				{
					rows[row + shift] ^= rows[row];
				}
			}
			program.emplace_back(mask, shift);
		}

		for(int row = 0; row < wordBits; ++row)
		{
			assert(rows[row] == static_cast<std::uint16_t>(1u << row));
		}

		std::printf("// dimension %d\n", dimension);
		for(auto step = program.rbegin(); step != program.rend(); ++step)
		{
			if(conjugate)
			{
				std::printf("index ^= static_cast<std::uint16_t>((index & "
				            "0x%04x) >> %d);\n",
				            oqmc::reverseBits16(step->first), step->second);
			}
			else
			{
				std::printf("index ^= static_cast<std::uint16_t>((index & "
				            "0x%04x) << %d);\n",
				            step->first, step->second);
			}
		}
		std::printf("\n");
	}
}

int main()
{
	constexpr auto dimensionSize = 4;
	constexpr auto indexSize = 16;

	printMatrices(dimensionSize, indexSize);
	printPrograms(dimensionSize, false);
	printPrograms(dimensionSize, true);

	return 0;
}
