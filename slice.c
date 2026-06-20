#include <stdlib.h>
#include <string.h>

#include "slice.h"

bool mhttp_slice_match(struct mhttp_slice *slice, const char *str)
{
	size_t len;
       
	len = strlen(str);
	if (mhttp_slice_len(slice) < len)
		return false;

	if (memcmp(slice->begin, str, len))
		return false;

	slice->begin += len;
	return true;
}

bool mhttp_slice_eq(const struct mhttp_slice *slice, const char *str)
{
	size_t len;
       
	len = strlen(str);
	if (mhttp_slice_len(slice) != len)
		return false;

	return !memcmp(slice->begin, str, len);
}

bool mhttp_slice_eol(const struct mhttp_slice *slice)
{
	return mhttp_slice_len(slice) == 2 && slice->begin[0] == '\r' && slice->begin[1] == '\n';
}
