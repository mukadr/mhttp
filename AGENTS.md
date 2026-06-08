# AGENTS.md — mhttp

## Build & test

```
make              # build test binary (gcc, C99, -O2 -Wall)
make check        # build + run tests
make CC=clang     # build with clang
make clean        # rm *.o *.d test
make sanitize     # clean + build with -fsanitize=address,undefined + run tests
make leaks        # clean + build + run under macOS leaks tool
```

CI builds and tests with both gcc and clang, plus sanitize steps for both compilers (including `ASAN_OPTIONS=detect_leaks=1` on Linux).

## Testing philosophy

Tests live in `test-<module>.c` / `test-<module>.h` pairs. `test.c` contains `main()` and calls each module's public test entrypoint.

- **No external test framework.** Only `assert()` from `<assert.h>`.
- Each test file exports a single `void test_<module>(void)` function.
- Internal test cases are `static void` helpers called from the public entrypoint.
- Tests follow the pattern: **create → exercise → assert → free**. Always clean up.
- Tests cover all code paths: success, edge cases (bounds, lengths, zero/null), error returns, and incremental/streaming input.
- When adding new source modules, create a matching `test-<module>.c/h`, add a call in `test.c:main()`, and add `test-<module>.o` to the `OBJS` list in the Makefile.

## Coding conventions

- **C99** (`-std=c99`). No C11/C17 features.
- Include guard: `MHTTP_<MODULE>_H`
- Types: `HttpXxx` (e.g. `HttpBuffer`, `HttpSlice`, `HttpRequest`)
- Functions: `http_<module>_<action>` (e.g. `http_buffer_concat`, `http_request_parse`)
- Enum values: `HTTP_<UPPER>` (e.g. `HTTP_OK`, `HTTP_BAD_REQUEST`)
- Public API in `.h`, static helpers in `.c`.
- Allocate with `calloc`, free with `free`. Check allocation results.
- No comments unless the line is genuinely obscure.
- Always open braces — `break`, `continue`, and `return` must stand on their own line inside braces.
- Include order: own module header first, then standard library headers.

## Architecture

Single-header HTTP parsing library — no external dependencies (libc only).

| File | Purpose |
|------|---------|
| `buffer.c/h` | Fixed-size ring buffer for incremental data feeding |
| `slice.c/h` | String view with `begin`/`end` pointers; most functions inline in header |
| `request.c/h` | Streaming HTTP request parser (method, URI, version, headers) |
| `response.c/h` | HTTP response builder (status code, reason phrase, headers, body) |
| `http.h` | Shared enums (`HttpResult`, `HttpMethod`, `HttpStatusCode`) and `HttpHeader` struct |

## Important constraints

- `http_buffer_new(0)` returns **NULL** — zero-size buffers are rejected.
- `http_buffer_concat` may write fewer bytes than the input string length if the buffer is full. Always check the return value.
- HTTP parsing is **incremental**: feed chunks via `http_buffer_concat`, call `http_request_parse` after each chunk, handle `HTTP_NEED_MORE_INPUT` to request more data.
- Header count is **cumulative** across multiple `http_request_parse` calls and must not exceed 100.
- Max URI length: 255 chars. Max header name/value length: 255 chars each.
- Line endings must be `\r\n`; bare `\n` is rejected.
- Null bytes in URI, header names, or header values trigger `HTTP_BAD_REQUEST`.
- The `test` binary aborts on first assertion failure — fix one test at a time.

## Response builder

- `http_response_set_header` returns `HttpResult` — `HTTP_OK` on success, `HTTP_INTERNAL_ERROR` if allocation fails. Always check the return value.
- `strncpy` calls must be followed by an explicit `buf[sizeof(buf) - 1] = '\0'` to guarantee null termination.
- `http_response_set_body` auto-sets the `Content-Length` header via `set_header`, so it may fail if allocation is exhausted.
- `http_response_set_body(NULL)` is valid — clears the body and sets `body_length` to 0.
- `http_response_set_status` looks up the reason phrase from a static table; unknown status codes get `"Unknown"`.
- `http_response_write` serializes into the buffer via sequential `http_buffer_concat` calls. If the buffer fills up mid-write, the response is truncated.
