#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define COMPRESION_ROUNDS 2
#define FINALIZATION_ROUNDS 4



uint64_t rotation_shift(const uint64_t value, int shift);
void sipround(uint64_t v[4]);
uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen);


/*  #### A VERIFIER QUAND MEME #####
* Fonction rotation_shift :
* Prend en arguments
*       - un uint_t64 value representant la valeur sur la quel effectuer la rotation
*       - un int shift      representant le nombre de rotation a effectuer
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
    v[3] = rotation_shift(v[1], 21);

    v[1] ^= v[2];
    v[3] ^= v[0];

    v[2] = rotation_shift(v[2], 32);
}


/*
* Procedure siphash_2_4 :
*
*/
uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen){
    uint64_t v[4];
    // Init
    v[0] = k[0] ^ 0x736f6d6570736575;
    v[1] = k[1] ^ 0x646f72616e646f6d;
    v[2] = k[0] ^ 0x6c7967656e657261;
    v[3] = k[1] ^ 0x7465646279746573;

    // Compress
    size_t len = strlen(m);
    size_t w = (len + 1) / 8;
    //uint64_t

    for (int i = 0; i < COMPRESION_ROUNDS; i++)
        sipround(v);

    // Finalize

    v[2] ^= 0xff;
    for (int i = 0; i < FINALIZATION_ROUNDS; i++)
        sipround(v);

    return v[0] ^ v[1] ^ v[2] ^ v[3];
}


int main(int argc, char** argv){

}
