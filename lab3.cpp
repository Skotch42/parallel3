#include <sstream>
#include <mpi.h>
#include <iostream>
#include <iomanip>
using namespace std;

class SHA256 {

public:
	SHA256();
	void update(uint8_t* data, size_t length);
	void update(string& s);
	uint8_t* digest();
	string hash(string& data)
	{
		update(data);
		uint8_t* digest = this->digest();
		string result = SHA256::toString(digest);
		delete[] digest;

		return result;
	}

	static string toString(uint8_t* digest);

private:
	uint8_t  m_data[64];
	uint32_t m_blocklen;
	uint64_t m_bitlen;
	uint32_t m_state[8]; //A, B, C, D, E, F, G, H

	static uint32_t K[64];
	static uint32_t rotr(uint32_t x, uint32_t n);
	static uint32_t choose(uint32_t e, uint32_t f, uint32_t g);
	static uint32_t majority(uint32_t a, uint32_t b, uint32_t c);
	static uint32_t sig0(uint32_t x);
	static uint32_t sig1(uint32_t x);
	void transform();
	void pad();
	void revert(uint8_t* hash);
};

uint32_t SHA256::K[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

SHA256::SHA256() : m_blocklen(0), m_bitlen(0) {
	m_state[0] = 0x6a09e667;
	m_state[1] = 0xbb67ae85;
	m_state[2] = 0x3c6ef372;
	m_state[3] = 0xa54ff53a;
	m_state[4] = 0x510e527f;
	m_state[5] = 0x9b05688c;
	m_state[6] = 0x1f83d9ab;
	m_state[7] = 0x5be0cd19;
}

void SHA256::update(uint8_t* data, size_t length) {
	for (size_t i = 0; i < length; i++) {
		m_data[m_blocklen++] = data[i];
		if (m_blocklen == 64) {
			transform();

			// End of the block
			m_bitlen += 512;
			m_blocklen = 0;
		}
	}
}

void SHA256::update(string& s) {
	string str = s;
	uint8_t* my_uint8_ptr = reinterpret_cast<uint8_t*>(const_cast<char*>(str.data()));
	update(my_uint8_ptr, s.size());
}

uint8_t* SHA256::digest() {
	uint8_t* hash = new uint8_t[32];

	pad();
	revert(hash);

	return hash;
}

uint32_t SHA256::rotr(uint32_t x, uint32_t n) {
	return (x >> n) | (x << (32 - n));
}

uint32_t SHA256::choose(uint32_t e, uint32_t f, uint32_t g) {
	return (e & f) ^ (~e & g);
}

uint32_t SHA256::majority(uint32_t a, uint32_t b, uint32_t c) {
	return (a & (b | c)) | (b & c);
}

uint32_t SHA256::sig0(uint32_t x) {
	return SHA256::rotr(x, 7) ^ SHA256::rotr(x, 18) ^ (x >> 3);
}

uint32_t SHA256::sig1(uint32_t x) {
	return SHA256::rotr(x, 17) ^ SHA256::rotr(x, 19) ^ (x >> 10);
}

void SHA256::transform() {
	uint32_t maj, xorA, ch, xorE, sum, newA, newE, m[64];
	uint32_t state[8];

	for (uint8_t i = 0, j = 0; i < 16; i++, j += 4) { // Split data in 32 bit blocks for the 16 first words
		m[i] = (m_data[j] << 24) | (m_data[j + 1] << 16) | (m_data[j + 2] << 8) | (m_data[j + 3]);
	}

	for (uint8_t k = 16; k < 64; k++) { // Remaining 48 blocks
		m[k] = SHA256::sig1(m[k - 2]) + m[k - 7] + SHA256::sig0(m[k - 15]) + m[k - 16];
	}

	for (uint8_t i = 0; i < 8; i++) {
		state[i] = m_state[i];
	}

	for (uint8_t i = 0; i < 64; i++) {
		maj = SHA256::majority(state[0], state[1], state[2]);
		xorA = SHA256::rotr(state[0], 2) ^ SHA256::rotr(state[0], 13) ^ SHA256::rotr(state[0], 22);

		ch = choose(state[4], state[5], state[6]);

		xorE = SHA256::rotr(state[4], 6) ^ SHA256::rotr(state[4], 11) ^ SHA256::rotr(state[4], 25);

		sum = m[i] + K[i] + state[7] + ch + xorE;
		newA = xorA + maj + sum;
		newE = state[3] + sum;

		state[7] = state[6];
		state[6] = state[5];
		state[5] = state[4];
		state[4] = newE;
		state[3] = state[2];
		state[2] = state[1];
		state[1] = state[0];
		state[0] = newA;
	}

	for (uint8_t i = 0; i < 8; i++) {
		m_state[i] += state[i];
	}
}

void SHA256::pad() {

	uint64_t i = m_blocklen;
	uint8_t end = m_blocklen < 56 ? 56 : 64;

	m_data[i++] = 0x80; // Append a bit 1
	while (i < end) {
		m_data[i++] = 0x00; // Pad with zeros
	}

	if (m_blocklen >= 56) {
		transform();
		memset(m_data, 0, 56);
	}

	// Append to the padding the total message's length in bits and transform.
	m_bitlen += m_blocklen * 8;
	m_data[63] = m_bitlen;
	m_data[62] = m_bitlen >> 8;
	m_data[61] = m_bitlen >> 16;
	m_data[60] = m_bitlen >> 24;
	m_data[59] = m_bitlen >> 32;
	m_data[58] = m_bitlen >> 40;
	m_data[57] = m_bitlen >> 48;
	m_data[56] = m_bitlen >> 56;
	transform();
}

void SHA256::revert(uint8_t* hash) {
	// SHA uses big endian byte ordering
	// Revert all bytes
	for (uint8_t i = 0; i < 4; i++) {
		for (uint8_t j = 0; j < 8; j++) {
			hash[i + (j * 4)] = (m_state[j] >> (24 - i * 8)) & 0x000000ff;
		}
	}
}

string SHA256::toString(uint8_t* digest) {
	stringstream s;
	s << setfill('0') << hex;

	for (uint8_t i = 0; i < 32; i++) {
		s << setw(2) << (unsigned int)digest[i];
	}

	return s.str();
}

const int K = 3;

bool isValid(string hash) {
	for (int i = 0; i < K; i++) {
		if (hash.at(i) != '0') return false;
	}

	return true;
}

string toString(unsigned int v) {
	stringstream ss;
	ss << v;
	return ss.str();
}

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);

	int size, rank;
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	string resultString, input;
	if (rank == 0)
	{
		cout << "Difficulty : " << K << endl;
		cout << "Enter a string: ";
		cin >> input;
	}

	int lineSize = input.size();
	MPI_Bcast(&lineSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank != 0) 
	{
		input.resize(lineSize);
	}

	MPI_Bcast(const_cast<char*>(input.data()), lineSize, MPI_CHAR, 0, MPI_COMM_WORLD);

	unsigned int nonce = 0;
	double startTime = MPI_Wtime();

	while (rank != 0)
	{
		int flag = 0;

		resultString = input + toString(nonce);
		SHA256 sha;

		if (isValid(sha.hash(resultString)))
		{
			MPI_Send(&nonce, 1, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD);
			flag = 1;
		}


		if (flag)
		{
			break;
		}

		++nonce;
	}

	if (rank == 0)
	{
		string hash;
		bool found = false;
		while (!found)
		{
			MPI_Recv(&nonce, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			string resultString = input + toString(nonce);
			SHA256 sha;
			hash = sha.hash(resultString);
			found = isValid(hash);

			if (found)
			{
				cout << "Operation took : " << MPI_Wtime() - startTime << "s" << endl;

				for (int i = 1; i < size; ++i)
				{
					MPI_Send(0, 0, MPI_INT, i, 0, MPI_COMM_WORLD);
				}

				cout << "Found hash(" << input << " + " << nonce << ") = " << hash << endl;
			}
		}
	}

	MPI_Finalize();

	return EXIT_SUCCESS;
}
