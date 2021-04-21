/**
 * Run a benchmark of EC-ElGamal ciphertext encryption / decryption.
 */

#include <stdio.h>
#include <string.h>
#include <string>

#include "epir.hpp"
#include "common.h"

#define LOOP (10 * 1000)

int main(int argc, char *argv[]) {
	
	// Generate messages to encrypt.
	printf("Generatig messages to encrypt...\n");
	std::vector<uint64_t> msg(LOOP);
	for(size_t i=0; i<LOOP; i++) {
		msg[i] = rand() & (EPIR_MG_MAX - 1);
	}
	
	// Create key pair.
	printf("Generatig a key pair...\n");
	const EllipticPIR::PrivKey privkey;
	const EllipticPIR::PubKey pubkey(privkey);
	
	// Load mG.bin.
	printf("Loading mG.bin...\n");
	PRINT_MEASUREMENT(true, "mG.bin loaded in %.0fms.\n",
		EllipticPIR::DecryptionContext decCtx(EPIR_MG_MAX, EPIR_MG_PATH);
	);
	
	std::vector<EllipticPIR::Cipher> ciphers;
	PRINT_MEASUREMENT(true, "Ciphertext encrypted in %.0fms.\n",
		for(size_t i=0; i<LOOP; i++) {
			ciphers.push_back(EllipticPIR::Cipher(privkey, msg[i]));
		}
	);
	
	PRINT_MEASUREMENT(true, "Ciphertext decrypted in %.0fms.\n",
		for(size_t i=0; i<LOOP; i++) {
			int32_t decrypted = decCtx.decryptCipher(privkey, ciphers[i]);
			if(decrypted != msg[i]) {
				printf("Decryption error occured! (i=%zd, msg=%ld, decrypted=%d)\n", i, msg[i], decrypted);
			}
		}
	);
	
	return 0;
	
}

