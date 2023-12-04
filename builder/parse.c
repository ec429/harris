#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "parse.h"

int for_each_line(int fd, int (*cb)(const char *line, void *data), void *data)
{
	size_t len, from = 0, to;
	int count = 0, rc;
	char line[256];
	ssize_t bytes;

	do {
		bytes = read(fd, line + from, sizeof(line) - from - 1);
		if (bytes < 0) {
			rc = -errno;
			goto out;
		}
		if (!bytes) {
			rc = from ? -EIO : count;
			goto out;
		}
		line[from + bytes] = 0;
		to = 0;
		do {
			for (len = to; line[len]; len++)
				if (line[len] == '\n')
					break;
			if (!line[len]) {
				if (to)
					break;
				rc = -EIO;
				goto out;
			}
			line[len] = 0;
			rc = cb(line + to, data);
			if (rc) {
				/* Reposition to un-consume subsequent lines */
				lseek(fd, len + 1 - from - bytes, SEEK_CUR);
				goto out;
			}
			count++;
			to = len + 1;
		} while (1);
		memmove(line, line + to, from + bytes + 1 - to);
		from = from + bytes - to;
	} while (1);
out:
	return rc;
}

int for_each_word(const char *line,
		  int (*cb)(const char *key, const char *value, void *data),
		  void *data)
{
	char word[256];
	char *equals;
	size_t len;
	bool end;
	int rc;

	do {
		len = strcspn(line, ":");
		if (len >= sizeof(word)) // can't happen
			return -EIO;
		memcpy(word, line, len);
		end = !line[len];
		word[len] = 0;
		equals = strchr(word, '=');
		if (equals)
			*equals++ = 0;
		rc = cb(word, equals, data);
		if (rc)
			return rc;
		line += len + 1;
	} while(!end);
	return 0;
}
