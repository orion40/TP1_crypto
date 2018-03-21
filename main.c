#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <endian.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#define COMPRESION_ROUNDS 2
#define FINALIZATION_ROUNDS 4

void print_internal_state(uint64_t* v);
uint64_t coll_search(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t));
uint64_t rotation_shift(const uint64_t value, int shift);
uint32_t sip_hash_fix32(uint32_t k, uint32_t m);
void sipround(uint64_t v[4]);
uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen);

void question1();
void question3();



/*
 * Procedure print_internal_state :
 * Affiche proprement un uint64
 * @ARG
 *       - un uint_t64 value representant la valeur sur la quel
 *          effectuer la rotation
 */
void print_internal_state(uint64_t* v){
    printf("0x%" PRIx64 "   ", v[0]);
    printf("0x%" PRIx64 "   ", v[1]);
    printf("0x%" PRIx64 "   ", v[2]);
    printf("0x%" PRIx64 "\n", v[3]);
}

/*
 * Fonction rotation_shift :
 * Effectue une rotation d'un chiffre binaire en réinjectant le
 *  bit de poid fort en bit de poid faible (rotation left)
 *  @ARG
 *      - un uint_t64 value representant la valeur sur la quel effectuer la rotation
 *      - un int shift      representant le nombre de rotation a effectuer
 *  @RETURN
 *      -uint64_t rotate on left
 */
uint64_t rotation_shift(const uint64_t value, int shift){
    if ((shift &= sizeof(value)*8 - 1) == 0)
        return value;
    return (value << shift) | (value >> (sizeof(value)*8 - shift));
}


/*
 * Procedure sipround :
 *
 */
void sipround(uint64_t v[4]){
    v[0] += v[1];
    v[2] += v[3];

    v[1] = rotation_shift(v[1], 13);
    v[3] = rotation_shift(v[3], 16);

    v[1] ^= v[0];
    v[3] ^= v[2];

    v[0] = rotation_shift(v[0], 32);

    v[2] += v[1];
    v[0] += v[3];

    v[1] = rotation_shift(v[1], 17);
    v[3] = rotation_shift(v[3], 21);

    v[1] ^= v[2];
    v[3] ^= v[0];

    v[2] = rotation_shift(v[2], 32);
}


/*
 * Procedure siphash_2_4 :
 *
 */
uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen){
    k[0] = htole64(k[0]);
    k[1] = htole64(k[1]);
//    printf("Input key is now:\n");
//    printf("0x%" PRIx64 "   ", k[0]);
//    printf("0x%" PRIx64 "\n", k[1]);
    uint64_t v[4];
    // Init
    v[0] = k[0] ^ 0x736f6d6570736575;
    v[1] = k[1] ^ 0x646f72616e646f6d;
    v[2] = k[0] ^ 0x6c7967656e657261;
    v[3] = k[1] ^ 0x7465646279746573;

//    printf("Initial state:\n");
//    print_internal_state(v);
//
//    printf("m value:\n");
//    for (int i = 0; i < mlen; i++){
//        printf("%02hhu  ", m[i]);
//        if (i !=0 && (i + 1) % 8 == 0) printf("\n");
//    }

    ////////////// PARSING WORD ////////////

    size_t w = ceil((mlen + 1) / 8);
    unsigned padding = 0;
    if (mlen % 8 == 0) padding++;
//    printf("w = %ld\n", w);
    uint64_t mi[w+padding];
    memset(mi, 0, (w + 1) * sizeof(uint64_t));

    int i;
    for (i = 0; i < w; i++){
        memcpy(mi+i, m+(i*8), 8);
    }

//    for (int i = 0; i < w + padding; i++){
//        printf("mi[%d] = %#018" PRIx64 "\n", i, mi[i]);
//    }

    // Fin du mot

    if (padding){
//        printf("Adding 00's and length to end\n");
        mi[w] ^= (uint64_t)(mlen % 256) << 56;
    } else {
//        printf("Adding length to end\n");
        mi[w-1] ^= (uint64_t)(mlen % 256) << 56;
    }
//    printf("End word: %#018" PRIx64 "\n", mi[w-1]);
//    for (int i = 0; i < w + padding; i++){
//        printf("mi[%d] = %#018" PRIx64 "\n", i, mi[i]);
//    }

    /////////////////////////////////////////////////
    // Compresion rounds

    for (i = 0; i < w + padding; i++){
        v[3] ^= mi[i];
//        print_internal_state(v);
        int j;
        for (j = 0; j < COMPRESION_ROUNDS; j++)
            sipround(v);
//        printf("After %d rounds of sipround:\n", COMPRESION_ROUNDS);
//        print_internal_state(v);
        v[0] ^= mi[i];
//        printf("XORing v[0] to mi[i]:\n");
//        print_internal_state(v);
//        printf("\n");
    }

    // Finalize

    v[2] ^= 0xff;
//    printf("After XORing 0xff\n");
//    print_internal_state(v);
    for (i = 0; i < FINALIZATION_ROUNDS; i++)
        sipround(v);
    uint64_t result = v[0] ^ v[1] ^ v[2] ^ v[3];

//    printf("And the result is: 0x%" PRIx64 "\n", result);

    return result;
}

uint32_t sip_hash_fix32(uint32_t k, uint32_t m){
    uint64_t k64[2];
    memset(k64, 0, 2 * sizeof(uint64_t));
    uint8_t m8[4];
    memset(m8, 0, 2 * sizeof(uint8_t));

    // Putting k 4 times into k64
    int i;
    for (i = 0; i < 4; i++){
        k64[i/2] ^= (uint64_t)k << 32 *(i % 2);
    }

    return siphash_2_4(k64, m8, 4) >> 32;
}

void question1(){
    // TODO: peut-etre un problème avec little/big endian
    uint64_t k[2] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    uint64_t k2[2] = {0, 0};
    uint8_t m[15] = {
        0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
        0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe
    };
    uint8_t m2[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    uint64_t result;

    printf("===== Test 1 ====\n");
    result = siphash_2_4(k, m, 15);
    assert(result == 0xa129ca6149be45e5);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 2 ====\n");
    result = siphash_2_4(k, m2, 8);
    assert(result == 0x93f5f5799a932462);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 3 ====\n");
    result = siphash_2_4(k, NULL, 0);
    assert(result == 0x726fdb47dd0e0e31);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 4 ====\n");
    result = siphash_2_4(k2, NULL, 0);
    assert(result == 0x1e924b9d737700d7);
    printf("OK - 0x%" PRIx64 "\n", result);
}

void question3(){
    uint32_t k, m, result;
    printf("SipHash32\n");
    printf("===== Test 1 ====\n");
    k = 0;
    m = 0;
    result = sip_hash_fix32(k, m);
    printf("OK - 0x%" PRIx32 "\n", result);

    printf("===== Test 2 ====\n");
    k = 0x03020100;
    m = 0x03020100;
    result = sip_hash_fix32(k, m);
    printf("OK - 0x%" PRIx32 "\n", result);
}

uint64_t coll_search(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t)){
    uint32_t result1, result2;

    for (uint32_t i = 0; i < UINT32_MAX; i++){

    }

    return 0;
}

int main(int argc, char** argv){
    question1();
    question3();

    return 1;
}
