
#define B_LAST_INDEX 	15
#define BLOCK_SIZE 		16

int existPadding(unsigned char *block);

void aes_cbc_encript(const unsigned char *data, int len, unsigned char*key,unsigned char *iv,unsigned char *kiv,unsigned char *k1,unsigned char *k2,unsigned char *mac);

void aes_cbc_decript(unsigned char *data, int len, unsigned char *key,unsigned char *iv,unsigned char *k1);
