#include "test-rbuf.h"
#include "test-request.h"
#include "test-response.h"
#include "test-wbuf.h"

int main()
{
    test_rbuf();
    test_wbuf();
    test_request();
    test_response();

    return 0;
}
