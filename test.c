#include "test-readbuf.h"
#include "test-request.h"
#include "test-response.h"
#include "test-writebuf.h"

int main()
{
    test_readbuf();
    test_request();
    test_response();
    test_writebuf();

    return 0;
}