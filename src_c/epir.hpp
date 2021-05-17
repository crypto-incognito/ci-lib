/**
 * Crypto Incognito C++ wrapper library.
 */

#ifndef EPIR_HPP
#define EPIR_HPP

#include <string.h>
#include <vector>
#include <array>
#include <string>
#include <algorithm>

#include "epir.h"

namespace EllipticPIR {
	
	typedef std::array<unsigned char, EPIR_CIPHER_SIZE> Cipher;
	
	class Scalar {
		
		public:
			
			unsigned char bytes[EPIR_SCALAR_SIZE];
			
			/**
			 * Create a new Scalar instance with a random scalar.
			 */
			Scalar() {
				epir_create_privkey(this->bytes);
			}
			
			/**
			 * Create a new Scalar instance with a given buffer.
			 */
			Scalar(const unsigned char *buf) {
				memcpy(this->bytes, buf, EPIR_SCALAR_SIZE);
			}
			
			Scalar(const std::array<unsigned char, EPIR_SCALAR_SIZE> buf) {
				memcpy(this->bytes, buf.data(), EPIR_SCALAR_SIZE);
			}
		
	};
	
	class Encryptor {
		
		public:
			
			/**
			 * Encrypt a message using a given randomness (if given).
			 */
			virtual Cipher encrypt(
				const uint64_t message, const unsigned char *r = NULL) const = 0;
			
			Cipher encrypt(
				const uint64_t message, const Scalar r) const {
				return this->encrypt(message, r.bytes);
			}
			
			Cipher encrypt(
				const uint64_t message, const std::array<unsigned char, EPIR_SCALAR_SIZE> r) const {
				return this->encrypt(message, r.data());
			}
			
			/**
			 * Compute a number of ciphers in a selector.
			 */
			static uint64_t ciphersCount(std::vector<uint64_t> indexCounts) {
				return epir_selector_ciphers_count(indexCounts.data(), indexCounts.size());
			}
			
			/**
			 * Compute a number of elements in a selector.
			 */
			static uint64_t elementsCount(std::vector<uint64_t> indexCounts) {
				return epir_selector_elements_count(indexCounts.data(), indexCounts.size());
			}
			
			virtual std::vector<unsigned char> createSelector(
				const std::vector<uint64_t> &indexCounts, const uint64_t idx, const unsigned char *r = NULL) const = 0;
		
	};
	
	class PrivateKey: public Scalar, Encryptor {
		
		public:
			
			Cipher encrypt(const uint64_t message, const unsigned char *r = NULL) const {
				Cipher cipher;
				epir_ecelgamal_encrypt_fast(cipher.data(), this->bytes, message, r);
				return cipher;
			}
			
			std::vector<unsigned char> createSelector(
				const std::vector<uint64_t> &indexCounts, const uint64_t idx, const unsigned char *r = NULL) const {
				std::vector<unsigned char> selector(Encryptor::ciphersCount(indexCounts));
				epir_selector_create_fast(
					selector.data(), this->bytes,
					indexCounts.data(), indexCounts.size(), idx, r);
				return selector;
			}
		
	};
	
	class PublicKey: Encryptor {
		
		public:
			
			unsigned char bytes[EPIR_POINT_SIZE];
			
			/**
			 * Create a new PublicKey instance using a PrivateKey.
			 */
			PublicKey(const PrivateKey &privkey) {
				epir_pubkey_from_privkey(this->bytes, privkey.bytes);
			}
			
			/**
			 * Create a new PublicKey instance with a given buffer.
			 */
			PublicKey(const unsigned char *buf) {
				memcpy(this->bytes, buf, EPIR_POINT_SIZE);
			}
			
			PublicKey(const std::array<unsigned char, EPIR_POINT_SIZE> buf) {
				memcpy(this->bytes, buf.data(), EPIR_POINT_SIZE);
			}
			
			Cipher encrypt(const uint64_t message, const unsigned char *r = NULL) const {
				Cipher cipher;
				epir_ecelgamal_encrypt(cipher.data(), this->bytes, message, r);
				return cipher;
			}
			
			std::vector<unsigned char> createSelector(
				const std::vector<uint64_t> &indexCounts, const uint64_t idx, const unsigned char *r = NULL) const {
				std::vector<unsigned char> selector(Encryptor::ciphersCount(indexCounts));
				epir_selector_create(
					selector.data(), this->bytes,
					indexCounts.data(), indexCounts.size(), idx, r);
				return selector;
			}
		
	};
	
	class DecryptionContext {
		
	public:
		
		const size_t mmax;
		std::vector<epir_mG_t> mG;
		
		/**
		 * Load mG.bin to create a new DecryptionContext instanct.
		 */
		DecryptionContext(const std::string path = "", const size_t mmax = EPIR_DEFAULT_MG_MAX):
			mmax(mmax), mG(mmax) {
			size_t elemsRead = epir_mG_load(this->mG.data(), mmax, (path == "" ? NULL : path.c_str()));
			if(elemsRead != mmax) throw "Failed to load mG.bin.";
		}
		
		/**
		 * Generate mG.bin.
		 */
		DecryptionContext(
			void (*cb)(const size_t, void*) = NULL, void *cbData = NULL, const size_t mmax = EPIR_DEFAULT_MG_MAX): mmax(mmax), mG(mmax) {
			epir_mG_generate_no_sort(this->mG.data(), this->mmax, cb, cbData);
			std::sort(mG.begin(), mG.end(), [](const epir_mG_t &a, const epir_mG_t &b) {
				return memcmp(a.point, b.point, EPIR_POINT_SIZE) < 0;
			});
		}
		
		int32_t decryptCipher(const PrivateKey &privkey, const unsigned char *cipher) const {
			return epir_ecelgamal_decrypt(privkey.bytes, cipher, this->mG.data(), this->mmax);
		}
		
		int32_t decryptCipher(const PrivateKey &privkey, const Cipher &cipher) const {
			return this->decryptCipher(privkey, cipher.data());
		}
		
		std::vector<unsigned char> decryptReply(
			const PrivateKey &privkey, const std::vector<unsigned char> &reply,
			const uint8_t dimension, const uint8_t packing) const {
			std::vector<unsigned char> buf(reply.size());
			memcpy(buf.data(), reply.data(), reply.size());
			int decryptedCount = epir_reply_decrypt(
				buf.data(), reply.size(), privkey.bytes, dimension, packing, this->mG.data(), this->mmax);
			if(decryptedCount < 0) throw "Failed to decrypt.";
			std::vector<unsigned char> ret(decryptedCount);
			memcpy(ret.data(), buf.data(), decryptedCount);
			return ret;
		}
		
	};
	
}

#endif

