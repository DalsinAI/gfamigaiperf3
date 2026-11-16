/* PC unit test for amiga/gfinet.c: its inet_pton, inet_ntop and
 * getaddrinfo against the C library's own, case by case.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* the C library's, taken before gfinet.h renames them */
static int c_pton(int af, const char *s, void *d) { return inet_pton(af, s, d); }
static const char *c_ntop(int af, const void *s, char *d, socklen_t n) { return inet_ntop(af, s, d, n); }

#include "gfinet.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void same4(const char *s)
{
	unsigned char a[4] = {0}, b[4] = {0};
	int ra = inet_pton(AF_INET, s, a), rb = c_pton(AF_INET, s, b);
	if (ra != rb || (ra == 1 && memcmp(a, b, 4))) { printf("FAIL pton4 \"%s\": ours %d, C library %d\n", s, ra, rb); fails++; }
}

static void same6(const char *s)
{
	unsigned char a[16] = {0}, b[16] = {0};
	char ta[64], tb[64];
	int ra = inet_pton(AF_INET6, s, a), rb = c_pton(AF_INET6, s, b);
	if (ra != rb || (ra == 1 && memcmp(a, b, 16))) { printf("FAIL pton6 \"%s\": ours %d, C library %d\n", s, ra, rb); fails++; return; }
	if (rb == 1) {
		inet_ntop(AF_INET6, b, ta, sizeof ta);
		c_ntop(AF_INET6, b, tb, sizeof tb);
		if (strcmp(ta, tb)) { printf("FAIL ntop6 \"%s\": ours %s, C library %s\n", s, ta, tb); fails++; }
	}
}

int main(void)
{
	static const char *v4[] = { "0.0.0.0", "127.0.0.1", "255.255.255.255", "192.168.1.10", "1.2.3", "1.2.3.4.5",
		"256.1.1.1", "01.2.3.4", "1..2.3", "", "a.b.c.d", "1.2.3.4 ", "10.0.0.1x", NULL };
	static const char *v6[] = { "::", "::1", "1::", "fe80::1", "2001:db8::8a2e:370:7334", "2001:0db8:0000:0000:0000:0000:0000:0001",
		"1:2:3:4:5:6:7:8", "1:0:0:2:0:0:0:3", "1:0:0:0:2:0:0:3", "::ffff:192.168.1.1", "::192.168.1.1",
		"1:2:3:4:5:6:1.2.3.4", ":::", "1:::2", "1:2", "12345::", "::g", "1:2:3:4:5:6:7:8:9", "1::2::3", ":1::",
		"1:", "fe80::1%eth0", "0:0:0:0:0:0:0:0", "a:b:c:d:e:f:0:0", NULL };
	char buf[64];
	unsigned char ip[4] = { 10, 1, 2, 3 }, ll[16];
	struct addrinfo hints, *ai;
	struct sockaddr_in *sin;
	int i;

	for (i = 0; v4[i]; i++) same4(v4[i]);
	for (i = 0; v6[i]; i++) same6(v6[i]);
	CHECK(strcmp(inet_ntop(AF_INET, ip, buf, sizeof buf), "10.1.2.3") == 0);
	CHECK(inet_ntop(AF_INET, ip, buf, 8) == NULL);   /* too small */
	CHECK(inet_pton(AF_INET6, "fe80::1", ll) == 1 && IN6_IS_ADDR_LINKLOCAL(ll));
	CHECK(inet_pton(AF_INET6, "2001:db8::1", ll) == 1 && !IN6_IS_ADDR_LINKLOCAL(ll));

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	CHECK(getaddrinfo("127.0.0.1", "5201", &hints, &ai) == 0);
	sin = (struct sockaddr_in *)ai->ai_addr;
	CHECK(ai->ai_family == AF_INET && ai->ai_socktype == SOCK_STREAM && ai->ai_addrlen == sizeof *sin);
	CHECK(ntohs(sin->sin_port) == 5201 && ntohl(sin->sin_addr.s_addr) == 0x7f000001);
	CHECK(ai->ai_next == NULL);
	freeaddrinfo(ai);
	hints.ai_flags = AI_PASSIVE;
	CHECK(getaddrinfo(NULL, "0", &hints, &ai) == 0);
	CHECK(((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr == htonl(INADDR_ANY));
	freeaddrinfo(ai);
	hints.ai_flags = 0;
	CHECK(getaddrinfo(NULL, "7", &hints, &ai) == 0);
	CHECK(((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr == htonl(INADDR_LOOPBACK));
	freeaddrinfo(ai);
	hints.ai_flags = AI_CANONNAME;
	CHECK(getaddrinfo("localhost", "80", &hints, &ai) == 0);
	CHECK(ai->ai_canonname != NULL && ntohs(((struct sockaddr_in *)ai->ai_addr)->sin_port) == 80);
	freeaddrinfo(ai);
	hints.ai_flags = AI_NUMERICHOST;
	CHECK(getaddrinfo("localhost", "80", &hints, &ai) == EAI_NONAME);
	hints.ai_flags = 0;
	CHECK(getaddrinfo("::1", "80", &hints, &ai) == EAI_FAMILY);
	hints.ai_family = AF_INET6;
	CHECK(getaddrinfo("127.0.0.1", "80", &hints, &ai) == EAI_FAMILY);
	hints.ai_family = AF_INET;
	CHECK(getaddrinfo("127.0.0.1", "99999", &hints, &ai) == EAI_SERVICE);
	CHECK(getaddrinfo("no-such-host.invalid", "80", &hints, &ai) == EAI_NONAME);
	CHECK(getaddrinfo(NULL, NULL, &hints, &ai) == EAI_NONAME);
	CHECK(strstr(gai_strerror(EAI_FAMILY), "IPv4") != NULL);

	if (!fails) printf("gfinet: all checks passed\n");
	return fails != 0;
}
