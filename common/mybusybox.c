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


#include <libtar.h>
#include <fcntl.h>

int untar_file(const char *file, char *path)
{
        TAR *ptar;
        int ret = 0;

        if((file == NULL) || (path == NULL)) {
                print("%s(), invalid inputs!\n", __func__);
                return -1;
        }

        if(0 != tar_open(&ptar, file, NULL, O_RDONLY, 0, 0)) {
		print("tar_open(): %s\n", strerror(errno));
                ret = -1;
        }
        if(0 != tar_extract_all(ptar, path)) {
		print("tar_extract_all(): %s\n", strerror(errno));
                ret = -1;
        }
        if(0 != tar_close(ptar)) {
		print("tar_close(): %s\n", strerror(errno));
		ret = -1;
	}

	if(ret == -1) {
		print("%s(): input file: %s, output dir: %s\n",
		       __func__, file, path);
	}

        return ret;
}


#include <bzlib.h>
int decompress_bzip2(const char *file, char *path)
{
	int ret = 0;
	int err, cntr, cntw;
	BZFILE *bzf;
	FILE *fr, *fw;
	char buf[4096];
	char out[256];

	/* open compressed input file */
	fr = fopen(file, "rb");
	if(fr == NULL) {
		print("%s(): Unable to open %s\n", __func__, file);
		return -1;
	}

	/* prepare for output stream */
	strcpy(out, path);
	strcat(out, "/");
	strcat(out, get_filename((char*)file));
	out[strlen(out)-4] = '\0';   // eliminate .bz2
	fw = fopen(out, "wb");
	if(fw == NULL) {
		print("%s(): Unable to open %s\n", __func__, out);
		ret = -1;
		goto exit1;
	}

	/* prepare the input compressed stream */
	fseek(fr, 0, SEEK_SET);
	bzf = BZ2_bzReadOpen(&err, fr, 0, 0, NULL, 0);
	if(err != BZ_OK) {
		print("Error: BZ2_bzReadOpen: %d\n", err);
		ret = -1;
		goto exit2;
	}

	/* decompress and write chunk by chunk */
	while(err == BZ_OK) {
		cntr = BZ2_bzRead(&err, bzf, buf, sizeof buf);
		if((err == BZ_OK) || (err == BZ_STREAM_END)) {
			cntw = fwrite(buf, 1, cntr, fw);
			if(cntr != cntw) {
				print("Error: %s(): read & write mismatch\n",
				      __func__);
				ret = -1;
				break;
			}
		}
	}

	BZ2_bzReadClose(&err, bzf);
exit2:
	fclose(fw);
exit1:
	fclose(fr);
	return ret;
}
