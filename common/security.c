/* 
 * Author: Aananth C N
 * email: c.n.aananth@gmail.com
 *
 * License: MPL v2
 * Date: 21 Jan 2016
 */
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "unixcommon.h"

#include <openssl/sha.h>



int sha256_file(char *file, char *outb)
{
	int i;
	FILE *fp;
	unsigned char *buffer;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256_ctx;

	const int bufSize = 32768;
	int bytesRead = 0;

	if(outb == NULL) {
		print("%s(): Invalid buffer passed as 2nd arg!\n", __func__);
		return -1;
	}

	fp = fopen(file, "rb");
	if(!fp) {
		print("%s(): Can't open file %s\n", __func__, file);
		return -1;
	}

	buffer = malloc(bufSize);
	if(!buffer) {
		print("%s(): malloc failed!\n", __func__);
		return ENOMEM;
	}

	SHA256_Init(&sha256_ctx);

	/* do a binary read one byte at a time and hash it */
	while((bytesRead = fread(buffer, 1, bufSize, fp))) {
		SHA256_Update(&sha256_ctx, buffer, bytesRead);
	}

	/* place the message digest to the buffer */
	SHA256_Final(hash, &sha256_ctx);

	/* convert the digest to hash string */
	for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		sprintf(outb + (i * 2), "%02x",	hash[i]);
	}


	fclose(fp);
	free(buffer);

	return 0;
}
